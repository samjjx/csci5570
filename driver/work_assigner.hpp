#pragma once

#include "base/magic.hpp"
#include "base/message.hpp"
#include "base/threadsafe_queue.hpp"
#include "glog/logging.h"

namespace csci5570 {
  struct DataRange {
    DataRange(uint32_t start_, uint32_t end_)
      : start(start_), end(end_) {
      length = end - start;
    };
    uint32_t start;
    uint32_t end;
    uint32_t length;
  };

  class WorkAssigner {
  public:
    WorkAssigner() = delete;
    WorkAssigner(DataRange range, ThreadsafeQueue<Message>* const sender_queue, uint32_t thread_id, uint32_t helpee)
      : range_(range), sender_queue_(sender_queue), thread_id_(thread_id), helpee_id_(helpee) {
      iter_num = 0;
      cur_sample = 0;
    };

    int next_sample() {
      if (cur_sample == range_.end) {
        cur_sample = range_.start;
        iter_num++;
        report_progress();
        return -1;
      }
      return cur_sample++;
    }

    DataRange get_data_range() {
      return range_;
    }

    void report_progress() {
      LOG(INFO) << helpee_id_ << ", " << thread_id_;
      Message m;
      m.meta.flag = Flag::kProgressReport;
      m.meta.sender = thread_id_;
      m.meta.recver = helpee_id_;
      sender_queue_->Push(m);
    }

    void onReceive(csci5570::Message &msg) const {
      LOG(INFO) << msg.DebugString();
    }
  private:
    DataRange range_;
    uint32_t iter_num;
    uint32_t cur_sample;
    uint32_t thread_id_;
//    uint32_t helper_id_; // thread to help me
    uint32_t helpee_id_; // thread that may need my help
    ThreadsafeQueue<Message>* const sender_queue_;             // not owned
  };
}