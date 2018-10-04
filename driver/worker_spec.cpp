#include "driver/worker_spec.hpp"
#include "glog/logging.h"

namespace csci5570 {

WorkerSpec::WorkerSpec(const std::vector<WorkerAlloc>& worker_alloc) {
  Init(worker_alloc);
}

bool WorkerSpec::HasLocalWorkers(uint32_t node_id) const {
  auto it = node_to_workers_.find(node_id);
  return it != node_to_workers_.end() && (it->second).size() > 0;
}

const std::vector<uint32_t>& WorkerSpec::GetLocalWorkers(uint32_t node_id) const {
  return node_to_workers_.find(node_id)->second;
}

const std::vector<uint32_t>& WorkerSpec::GetLocalThreads(uint32_t node_id) const {
  return node_to_threads_.find(node_id)->second;
}

std::map<uint32_t, std::vector<uint32_t>> WorkerSpec::GetNodeToWorkers() {
  return node_to_workers_;
}

std::vector<uint32_t> WorkerSpec::GetAllThreadIds() {
  return std::vector<uint32_t>(thread_ids_.begin(), thread_ids_.end());
}

void WorkerSpec::InsertWorkerIdThreadId(uint32_t worker_id, uint32_t thread_id) {
  worker_to_thread_[worker_id] = thread_id;
  thread_to_worker_[thread_id] = worker_id;
  uint32_t node_id = worker_to_node_[worker_id];
  node_to_threads_[node_id].push_back(thread_id);
  thread_ids_.insert(thread_id);
}

void WorkerSpec::Init(const std::vector<WorkerAlloc>& worker_alloc) {
  for (WorkerAlloc alloc : worker_alloc) {
    uint32_t node_id = alloc.node_id;
    node_to_workers_[node_id] = std::vector<uint32_t>();
    for (uint32_t worker_id = num_workers_; worker_id < num_workers_ + alloc.num_workers; worker_id++) {
      worker_to_node_[worker_id] = node_id;
      node_to_workers_[node_id].push_back(worker_id);
    }
    num_workers_ += alloc.num_workers;
  }
}

}  // namespace csci5570
