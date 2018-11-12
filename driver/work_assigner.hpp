#pragma once

#include "base/magic.hpp"

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
    WorkAssigner(DataRange range) : range_(range) {
      iter_num = 0;
      cur_sample = 0;
    };
    int next_sample() {
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
  private:
    DataRange range_;
    uint32_t iter_num;
    uint32_t cur_sample;
  };
}