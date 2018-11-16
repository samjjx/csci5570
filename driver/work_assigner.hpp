#pragma once

#include "base/magic.hpp"
#include "base/message.hpp"
#include "base/threadsafe_queue.hpp"
#include "glog/logging.h"
#include <limits>
#include <ctime>
#include <algorithm>
#include <assert.h>
#include <chrono>

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
      assert(range.length > 0);
      iter_num = 0;
      cur_sample = 0;
      avg_iter_time = std::numeric_limits<double>::infinity();
    };

    int next_sample() {
      // update avg_iter_time in sliding window manner
      uint32_t window_size = std::min(uint32_t(100), uint32_t(std::floor(range_.length * check_point_)));
      window_size = std::max(window_size, uint32_t(1));
      if (cur_sample % window_size == 0) {
        std::clock_t now = std::clock();
        if (start > 0) {
          avg_iter_time = ( now - start ) / (double) CLOCKS_PER_SEC * range_.length / window_size;
        }
        start = now;
      }
      if (cur_sample - range_.start == uint32_t(range_.length * check_point_)) {
        report_progress();
      }
      if (cur_sample == range_.end) {
        cur_sample = range_.start;
        iter_num++;
        return -1;
      }
      return cur_sample++;
    }

    DataRange get_data_range() {
      return range_;
    }

    void report_progress() {
      Message m;
      m.meta.flag = Flag::kProgressReport;
      m.meta.sender = thread_id_;
      m.meta.recver = helpee_id_;
      // current iteration, progress percentage, timestamp
      third_party::SArray<long> data;
      data.push_back(long(iter_num));
      data.push_back(long(double(cur_sample - range_.start)/range_.length * 100));
      long timestamp = std::chrono::duration_cast< std::chrono::milliseconds >(
              std::chrono::system_clock::now().time_since_epoch()
      ).count();
      data.push_back(timestamp);
      m.AddData(data);
      sender_queue_->Push(m);
    }

    bool check_is_behind(csci5570::Message &msg) const {
      third_party::SArray<long> msg_data(msg.data[0]);
      long his_iter_num = msg_data[0];
      long his_progress = msg_data[1];
      long timestamp = msg_data[2]; // in ms
      LOG(INFO) << "my iter_num: " << iter_num << " his iter_num: " << his_iter_num;
      if (his_iter_num < iter_num) {return false;}
      long my_progress = float(cur_sample - range_.start) / range_.length * 100;
      LOG(INFO) << "my progress: " << my_progress << " his progress: " << his_progress;
      double completion_diff = double(his_progress - my_progress) / 100;
      long now = std::chrono::duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch()
      ).count();
      double time_diff = double(now - timestamp) / 1000;
      double progress_diff = completion_diff + std::min(time_diff / avg_iter_time, 1-check_point_);
      return progress_diff > 0.2;
    }

    void onReceive(csci5570::Message &msg) const {
      LOG(INFO) << msg.DebugString();
      if (msg.meta.flag == Flag::kProgressReport) {
        if (check_is_behind(msg)) {
          LOG(INFO) << "I'm behind, I need help.";
        }
      }
    }
  private:
    DataRange range_;
    uint32_t iter_num;
    uint32_t cur_sample;
    uint32_t thread_id_;
    std::clock_t start = 0;
    double check_point_ = 0.75; // progress check point of each iter
    double avg_iter_time; // average time to complete an interation
//    uint32_t helper_id_; // thread to help me
    uint32_t helpee_id_; // thread that may need my help
    ThreadsafeQueue<Message>* const sender_queue_;             // not owned
    ThreadsafeQueue<Message> msg_queue_; // received msgs
  };
}