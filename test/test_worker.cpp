#include "worker/worker_helper_thread.hpp"

#include "worker/app_blocker.hpp"
#include "worker/kv_client_table.hpp"
//#include "worker/simple_range_manager.hpp"
#include <atomic>
#include <thread>

namespace csci5570 {
class SimpleRangeManager : public AbstractPartitionManager {
public:
  SimpleRangeManager(const std::vector<uint32_t>& server_thread_ids, int split)
          : AbstractPartitionManager(server_thread_ids), split_(split) {}

  void Slice(const Keys& keys, std::vector<std::pair<int, Keys>>* sliced) const override {
    size_t n = keys.size();
    sliced->resize(2);
    auto pos = std::lower_bound(keys.begin(), keys.end(), split_) - keys.begin();
    sliced->at(0).first = 0;
    sliced->at(0).second = keys.segment(0, pos);
    sliced->at(1).first = 1;
    sliced->at(1).second = keys.segment(pos, n);
  }

  void Slice(const KVPairs& kvs, std::vector<std::pair<int, KVPairs>>* sliced) const override {
    size_t n = kvs.first.size();
    sliced->resize(2);
    auto pos = std::lower_bound(kvs.first.begin(), kvs.first.end(), split_) - kvs.first.begin();
    sliced->at(0).first = server_thread_ids_[0];
    sliced->at(0).second.first = kvs.first.segment(0, pos);
    sliced->at(0).second.second = kvs.second.segment(0, pos);
    sliced->at(1).first = server_thread_ids_[1];
    sliced->at(1).second.first = kvs.first.segment(pos, n);
    sliced->at(1).second.second = kvs.second.segment(pos, n);
  }

private:
  int split_ = 0;
};

void TestBgWorker() {
  // Create app_blocker and worker_helper_thread
  AppBlocker app_blocker;
  WorkerHelperThread worker_helper_thread(0, &app_blocker);

  std::atomic<bool> ready(false);

  // Create a worker thread which runs the KVClientTable
  SimpleRangeManager range_manager({1, 2}, 2);
  ThreadsafeQueue<Message> downstream_queue;

  const uint32_t kTestAppThreadId1 = 1;
  const uint32_t kTestAppThreadId2 = 2;
  const uint32_t kTestModelId = 59;
  std::thread worker_thread1(
      [kTestAppThreadId1, kTestModelId, &app_blocker, &downstream_queue, &range_manager, &ready]() {
        while (!ready) {
          std::this_thread::yield();
        }
        KVClientTable<double> table(kTestAppThreadId1, kTestModelId, &downstream_queue, &range_manager, &app_blocker);
        // Add

        std::vector<Key> keys = {3, 4, 5, 6};
        std::vector<double> vals = {0.1, 0.1, 0.1, 0.1};
        table.Add(keys, vals);  // {3,4,5,6} -> {3}, {4,5,6}

        // Get
        std::vector<double> rets;
        table.Get(keys, &rets);

        CHECK_EQ(rets.size(), 4);
        LOG(INFO) << rets[0] << " " << rets[1] << " " << rets[2] << " " << rets[3];
      });
  std::thread worker_thread2(
      [kTestAppThreadId2, kTestModelId, &app_blocker, &downstream_queue, &range_manager, &ready]() {
        while (!ready) {
          std::this_thread::yield();
        }
        KVClientTable<double> table(kTestAppThreadId2, kTestModelId, &downstream_queue, &range_manager, &app_blocker);
        // Add
        std::vector<Key> keys = {3, 4, 5, 6};
        std::vector<double> vals = {0.1, 0.1, 0.1, 0.1};
        table.Add(keys, vals);  // {3,4,5,6} -> {3}, {4,5,6}

        // Get
        std::vector<double> rets;
        table.Get(keys, &rets);
        CHECK_EQ(rets.size(), 4);
        LOG(INFO) << rets[0] << " " << rets[1] << " " << rets[2] << " " << rets[3];
      });

  auto* work_queue = worker_helper_thread.GetWorkQueue();

  worker_helper_thread.Start();
  // resume the worker thread when worker_helper_thread starts
  ready = true;

  // Consume messages in downstream_queue
  const int kNumMessages = 8;
  for (int i = 0; i < kNumMessages; ++i) {
    Message msg;
    downstream_queue.WaitAndPop(&msg);
    LOG(INFO) << msg.DebugString();
  }
  // response
  Message msg1, msg2;
  msg1.meta.recver = kTestAppThreadId1;
  msg1.meta.model_id = kTestModelId;
  msg1.meta.flag = Flag::kGet;
  msg2.meta.recver = kTestAppThreadId1;
  msg2.meta.model_id = kTestModelId;
  msg2.meta.flag = Flag::kGet;
  third_party::SArray<Key> r1_keys{3};
  third_party::SArray<double> r1_vals{0.1};
  msg1.AddData(r1_keys);
  msg1.AddData(r1_vals);
  third_party::SArray<Key> r2_keys{4, 5, 6};
  third_party::SArray<double> r2_vals{0.4, 0.2, 0.3};
  msg2.AddData(r2_keys);
  msg2.AddData(r2_vals);
  work_queue->Push(msg1);
  work_queue->Push(msg2);

  // To worker_thread2
  msg1.meta.recver = kTestAppThreadId2;
  msg1.meta.model_id = kTestModelId;
  msg2.meta.recver = kTestAppThreadId2;
  msg2.meta.model_id = kTestModelId;
  work_queue->Push(msg1);
  work_queue->Push(msg2);

  Message exit_msg;
  exit_msg.meta.flag = Flag::kExit;
  work_queue->Push(exit_msg);

  worker_helper_thread.Stop();
  worker_thread1.join();
  worker_thread2.join();
}

}  // namespace csci5570

int main() { csci5570::TestBgWorker(); }
