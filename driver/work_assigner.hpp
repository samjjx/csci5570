#pragma once

#include <cmath>
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
    DataRange() {
      start = 0;
      end = 0;
      length = 0;
    }
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
    WorkAssigner(DataRange range, DataRange helpee_range, ThreadsafeQueue<Message>* const sender_queue, uint32_t thread_id, uint32_t helpee)
      : range_(range), helpee_range_(helpee_range), sender_queue_(sender_queue), thread_id_(thread_id), helpee_id_(helpee) {
      assert(range.length > 0);
      iter_num = 0;
      cur_sample = 0;
      stop_idx = range_.end;
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
      if (helping_status == -1 && cur_sample - range_.start == uint32_t((stop_idx - range_.start) * check_point_)) {
        report_progress();
        std::unique_lock<std::mutex> lk(mu_);
        cond_.wait(lk, [this] {
          return helping_status != 1;
        }); // wait for report_progress response
      }

      if ( high_priority_queue.Size() != 0 ) {
        helping_status = 3;
        high_priority_queue.WaitAndPop(&helpee_cur_sample);
        if (high_priority_queue.Size() == 0 )
        {
          Message msg;
          msg.meta.sender = thread_id_;
          msg.meta.recver = helpee_id_;
          msg.meta.flag = Flag::kHelpCompleted;
          third_party::SArray<long> data;
          long timestamp = std::chrono::duration_cast< std::chrono::milliseconds >(
                  std::chrono::system_clock::now().time_since_epoch()
          ).count();
          data.push_back(timestamp);
          msg.AddData(data);
          sender_queue_->Push(msg);
          LOG(INFO) << "finished high_priority_queue";
        }
        return helpee_cur_sample;
      }

      // reach the end of my data
      if (cur_sample == stop_idx) {
            //LOG(INFO) << "reach end of my data ";
        if( low_priority_queue.Size() != 0 ) {
          low_priority_queue.WaitAndPop(&helpee_cur_sample);
          if ( helping_status == 2 )
          {
            Message m;
            m.meta.flag = Flag::kBegunHelping;
            m.meta.sender = thread_id_;
            m.meta.recver = helpee_id_;
            third_party::SArray<long> data;
            long timestamp = std::chrono::duration_cast< std::chrono::milliseconds >(
                    std::chrono::system_clock::now().time_since_epoch()
            ).count();
            data.push_back(timestamp);
            m.AddData(data);
            sender_queue_->Push(m);
            LOG(INFO) << "start low_priority_queue";
            helping_status = 3;
          }
          if (low_priority_queue.Size() == 0) {
            Message msg;
            msg.meta.sender = thread_id_;
            msg.meta.recver = helpee_id_;
            msg.meta.flag = Flag::kHelpCompleted;
            third_party::SArray<long> data;
            long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
            ).count();
            data.push_back(timestamp);
            msg.AddData(data);
            sender_queue_->Push(msg);
            LOG(INFO) << "finished low_priority_queue";
          }
          return helpee_cur_sample;
        }
        if (help_request_status == 1 && stop_idx < range_.end) {
          LOG(INFO) << "cancel_reassigment";
          // hasn't received begun-helping
          cancel_reassigment();
          stop_idx = range_.end;
          help_request_status = 0;
          return cur_sample++;
        } else if (help_request_status == 2) {
          // has begun helping, wait for helper
          std::unique_lock<std::mutex> lk(mu_);
          cond_.wait(lk, [this] {
            return help_request_status != 2;
          });
          LOG(INFO) << "helper complete help";
          help_request_status = 0;
          helping_status = -1;
          stop_idx = range_.end;
          // start over
          cur_sample = range_.start;
          iter_num++;
          return -1;
        } else {
          help_request_status = 0;
          helping_status = -1;
          stop_idx = range_.end;
          // start over
          cur_sample = range_.start;
          iter_num++;
          return -1;
        }
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
      helping_status = 1;
    }

    float check_is_behind(csci5570::Message &msg) const {
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
//      return progress_diff > 0.2;
      return progress_diff;
    }

    void ask_for_help(uint32_t helper_id, float ratio) {
      uint32_t amount = (uint32_t)std::floor(ratio * range_.length);
      if (amount == 0) {return;}
      uint32_t start = range_.end - amount;
      uint32_t end = range_.end;
      Message m;
      m.meta.flag = Flag::kDoThis;
      m.meta.sender = thread_id_;
      m.meta.recver = helper_id;
      third_party::SArray<long> data;
      long timestamp = std::chrono::duration_cast< std::chrono::milliseconds >(
              std::chrono::system_clock::now().time_since_epoch()
      ).count();
      // iteration, start, end, timestamp
      data.push_back(long(iter_num));
      data.push_back(long(start));
      data.push_back(long(end));
      data.push_back(timestamp);
      m.AddData(data);
      sender_queue_->Push(m);
      help_request_status = 1;
      helper_id_ = helper_id;
      stop_idx = start;
    }

    void tell_dont_help(uint32_t helper_id) {
      Message m;
      m.meta.flag = Flag::kDontNeedHelp;
      m.meta.sender = thread_id_;
      m.meta.recver = helper_id;
      third_party::SArray<long> data;
      long timestamp = std::chrono::duration_cast< std::chrono::milliseconds >(
              std::chrono::system_clock::now().time_since_epoch()
      ).count();
      // iteration, timestamp
      data.push_back(long(iter_num));
      data.push_back(timestamp);
      m.AddData(data);
      sender_queue_->Push(m);
    }

    void help_with_work(Message &msg) {
      // msg : do-this msg
      // data: 0 : iter
      //       1 : start
      //       2 : end
      //       3 : timestamp
      // transform into msg_data

      third_party::SArray<long> msg_data(msg.data[0]);

      if ( msg_data[3] > lastest_cancel_time ) {
        if ( msg_data[0] < iter_num ) {
          // BUG: if current thread had reach end, next_sample will not be called anymore
          // hence will never begin helping
          //send beginhelp
          Message m;
          m.meta.flag = Flag::kBegunHelping;
          m.meta.sender = thread_id_;
          m.meta.recver = helpee_id_;
          third_party::SArray<long> data;
          long timestamp = std::chrono::duration_cast< std::chrono::milliseconds >(
                  std::chrono::system_clock::now().time_since_epoch()
          ).count();
          data.push_back(timestamp);
          m.AddData(data);
          sender_queue_->Push(m);
          LOG(INFO) << "add to high_priority_queue";
          third_party::SArray<long> msg_data(msg.data[0]);
          for ( int i = msg_data[1]; i < msg_data[2]; i++ )
          {
            high_priority_queue.Push(i + range_.length);
          }
          std::lock_guard<std::mutex> lk(mu_);
          helping_status = 2;
          cond_.notify_all();
        }
        else if ( msg_data[0] == iter_num ) {
          LOG(INFO) << "add to low_priority_queue";
          third_party::SArray<long> msg_data(msg.data[0]);
          u_int32_t sample = msg_data[1] + range_.length;
          for ( int i = 0; i < msg_data[2] - msg_data[1]; i++ )
          {
            low_priority_queue.Push(sample);
            sample++;
          }
          std::lock_guard<std::mutex> lk(mu_);
          helping_status = 2;
          cond_.notify_all();
        }
      }
    }

    void cancel_reassigment() {
      Message m;
      m.meta.flag = Flag::kCancelHelp;
      m.meta.sender = thread_id_;
      m.meta.recver = helper_id_;
      // iteration, timestamp
      third_party::SArray<long> data;
      long timestamp = std::chrono::duration_cast< std::chrono::milliseconds >(
              std::chrono::system_clock::now().time_since_epoch()
      ).count();
      data.push_back(long(iter_num));
      data.push_back(timestamp);
      m.AddData(data);
      sender_queue_->Push(m);
      help_request_status = 0;
    }

    void onReceive(csci5570::Message &msg) {
      LOG(INFO) << msg.DebugString();
      if (msg.meta.flag == Flag::kProgressReport) {
        float progress_diff = check_is_behind(msg);
        float unfinished = double(range_.end - cur_sample) / range_.length;
        if (progress_diff > 0.2
            && double(range_.end - cur_sample) / range_.length > min_reassign_ratio) {
          float ratio = std::min(progress_diff / 2, unfinished / 2);
          LOG(INFO) << "I'm behind, I need help. Reassign " << ratio * 100 << "% of work";
          ask_for_help(uint32_t(msg.meta.sender), ratio);
        } else {
          tell_dont_help(msg.meta.sender);
        }
      }
      if (msg.meta.flag == Flag::kDontNeedHelp) {
        // helpee don't need help
        std::lock_guard<std::mutex> lk(mu_);
        helping_status = 0;
        cond_.notify_all();
      }
      if (msg.meta.flag == Flag::kDoThis) {
        third_party::SArray<long> msg_data(msg.data[0]);
        LOG(INFO) << "Rceived help request: " << msg_data[1] << "-" << msg_data[2];
        help_with_work(msg);
      }
      if (msg.meta.flag == Flag::kBegunHelping) {
        help_request_status = 2;
      }
      if (msg.meta.flag == Flag::kHelpCompleted) {
        std::lock_guard<std::mutex> lk(mu_);
        help_request_status = 3;
        cond_.notify_all();
      }
      if (msg.meta.flag == Flag::kCancelHelp){
          third_party::SArray<long> msg_data(msg.data[0]);
          lastest_cancel_time = msg_data[0];
      }
    }
  private:
    std::mutex mu_;
    std::condition_variable cond_;
    DataRange range_; // data range of mine
    DataRange helpee_range_; // data range of helpee
    uint32_t iter_num;
    uint32_t cur_sample;
    uint32_t thread_id_;
    uint32_t stop_idx; // for work reassigment
    std::clock_t start = 0;
    double check_point_ = 0.75; // progress check point of each iter
    double min_reassign_ratio = 0.05; // min percentage of work to reassign
    double avg_iter_time; // average time to complete an interation
    uint8_t help_request_status = 0; // 0: not sent 1: sent 2: begin 3: complete
    uint32_t helper_id_; // thread to help me
    uint32_t helpee_id_; // thread that may need my help
    ThreadsafeQueue<Message>* const sender_queue_; // not owned
    ThreadsafeQueue<Message> msg_queue_; // received msgs
    ThreadsafeQueue<u_int32_t> high_priority_queue;
    ThreadsafeQueue<u_int32_t> low_priority_queue;
    u_int32_t helpee_cur_sample;
    long lastest_cancel_time = 0;
    // -1: haven't asked, 0: no need to help, 1: waiting for helpee's response, 2: need to help but not start, 3: start helping
    int helping_status = -1;
  };
}
