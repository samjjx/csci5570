#include "server/consistency/bsp_model.hpp"
#include "glog/logging.h"

namespace csci5570 {

BSPModel::BSPModel(uint32_t model_id, std::unique_ptr<AbstractStorage>&& storage_ptr,
                   ThreadsafeQueue<Message>* reply_queue) {
  model_id_ = model_id;
  storage_ = std::move(storage_ptr);
  reply_queue_ = reply_queue;
}

void BSPModel::Clock(Message& msg) {
  if (msg.meta.model_id != model_id_) {return;}
  int min_clock = progress_tracker_.AdvanceAndGetChangedMinClock(msg.meta.sender);
  if (min_clock <= -1) {
    return;
  }
  // do update first
  for (auto& m : add_buffer_) {
    storage_->Add(m);
  }
  add_buffer_.clear();
  // get message for next iter
  for (auto& m : get_buffer_) {
    storage_->Get(m);
  }
  get_buffer_.clear();
}

void BSPModel::Add(Message& msg) {
  if (msg.meta.model_id != model_id_) {return;}
  // NOTICE: can only push Add-Message in current iter, will ignore other messages
  // Updating will be executed in BSPModel::Clock() when all worker advance one step
  if (progress_tracker_.GetProgress(msg.meta.sender) == progress_tracker_.GetMinClock()) {
    add_buffer_.push_back(msg);
  }
}

void BSPModel::Get(Message& msg) {
  if (msg.meta.model_id != model_id_) {return;}
  uint32_t tid = msg.meta.sender;
  // can only read after all updated
  if (progress_tracker_.GetProgress(tid) > progress_tracker_.GetMinClock()) {
    get_buffer_.push_back(msg);
  } else {
    Message reply_msg = storage_->Get(msg);
    reply_queue_->Push(reply_msg);
  }
}

int BSPModel::GetProgress(int tid) {
  return progress_tracker_.GetProgress(tid);
}

int BSPModel::GetGetPendingSize() {
  return get_buffer_.size();
}

int BSPModel::GetAddPendingSize() {
  return add_buffer_.size();
}

void BSPModel::ResetWorker(Message& msg) {
  std::vector<uint32_t> tids;
  auto msg_data = third_party::SArray<uint32_t>(msg.data[0]);
  for (uint32_t tid : msg_data) {tids.push_back(tid);}
  progress_tracker_.Init(tids);
  msg.meta.flag = Flag::kResetWorkerInModel;
  reply_queue_->Push(msg);
}

}  // namespace csci5570
