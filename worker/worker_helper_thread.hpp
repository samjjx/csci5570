#pragma once

#include "worker/worker_thread.hpp"
#include "worker/abstract_callback_runner.hpp"
#include "driver/work_assigner.hpp"

namespace csci5570 {

class WorkerHelperThread : public AbstractWorkerThread {
 public:
  WorkerHelperThread(uint32_t worker_id, AbstractCallbackRunner* const callback_runner)
    : AbstractWorkerThread(worker_id), callback_runner_(callback_runner) {}

    void RegisterWorkAssigner(WorkAssigner* work_assigner);

 protected:
  void Main() override;
  void OnReceive(Message& msg) override;
 private:
  AbstractCallbackRunner* callback_runner_;
  WorkAssigner* work_assigner_;
  // there may be other functions
  //   Wait() and Nofify() for telling when parameters are ready

  // there may be other states such as
  //   a local copy of parameters
  //   some locking mechanism for notifying when parameters are ready
  //   a counter of msgs from servers, etc.
};

}  // namespace csci5570
