//
// Created by jennings on 2018/10/8.
//

#ifndef CSCI5570_CALLBACK_RUNNER_HPP
#define CSCI5570_CALLBACK_RUNNER_HPP

#include <functional>
#include <iostream>

#include "worker/abstract_callback_runner.hpp"

namespace csci5570 {

class AppBlocker : public AbstractCallbackRunner {
public:
  AppBlocker() {}
  void RegisterRecvHandle(uint32_t app_thread_id, uint32_t model_id,
                          const std::function<void(Message&)>& recv_handle) override {
    std::lock_guard<std::mutex> lk(mu_);
    recv_handles_[app_thread_id] = recv_handle;
  }
  void RegisterRecvFinishHandle(uint32_t app_thread_id, uint32_t model_id,
                                const std::function<void()>& recv_finish_handle) override {
    std::lock_guard<std::mutex> lk(mu_);
    recv_finish_handles_[app_thread_id] = recv_finish_handle;
  }

  void NewRequest(uint32_t app_thread_id, uint32_t model_id, uint32_t expected_responses) override {
    std::unique_lock<std::mutex> lk(mu_);
    tracker_[app_thread_id] = {expected_responses, 0};
  }
  void WaitRequest(uint32_t app_thread_id, uint32_t model_id) override {
    std::unique_lock<std::mutex> lk(mu_);
    cond_.wait(lk, [this, app_thread_id, model_id] {
        return tracker_[app_thread_id].first == tracker_[app_thread_id].second;
    });
  }
  void AddResponse(uint32_t app_thread_id, uint32_t model_id, Message& m) override {
    bool recv_finish = false;
    {
      std::lock_guard<std::mutex> lk(mu_);
      recv_finish = tracker_[app_thread_id].first == tracker_[app_thread_id].second + 1 ? true : false;
    }
    recv_handles_[app_thread_id](m);
    if (recv_finish) {
      recv_finish_handles_[app_thread_id]();
    }
    {
      std::lock_guard<std::mutex> lk(mu_);
      tracker_[app_thread_id].second += 1;
      if (recv_finish) {
        cond_.notify_all();
      }
    }
  }

private:
  // TODO: use both app_thread_id and model_id?
  std::map<uint32_t, std::function<void(Message&)>> recv_handles_;
  std::map<uint32_t, std::function<void()>> recv_finish_handles_;

  std::mutex mu_;
  std::condition_variable cond_;
  std::map<uint32_t, std::pair<uint32_t, uint32_t>> tracker_;
};

}
#endif //CSCI5570_CALLBACK_RUNNER_HPP
