#pragma once

#include <cinttypes>
#include <vector>

#include "base/abstract_partition_manager.hpp"
#include "base/magic.hpp"
#include "base/third_party/sarray.h"

#include "glog/logging.h"

namespace csci5570 {

class HashPartitionManager : public AbstractPartitionManager {
 public:
  HashPartitionManager(const std::vector<uint32_t>& server_thread_ids) : AbstractPartitionManager(server_thread_ids) {}

  size_t GetNumServers() {
      return server_thread_ids_.size();
  }

  void Slice(const Keys& keys, std::vector<std::pair<int, Keys>>* sliced) const override {
    uint32_t server_id_num = server_thread_ids_.size();
    std::unordered_map<uint32_t, Keys> server2keys;
    for (uint32_t key : keys) {
        uint32_t server_id = server_thread_ids_[key % server_id_num];

        if (server2keys.find(server_id) != server2keys.end()) {
            server2keys[server_id].push_back(key);
        } else {
            server2keys[server_id] = Keys({key});
        }
    }
    for (auto& kv : server2keys) {
        sliced->push_back(std::make_pair(kv.first, kv.second));
    }
  }

  void Slice(const KVPairs& kvs, std::vector<std::pair<int, KVPairs>>* sliced) const override {
      uint32_t server_id_num = server_thread_ids_.size();
      std::unordered_map<uint32_t, KVPairs> server2kvs;
      for (uint32_t i = 0; i < kvs.first.size(); i++) {
          uint32_t key = kvs.first[i];
          double val = kvs.second[i];
          uint32_t server_id = server_thread_ids_[key % server_id_num];

          if (server2kvs.find(server_id) != server2kvs.end()) {
              server2kvs[server_id].first.push_back(key);
              server2kvs[server_id].second.push_back(val);
          } else {
              server2kvs[server_id] = KVPairs(
                      third_party::SArray<Key>({key}),
                      third_party::SArray<double>({val}));
          }
      }
      for (auto& kv : server2kvs) {
          sliced->push_back(std::make_pair(kv.first, kv.second));
      }
  }
};

}  // namespace csci5570
