#pragma once

#include "base/abstract_partition_manager.hpp"
#include "base/magic.hpp"
#include "base/message.hpp"
#include "base/third_party/sarray.h"
#include "base/threadsafe_queue.hpp"
#include "worker/abstract_callback_runner.hpp"

#include <cinttypes>
#include <vector>

namespace csci5570 {

/**
 * Provides the API to users, and implements the worker-side abstraction of model
 * Each model in one application is uniquely handled by one KVClientTable
 *
 * @param Val type of model parameter values
 */
template <typename Val>
class KVClientTable {
 public:
  /**
   * @param app_thread_id       user thread id
   * @param model_id            model id
   * @param sender_queue        the work queue of a sender communication thread
   * @param partition_manager   model partition manager
   * @param callback_runner     callback runner to handle received replies from servers
   */
  KVClientTable(uint32_t app_thread_id, uint32_t model_id, ThreadsafeQueue<Message>* const sender_queue,
                const AbstractPartitionManager* const partition_manager, AbstractCallbackRunner* const callback_runner)
      : app_thread_id_(app_thread_id),
        model_id_(model_id),
        sender_queue_(sender_queue),
        partition_manager_(partition_manager),
        callback_runner_(callback_runner){};





  // ========== API ========== //
  void Clock();

  // what ¿

  // vector version
  void Add(const std::vector<Key>& keys, const std::vector<Val>& vals) {
      // test code keys {3,4,5,6} -> {3},{4,5,6}

      Message msg;
      msg.meta.flag = Flag::kAdd; // indicate Add operation to server thread
      msg.meta.sender = this->app_thread_id_; //sender_id
      uint32_t server_threads_num = partition_manager_->GetNumServers();
      std::vector<uint32_t> server_threads_id = partition_manager_->GetServerThreadIds();
      partition_manager_->Slice(keys, vals); //partition keys into slices
      msg.data = vals; // vals -> data
       while ( server_threads_num ) {
          msg.meta.recver = server_threads_id[server_threads_num-1];
          msg.meta.model_id = model_id_;
          sender_queue_->Push(msg);
          server_threads_num--;
      }
  }
  void Get(const std::vector<Key>& keys, std::vector<Val>* vals) {
      // all tasks done, get sth user need from server

      callback_runner_->RegisterRecvFinishHandle(app_thread_id_, model_id_, this->FinalResult());
      // implement callback runner

      // lol discussion

  }
  // sarray version
  void Add(const third_party::SArray<Key>& keys, const third_party::SArray<Val>& vals) {
      Message msg;
      msg.meta.flag = Flag::kAdd; // indicate Add operation to server thread
      msg.meta.sender = this->app_thread_id_; //sender_id
      uint32_t server_threads_num = partition_manager_->GetNumServers();
      std::vector<uint32_t> server_threads_id = partition_manager_->GetServerThreadIds();
      partition_manager_->Slice(keys, vals); //partition keys into slices
      // partition_manager slice function has 2 versions : vector && SArray
      msg.data = vals; // vals -> data
      while ( server_threads_num ) {
          msg.meta.recver = server_threads_id[server_threads_num-1];
          msg.meta.model_id = model_id_;
          sender_queue_->Push(msg);
          server_threads_num--;
      }


  }
  void Get(const third_party::SArray<Key>& keys, third_party::SArray<Val>* vals) {
      callback_runner_->RegisterRecvFinishHandle(app_thread_id_, model_id_, this->FinalResult());
  }
  // ========== API ========== //

 private:
  uint32_t app_thread_id_;  // identifies the user thread
  uint32_t model_id_;       // identifies the model on servers

  ThreadsafeQueue<Message>* const sender_queue_;             // not owned
  AbstractCallbackRunner* const callback_runner_;            // not owned
  const AbstractPartitionManager* const partition_manager_;  // not owned

  void FinalResult();

};  // class KVClientTable

}  // namespace csci5570
