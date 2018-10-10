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
// TODO
// Nodes should be parsed from params
Node n1{0, "localhost", 12352};
Node n2{3, "localhost", 12353};
Node n3{4, "localhost", 12354};

void Engine::StartEverything(int num_server_threads_per_node) {
  CreateIdMapper(num_server_threads_per_node);
  CreateMailbox();
  StartSender();
  StartServerThreads();
  StartWorkerThreads();
  StartMailbox();
}
void Engine::CreateIdMapper(int num_server_threads_per_node) {
  std::unique_ptr<SimpleIdMapper> mapper_ptr(new SimpleIdMapper(n1, {n1, n2, n3}));
  mapper_ptr->Init(num_server_threads_per_node);
  id_mapper_ = std::move(mapper_ptr);
}
void Engine::CreateMailbox() {
  std::unique_ptr<Mailbox> mailbox_ptr(new Mailbox(n1, {n1, n2, n3}, id_mapper_.get()));
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
    mailbox_->RegisterQueue(wid, worker_thread->GetWorkQueue());
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
  StopServerThreads();
  StopWorkerThreads();
  StopSender();
  StopMailbox();
}
void Engine::StopServerThreads() {
  for(auto& server_thread : server_thread_group_) {
    // TODO: send exit message
    server_thread->Stop();
  }
}
void Engine::StopWorkerThreads() {
  for(auto& worker_thread : worker_thread_group_) {
    // TODO: send exit message
    worker_thread->Stop();
  }
}
void Engine::StopSender() {
  sender_->Stop();
}
void Engine::StopMailbox() {
  mailbox_->Stop();
}

void Engine::Barrier() {
  // TODO
}

WorkerSpec Engine::AllocateWorkers(const std::vector<WorkerAlloc>& worker_alloc) {
  WorkerSpec spec(worker_alloc);
  std::map<uint32_t, std::vector<uint32_t>> node_to_workers = spec.GetNodeToWorkers();
  for (auto& pair : node_to_workers) {
    uint32_t node_id = pair.first;
    std::vector<uint32_t> workers = pair.second;
    // register workers
    for (auto worker_id : workers) {
      int thread_id = id_mapper_->AllocateWorkerThread(node_id);
      if (thread_id == -1) {
        throw "Allocate worker thread failed!";
      }
      spec.InsertWorkerIdThreadId(worker_id, thread_id);
    }
  }
  return spec;
}

void Engine::InitTable(uint32_t table_id, const std::vector<uint32_t>& worker_ids) {
  // TODO
}

void Engine::Run(const MLTask& task) {
  // TODO
}

void Engine::RegisterPartitionManager(uint32_t table_id, std::unique_ptr<AbstractPartitionManager> partition_manager) {
  partition_manager_map_[table_id] = std::move(partition_manager);
}

}  // namespace csci5570
