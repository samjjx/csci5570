#pragma once

#include "glog/logging.h"
#include "base/abstract_partition_manager.hpp"
#include "base/magic.hpp"
#include "base/message.hpp"
#include "base/third_party/sarray.h"
#include "base/threadsafe_queue.hpp"
#include "worker/abstract_callback_runner.hpp"

#include <cinttypes>
#include <vector>
#include <iostream>

namespace csci5570 {

/**
 * Provides the API to users, and implements the worker-side abstraction of model
 * Each model in one application is uniquely handled by one KVClientTable
 *
 * @param Val type of model parameter values
 */
template <typename Val>
class KVClientTable {
  using Keys = third_party::SArray<Key>;
  using Vals = third_party::SArray<Val>;
  using KVPairs = std::pair<third_party::SArray<Key>, third_party::SArray<Val>>;
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
        callback_runner_(callback_runner) {
  };

  // ========== API ========== //
  void Clock() {
    auto server_ids = partition_manager_->GetServerThreadIds();
    for (auto sid : server_ids) {
      Message m;
      m.meta.flag = Flag::kClock;
      m.meta.model_id = model_id_;
      m.meta.sender = app_thread_id_;
      m.meta.recver = sid;
      sender_queue_->Push(m);
    }
  }
  // vector version
  void Add(const std::vector<Key>& keys, const std::vector<Val>& vals) {
    Add(Keys(keys), Vals(vals));
  }
  void Get(const std::vector<Key>& keys, std::vector<Val>* vals) {
    Vals* vals_ = new Vals();
    Get(Keys(keys), vals_);
    vals->insert(vals->end(), vals_->begin(), vals_->end());
    delete vals_;
  }
  // sarray version
  void Add(const third_party::SArray<Key>& keys, const third_party::SArray<Val>& vals) {
    std::vector<std::pair<int, KVPairs>> sliced_pairs;
    partition_manager_->Slice(std::make_pair(keys, vals), &sliced_pairs);
    for (auto& server_kv : sliced_pairs) {
      Message m;
      m.meta.flag = Flag::kAdd;
      m.meta.model_id = model_id_;
      // QUESTION: should I use app_thread_id_?
      m.meta.sender = app_thread_id_;
      m.meta.recver = server_kv.first;
      m.AddData(server_kv.second.first);
      m.AddData(server_kv.second.second);
      sender_queue_->Push(m);
    }
  }
  void Get(const third_party::SArray<Key>& keys, third_party::SArray<Val>* vals) {
    std::vector<std::pair<int, Keys>> sliced_keys;
    partition_manager_->Slice(keys, &sliced_keys);
    std::map<Key, Val> cache;
    callback_runner_->RegisterRecvHandle(app_thread_id_, model_id_, [&cache](Message& msg) {
      Keys data_keys(msg.data[0]);
      Vals data_vals(msg.data[1]);
      for(uint32_t i = 0; i < data_keys.size(); i++) {
        cache[data_keys[i]] = data_vals[i];
      }
    });
    callback_runner_->RegisterRecvFinishHandle(app_thread_id_, model_id_, []() {

    });
    callback_runner_->NewRequest(app_thread_id_, model_id_, sliced_keys.size());
    for (auto& server_keys : sliced_keys) {
      Message m;
      m.meta.flag = Flag::kGet;
      m.meta.model_id = model_id_;
      // QUESTION: should I use app_thread_id_?
      m.meta.sender = app_thread_id_;
      m.meta.recver = server_keys.first;
      m.AddData(server_keys.second);
      sender_queue_->Push(m);
    }
    callback_runner_->WaitRequest(app_thread_id_, model_id_);
    for (auto k : keys) {
      vals->push_back(cache[k]);
    }
  }
  // ========== API ========== //

 private:
  uint32_t app_thread_id_;  // identifies the user thread
  uint32_t model_id_;       // identifies the model on servers

  ThreadsafeQueue<Message>* const sender_queue_;             // not owned
  AbstractCallbackRunner* const callback_runner_;            // not owned
  const AbstractPartitionManager* const partition_manager_;  // not owned

};  // class KVClientTable

}  // namespace csci5570
