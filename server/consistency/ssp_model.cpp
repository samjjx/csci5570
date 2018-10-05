#include "server/consistency/ssp_model.hpp"
#include "glog/logging.h"

namespace csci5570 {

SSPModel::SSPModel(uint32_t model_id, std::unique_ptr<AbstractStorage>&& storage_ptr, int staleness,
                   ThreadsafeQueue<Message>* reply_queue) {
  model_id_ = model_id;
  storage_ = std::move(storage_ptr);
  staleness_ = staleness;
  reply_queue_ = reply_queue;
}

void SSPModel::Clock(Message& msg) {
  int min_clock = progress_tracker_.AdvanceAndGetChangedMinClock(msg.meta.sender);
  if (min_clock <= -1) {
    return;
  }
  // handle pending messages
  std::vector<Message> pending_msgs = buffer_.Pop(min_clock);
  for (Message m : pending_msgs) {
    if (m.meta.flag == Flag::kAdd) {
      Add(m);
    }
    if (m.meta.flag == Flag::kGet) {
      Get(m);
    }
  }
}

void SSPModel::Add(Message& msg) {
  storage_->Add(msg);
}

void SSPModel::Get(Message& msg) {
  int worker_progress = progress_tracker_.GetProgress(msg.meta.sender);
  int min_clock = progress_tracker_.GetMinClock();
  if (worker_progress - min_clock > staleness_) {
    // wait for other workers
    int barrier_clock = worker_progress - staleness_;
    buffer_.Push(barrier_clock, msg);
  } else {
    // response immediately
    Message reply_msg = storage_->Get(msg);
    reply_queue_->Push(reply_msg);
  }
}

int SSPModel::GetProgress(int tid) {
  return progress_tracker_.GetProgress(tid);
}

int SSPModel::GetPendingSize(int progress) {
  return buffer_.Size(progress);
}

void SSPModel::ResetWorker(Message& msg) {
  std::vector<uint32_t> tids;
  auto msg_data = third_party::SArray<uint32_t>(msg.data[0]);
  for (uint32_t tid : msg_data) {tids.push_back(tid);}
  progress_tracker_.Init(tids);
  msg.meta.flag = Flag::kResetWorkerInModel;
  reply_queue_->Push(msg);
}

}  // namespace csci5570
