#include "glog/logging.h"
#include "gtest/gtest.h"

#include "server/server_thread.hpp"
#include "server/server_thread_group.hpp"
#include "server/consistency/ssp_model.hpp"
#include "server/map_storage.hpp"

#include <iostream>

namespace csci5570 {

void TestServer() {
  // This should be owned by the sender
  ThreadsafeQueue<Message> reply_queue;

  // Create a group of ServerThread
  std::vector<uint32_t> server_id_vec{0, 1};
  ServerThreadGroup server_thread_group(server_id_vec, &reply_queue);

  // Create models and register models into ServerThread
  const int num_models = 3;
  int model_staleness = 1;
  for (int i = 0; i < num_models; ++i) {
    for (auto& server_thread : server_thread_group) {
      std::unique_ptr<AbstractStorage> storage(new MapStorage<int>());
      std::unique_ptr<AbstractModel> model(
          new SSPModel(i, std::move(storage), model_staleness, server_thread_group.GetReplyQueue()));
      server_thread->RegisterModel(i, std::move(model));
    }
  }

  // Collect server queues
  std::map<uint32_t, ThreadsafeQueue<Message>*> server_queues;
  for (auto& server_thread : server_thread_group) {
    server_queues.insert({server_thread->GetServerId(), server_thread->GetWorkQueue()});
  }

  // Dispatch messages to queues
  std::vector<Message> messages;

  // Start
  for (auto& server_thread : server_thread_group) {
    server_thread->Start();
  }

  // ResetWorker
  third_party::SArray<uint32_t> tids{2, 3};
  for (auto& queue : server_queues) {
    for (int i = 0; i < num_models; ++ i) {
      Message reset_msg;
      reset_msg.meta.model_id = i;
      reset_msg.meta.recver = queue.first;
      reset_msg.meta.flag = Flag::kResetWorkerInModel;
      reset_msg.AddData(tids);
      queue.second->Push(reset_msg);
    }
  }
  for (auto& queue : server_queues) {
    for (int i = 0; i < num_models; ++ i) {
      Message reset_reply_msg;
      reply_queue.WaitAndPop(&reset_reply_msg);
    }
  }

  // ADD msg
  {
    Message m1;
    m1.meta.flag = Flag::kAdd;
    m1.meta.model_id = 0;
    m1.meta.sender = 2;
    m1.meta.recver = 0;
    third_party::SArray<int> m1_keys({0});
    third_party::SArray<int> m1_vals({1});
    m1.AddData(m1_keys);
    m1.AddData(m1_vals);
    messages.push_back(m1);

    Message m2;
    m2.meta.flag = Flag::kAdd;
    m2.meta.model_id = 0;
    m2.meta.sender = 3;
    m2.meta.recver = 0;
    third_party::SArray<int> m2_keys({1});
    third_party::SArray<int> m2_vals({2});
    m2.AddData(m2_keys);
    m2.AddData(m2_vals);
    messages.push_back(m2);
  }

  // GET msg
  {
    // Message1
    Message m1;
    m1.meta.flag = Flag::kGet;
    m1.meta.model_id = 0;
    m1.meta.sender = 2;
    m1.meta.recver = 0;
    third_party::SArray<int> m1_keys({0});
    m1.AddData(m1_keys);
    messages.push_back(m1);

    // Message2
    Message m2;
    m2.meta.flag = Flag::kGet;
    m2.meta.model_id = 0;
    m2.meta.sender = 3;
    m2.meta.recver = 0;
    third_party::SArray<int> m2_keys({1});
    m2.AddData(m2_keys);
    messages.push_back(m2);
  }

  // Push the messages into queues
  for (auto& msg : messages) {
    CHECK(server_queues.find(msg.meta.recver) != server_queues.end());
    server_queues[msg.meta.recver]->Push(msg);
  }

  // Check
  Message check_message;
  auto rep_keys = third_party::SArray<int>();
  auto rep_vals = third_party::SArray<int>();

  reply_queue.WaitAndPop(&check_message);
  CHECK(check_message.data.size() == 2);
  rep_keys = third_party::SArray<int>(check_message.data[0]);
  rep_vals = third_party::SArray<int>(check_message.data[1]);
  CHECK(rep_keys.size() == 1);
  CHECK(rep_vals.size() == 1);
  CHECK(rep_keys[0] == 0);
  CHECK(rep_vals[0] == 1);

  reply_queue.WaitAndPop(&check_message);
  CHECK(check_message.data.size() == 2);
  rep_keys = third_party::SArray<int>(check_message.data[0]);
  rep_vals = third_party::SArray<int>(check_message.data[1]);
  CHECK(rep_keys.size() == 1);
  CHECK(rep_vals.size() == 1);
  CHECK(rep_keys[0] == 1);
  CHECK(rep_vals[0] == 2);

  // EXIT msg
  {
    Message exit_msg;
    exit_msg.meta.flag = Flag::kExit;
    server_queues[0]->Push(exit_msg);
    server_queues[1]->Push(exit_msg);
  }

  // Stop
  for (auto& server_thread : server_thread_group) {
    server_thread->Stop();
  }
}
}  // namespace csci5570

int main() { csci5570::TestServer(); }
