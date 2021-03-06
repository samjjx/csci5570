#include "driver/engine.hpp"

#include <vector>

#include "base/abstract_partition_manager.hpp"
#include "base/node.hpp"
#include "comm/mailbox.hpp"
#include "comm/sender.hpp"
#include "driver/ml_task.hpp"
#include "driver/simple_id_mapper.hpp"
#include "driver/worker_spec.hpp"
#include "server/server_thread.hpp"
#include "worker/abstract_callback_runner.hpp"
#include "worker/worker_helper_thread.hpp"

namespace csci5570 {

void Engine::StartEverything(int num_server_threads_per_node) {
  CreateIdMapper(num_server_threads_per_node);
  CreateMailbox();
  StartSender();
  StartServerThreads();
  StartWorkerThreads();
  StartMailbox();
}
void Engine::CreateIdMapper(int num_server_threads_per_node) {
  std::unique_ptr<SimpleIdMapper> mapper_ptr(new SimpleIdMapper(node_, nodes_));
  mapper_ptr->Init(num_server_threads_per_node);
  id_mapper_ = std::move(mapper_ptr);
}
void Engine::CreateMailbox() {
  std::unique_ptr<Mailbox> mailbox_ptr(new Mailbox(node_, nodes_, id_mapper_.get()));
  mailbox_ = std::move(mailbox_ptr);
}
void Engine::StartServerThreads() {
  std::vector<uint32_t> local_servers = id_mapper_->GetServerThreadsForId(node_.id);
  for (uint32_t sid : local_servers) {
    std::unique_ptr<ServerThread>server_thread (new ServerThread(sid));
    server_thread->Start();
    mailbox_->RegisterQueue(sid, server_thread->GetWorkQueue());
    server_thread_group_.push_back(std::move(server_thread));
  }
}
void Engine::StartWorkerThreads() {
  std::vector<uint32_t> local_workers = id_mapper_->GetWorkerHelperThreadsForId(node_.id);
  for (uint32_t wid : local_workers) {
    std::unique_ptr<WorkerHelperThread>worker_thread (new WorkerHelperThread(wid, callback_runner_.get()));
    worker_thread->Start();
    worker_thread_group_.push_back(std::move(worker_thread));
  }
}
void Engine::StartMailbox() {
  mailbox_->Start();
}
void Engine::StartSender() {
  std::unique_ptr<Sender> sender(new Sender(mailbox_.get()));
  sender->Start();
  sender_ = std::move(sender);
}

void Engine::StopEverything() {
  StopSender();
  StopMailbox();
  StopServerThreads();
  StopWorkerThreads();
}
void Engine::StopServerThreads() {
  for(auto& server_thread : server_thread_group_) {
    server_thread->Stop();
  }
  LOG(INFO) << "StopServerThreads";
}
void Engine::StopWorkerThreads() {
  for(auto& worker_thread : worker_thread_group_) {
    Message exit_msg;
    exit_msg.meta.flag = Flag::kExit;
    worker_thread->GetWorkQueue()->Push(exit_msg);
    worker_thread->Stop();
  }
  LOG(INFO) << "StopWorkerThreads";
}
void Engine::StopSender() {
  mailbox_->Barrier();
  sender_->Stop();
  LOG(INFO) << "StopSender";
}
void Engine::StopMailbox() {
  mailbox_->Stop();
  LOG(INFO) << "StopMailbox";
}

void Engine::Barrier() {
  // TODO
  mailbox_->Barrier();
}

WorkerSpec Engine::AllocateWorkers(const std::vector<WorkerAlloc>& worker_alloc) {
  WorkerSpec spec(worker_alloc);
  std::map<uint32_t, std::vector<uint32_t>> node_to_workers = spec.GetNodeToWorkers();
  for (auto& pair : node_to_workers) {
    uint32_t node_id = pair.first;
    if (node_.id != node_id) {continue;}
    std::vector<uint32_t> workers = pair.second;
    // register workers
    for (auto worker_id : workers) {
      int thread_id = id_mapper_->AllocateWorkerThread(node_id);
      if (thread_id == -1) {
        throw "Allocate worker thread failed!";
      }
      spec.InsertWorkerIdThreadId(worker_id, thread_id);
      // register worker_thread_queue
      uint32_t helper_thread_id = id_mapper_->GetHelperForWorker(thread_id);
      LOG(INFO) << "bind worker_thread: " << thread_id << " with helper thread: " << helper_thread_id;
      for(auto& helper_thread : worker_thread_group_) {
        if (helper_thread->GetId() == helper_thread_id) {
          mailbox_->RegisterQueue(thread_id, helper_thread->GetWorkQueue());
          break;
        }
      }
    }
  }
  return spec;
}

void Engine::InitTable(uint32_t table_id, const std::vector<uint32_t>& worker_ids) {
  std::vector<uint32_t> server_ids = id_mapper_->GetAllServerThreads();
  for(auto sid : server_ids) {
    Message reset_msg;
    reset_msg.meta.model_id = table_id;
    reset_msg.meta.flag = Flag::kResetWorkerInModel;
    reset_msg.meta.recver = sid;
    reset_msg.meta.sender = worker_ids[0];
    third_party::SArray<uint32_t> tids(worker_ids);
    reset_msg.AddData(tids);
    sender_->GetMessageQueue()->Push(reset_msg);
  }
}

void Engine::Run(const MLTask& task) {
  if (!task.IsSetup()) {return;}
  const std::vector<WorkerAlloc>& worker_allocs = task.GetWorkerAlloc();
  WorkerSpec spec = AllocateWorkers(worker_allocs);
  if (!spec.HasLocalWorkers(node_.id)) {return;}
  // spawned user worker threads
  const std::vector<uint32_t>& worker_ids = spec.GetLocalWorkers(node_.id);
  const std::vector<uint32_t>& thread_ids = spec.GetLocalThreads(node_.id);
  std::vector<std::thread> threads;
  // Is this correct?
  auto tables = task.GetTables();
  for (uint32_t table_id : tables) {
    InitTable(table_id, thread_ids);
  }
  for(uint32_t i = 0; i < worker_ids.size(); i++) {
    uint32_t thread_id = thread_ids[i];
    uint32_t worker_id = worker_ids[i];

    std::thread thread(
      [thread_id, worker_id, &task, this]() {
        Info info;
        info.thread_id = thread_id;
        info.worker_id = worker_id;
        info.send_queue = sender_->GetMessageQueue();
        for(auto& kv : partition_manager_map_) {
          info.partition_manager_map[kv.first] = kv.second.get();
        }
        info.callback_runner = callback_runner_.get();
        task.RunLambda(info);
        // free worker thread id
        id_mapper_->DeallocateWorkerThread(node_.id, thread_id);
      });
    threads.push_back(std::move(thread));
  }
  for(std::thread& th : threads) {
    if (th.joinable()) {
      th.join();
    }
  }
}

void Engine::RegisterPartitionManager(uint32_t table_id, std::unique_ptr<AbstractPartitionManager>&& partition_manager) {
  partition_manager_map_[table_id] = std::move(partition_manager);
}

}  // namespace csci5570
