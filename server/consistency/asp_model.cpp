#include "server/consistency/asp_model.hpp"
#include "glog/logging.h"

namespace csci5570 {

ASPModel::ASPModel(uint32_t model_id, std::unique_ptr<AbstractStorage>&& storage_ptr,
                   ThreadsafeQueue<Message>* reply_queue) {
  model_id_ = model_id;
  storage_ = std::move(storage_ptr);
  reply_queue_ = reply_queue;
}

void ASPModel::Clock(Message& msg) {
  if (msg.meta.model_id != model_id_) {return;}
  progress_tracker_.AdvanceAndGetChangedMinClock(msg.meta.sender);
}

void ASPModel::Add(Message& msg) {
  if (msg.meta.model_id != model_id_) {return;}
  storage_->Add(msg);
}

void ASPModel::Get(Message& msg) {
  if (msg.meta.model_id != model_id_) {return;}
  Message reply_msg = storage_->Get(msg);
  reply_queue_->Push(reply_msg);
}

int ASPModel::GetProgress(int tid) {
  return progress_tracker_.GetProgress(tid);
}

void ASPModel::ResetWorker(Message& msg) {
  std::vector<uint32_t> tids;
  auto msg_data = third_party::SArray<uint32_t>(msg.data[0]);
  for (uint32_t tid : msg_data) {tids.push_back(tid);}
  progress_tracker_.Init(tids);
  Message response;
  response.meta.sender = msg.meta.recver;
  response.meta.recver = msg.meta.sender;
  response.meta.flag = Flag::kResetWorkerInModel;
  reply_queue_->Push(response);
}

}  // namespace csci5570
