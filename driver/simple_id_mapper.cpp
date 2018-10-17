#include "driver/simple_id_mapper.hpp"

#include <cinttypes>
#include <vector>

#include "base/node.hpp"

namespace csci5570 {

SimpleIdMapper::SimpleIdMapper(Node node, const std::vector<Node>& nodes) {
  node_ = node;
  nodes_ = nodes;
}

uint32_t SimpleIdMapper::GetNodeIdForThread(uint32_t tid) {
  return tid / kMaxThreadsPerNode;
}

void SimpleIdMapper::Init(int num_server_threads_per_node) {
  if (num_server_threads_per_node < 1 || num_server_threads_per_node > kWorkerHelperThreadId) {
    throw std::invalid_argument("num_server_threads_per_node should be in [1, kWorkerHelperThreadId]");
  }
  for (auto node : nodes_) {
    uint32_t node_id = node.id;
    uint32_t id_base = kMaxThreadsPerNode * node_id;
    // server_threads
    std::vector<uint32_t> server_thread_ids;
    for (uint32_t sid = id_base; sid < id_base + num_server_threads_per_node; sid++) {
      server_thread_ids.push_back(sid);
    }
    node2server_[node_id] = server_thread_ids;
    // worker_threads
    std::vector<uint32_t> worker_thread_ids;
    for (uint32_t wid = id_base + kWorkerHelperThreadId; wid < id_base + kMaxBgThreadsPerNode; wid++) {
      worker_thread_ids.push_back(wid);
    }
    node2worker_helper_[node_id] = worker_thread_ids;
    // user_threads
    std::set<uint32_t> user_thread_ids;
    node2worker_[node_id] = user_thread_ids;
  }
}

int SimpleIdMapper::AllocateWorkerThread(uint32_t node_id) {
  if (node2worker_[node_id].size() >= kMaxThreadsPerNode - kMaxBgThreadsPerNode) {
    return -1;
  }
  int min_id = kMaxThreadsPerNode * node_id + kMaxBgThreadsPerNode;
  int max_id = kMaxThreadsPerNode * (node_id + 1);
  for (int tid = min_id; tid < max_id; tid++) {
    if (node2worker_[node_id].find(tid) == node2worker_[node_id].end()) {
      int alloc_helper = AllocateHelperForWorker(node_id, tid);
      if (alloc_helper == -1) {
        return -1;
      }
      node2worker_[node_id].insert(tid);
      return tid;
    }
  }
  return -1;
}

int SimpleIdMapper::AllocateHelperForWorker(uint32_t node_id, uint32_t worker_thread_id) {
  std::set<uint32_t> occupied_helpers;
  for(auto pair : worker2helper_) {
    occupied_helpers.insert(pair.second);
  }
  std::vector<uint32_t> all_helpers = node2worker_helper_[node_id];
  for(auto helper_id : all_helpers) {
    if (occupied_helpers.find(helper_id) == occupied_helpers.end()) {
      worker2helper_[worker_thread_id] = helper_id;
      return helper_id;
    }
  }
}

int SimpleIdMapper::GetHelperForWorker(uint32_t worker_thread_id) {
  if (worker2helper_.find(worker_thread_id) == worker2helper_.end()) {
    return -1;
  }
  return worker2helper_[worker_thread_id];
}

void SimpleIdMapper::DeallocateWorkerThread(uint32_t node_id, uint32_t tid) {
  if (node2worker_[node_id].find(tid) == node2worker_[node_id].end()) {
    return;
  }
  node2worker_[node_id].erase(tid);
  worker2helper_.erase(tid);
}

std::vector<uint32_t> SimpleIdMapper::GetServerThreadsForId(uint32_t node_id) {
  return node2server_[node_id];
}

std::vector<uint32_t> SimpleIdMapper::GetWorkerHelperThreadsForId(uint32_t node_id) {
  return node2worker_helper_[node_id];
}

std::vector<uint32_t> SimpleIdMapper::GetWorkerThreadsForId(uint32_t node_id) {
  std::vector<uint32_t> worker_threads;
  for (auto tid : node2worker_[node_id]) {
    worker_threads.push_back(tid);
  }
  return worker_threads;
}

std::vector<uint32_t> SimpleIdMapper::GetAllServerThreads() {
  std::vector<uint32_t> all_threads;
  for (auto& pair : node2server_) {
    all_threads.insert(all_threads.end(), pair.second.begin(), pair.second.end());
  }
  return all_threads;
}

const uint32_t SimpleIdMapper::kMaxNodeId;
const uint32_t SimpleIdMapper::kMaxThreadsPerNode;
const uint32_t SimpleIdMapper::kMaxBgThreadsPerNode;
const uint32_t SimpleIdMapper::kWorkerHelperThreadId;

}  // namespace csci5570
