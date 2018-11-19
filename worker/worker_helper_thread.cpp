//
// Created by jennings on 2018/10/8.
//
#include "glog/logging.h"
#include "worker/worker_helper_thread.hpp"

namespace csci5570 {

void WorkerHelperThread::Main() {
  while(true) {
    Message msg;
    work_queue_.WaitAndPop(&msg);
    if (msg.meta.flag == Flag::kExit) {
      break;
    }
    if (msg.meta.flag == Flag::kGet) {
      callback_runner_->AddResponse(msg.meta.recver, msg.meta.model_id, msg);
    }
    if (msg.meta.flag != Flag::kGet) {
      // pass other msgs to work_assigner
      if (work_assigner_ != nullptr) {
        work_assigner_->onReceive(msg);
      }
    }
  }
}

void WorkerHelperThread::RegisterWorkAssigner(WorkAssigner* work_assigner) {
  work_assigner_ = work_assigner;
}

void WorkerHelperThread::OnReceive(csci5570::Message &msg) {}

}