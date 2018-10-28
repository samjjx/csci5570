#include "server/util/progress_tracker.hpp"
#include <algorithm>
#include "glog/logging.h"

namespace csci5570 {

void ProgressTracker::Init(const std::vector<uint32_t>& tids) {
  // should this method clear all progresses ?
//  progresses_.clear();
  for (auto tid : tids) {
    progresses_[tid] = 0;
  }
  min_clock_ = 0;
}

int ProgressTracker::AdvanceAndGetChangedMinClock(int tid) {
  if (!CheckThreadValid(tid)) {
    return -1;
  }
  int changed_min_clock = -1;
  if (IsUniqueMin(tid)) {
    min_clock_ += 1;
    changed_min_clock = min_clock_;
  }
  progresses_[tid] += 1;
  return changed_min_clock;
}

int ProgressTracker::GetNumThreads() const {
  return progresses_.size();
}

int ProgressTracker::GetProgress(int tid) const {
  if (!CheckThreadValid(tid)) {
    return -1;
  }
  return progresses_.find(tid)->second;
}

int ProgressTracker::GetMinClock() const {
  return min_clock_;
}

bool ProgressTracker::IsUniqueMin(int tid) const {
  int progress = GetProgress(tid);
  if (progress == -1 || progress > min_clock_) {
    return false;
  }
  return std::count_if(progresses_.begin(), progresses_.end(), [this](const std::pair<int, int> entry) {
    return entry.second == min_clock_;
  }) == 1;
}

bool ProgressTracker::CheckThreadValid(int tid) const {
  return progresses_.find(tid) != progresses_.end();
}

}  // namespace csci5570
