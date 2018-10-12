#pragma once

#include <cinttypes>
#include <vector>

#include "base/abstract_partition_manager.hpp"
#include "base/magic.hpp"
#include "base/third_party/sarray.h"

#include "glog/logging.h"

namespace csci5570 {

class RangePartitionManager : public AbstractPartitionManager {
  using Val = double;
  using Vals = third_party::SArray<Val>;
 public:
  RangePartitionManager(const std::vector<uint32_t>& server_thread_ids, const std::vector<third_party::Range>& ranges)
      : AbstractPartitionManager(server_thread_ids), ranges_(ranges) {}

  uint32_t GetServerIdForKey(Key key) const {
    uint32_t i = 0;
    while(i < ranges_.size()) {
      if (ranges_[i].begin() <= key && ranges_[i].end() > key) {
        break;
      }
      i++;
    }
    return server_thread_ids_[i];
  }

  void Slice(const Keys& keys, std::vector<std::pair<int, Keys>>* sliced) const override {
    std::map<int, Keys> partitions;
    for (auto key : keys) {
      uint32_t server_id = GetServerIdForKey(key);
      if (partitions.find(server_id) == partitions.end()) {
        partitions[server_id] = Keys({key});
      } else {
        partitions[server_id].push_back(key);
      }
    }
    for(auto& part : partitions) {
      sliced->push_back({part.first, part.second});
    }
  }

  void Slice(const KVPairs& kvs, std::vector<std::pair<int, KVPairs>>* sliced) const override {
    std::map<int, KVPairs> partitions;
    for(uint32_t i = 0; i < kvs.first.size(); i++) {
      Key key = kvs.first[i];
      Val val = kvs.second[i];
      uint32_t server_id = GetServerIdForKey(key);
      if (partitions.find(server_id) == partitions.end()) {
        partitions[server_id] = KVPairs(Keys({key}), Vals({val}));
      } else {
        partitions[server_id].first.push_back(key);
        partitions[server_id].second.push_back(val);
      }
    }
    for(auto& part : partitions) {
      sliced->push_back({part.first, part.second});
    }
  }

 private:
  std::vector<third_party::Range> ranges_;
};

}  // namespace csci5570
