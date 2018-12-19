#include <cmath>

#include <iostream>
#include <random>
#include <thread>
#include <chrono>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "driver/engine.hpp"
#include "lib/abstract_data_loader.hpp"
#include "lib/labeled_sample.hpp"
#include "lib/parser.hpp"
#include "app/logitstic_regression.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/range/irange.hpp>
#include <fstream>
#include <lib/svm_loader.hpp>
//#include "lib/svm_sample.hpp"

using namespace csci5570;

DEFINE_string(config_file, "", "The config file path");
DEFINE_string(my_id, "", "Local node id");
DEFINE_string(input, "", "The hdfs input url");

void get_nodes_from_config(std::string config_file, std::vector<Node>& nodes) {
  std::ifstream infile(config_file);
  std::string line;
  while (std::getline(infile, line)) {
    std::vector<std::string> cols;
    boost::split(cols, line, [](char c){return c == ':';});
    uint32_t node_id = std::stoi(cols[0]);
    std::string host_name = cols[1];
    boost::trim(host_name);
    int port = std::stoi(cols[2]);
    nodes.push_back(Node({node_id, host_name, port}));
  }
}

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  FLAGS_stderrthreshold = 0;
  FLAGS_colorlogtostderr = true;
  using DataStore = std::vector<lib::SVMSample>;
  DataStore data_store;

  LOG(INFO) << FLAGS_config_file;
  LOG(INFO) << FLAGS_my_id;
  LOG(INFO) << FLAGS_input;

  std::vector<Node> nodes;
  get_nodes_from_config(FLAGS_config_file, nodes);
  uint32_t my_id = std::stoi(FLAGS_my_id);
  auto node = std::find_if(nodes.begin(), nodes.end(), [my_id](Node& n){return n.id == my_id;});
  if (node == nodes.end()) {
    LOG(INFO) << "My_id not in nodes list.";
    return -1;
  }

  Engine engine(*node, nodes);

  /*
   * Begin IO config
   */
//  std::string url = "hdfs:///datasets/classification/a9";
  std::string url = FLAGS_input;
  std::string hdfs_namenode = "proj10";                        // Do not change
  std::string master_host = "proj10";  // Set to worker name
  std::string worker_host = node->hostname;  // Set to worker name
  uint32_t id = node->id;
  int hdfs_namenode_port = 9000;
  int master_port = 19817;  // use a random port number to avoid collision with other users
  const uint32_t n_features = 100;

  /*
   * End IO config
   */
  std::map<std::string, std::string> host_pairs = engine.GetHostPairs();
  lib::Parser<lib::SVMSample, DataStore> parser;
  lib::DataLoader<lib::SVMSample, DataStore> data_loader;
  data_loader.load(url, hdfs_namenode, master_host, worker_host, hdfs_namenode_port, master_port, n_features,
                                parser, &data_store, id, nodes.size(), host_pairs);

  /*
  // for test
  for (int i = 0; i < 10e4; i++) {
    lib::SVMSample sample;
    sample.x_ = std::vector<std::pair<int, int>>({{0, 2}, {3, 1}});
    sample.y_ = -1;
    data_store.push_back(sample);
    lib::SVMSample sample1;
    sample1.x_ = std::vector<std::pair<int, int>>({{2, 2}, {3, 1}});
    sample1.y_ = 1;
    data_store.push_back(sample1);
  }
   */

  // 1. Start system
  engine.StartEverything();

  // 1.1 Create table
//  const auto kTableId = engine.CreateTable<double>(ModelType::SSP, StorageType::Map, 2);  // table 0
  const auto kTableId = engine.CreateTable<double>(ModelType::BSP, StorageType::Map);  // table 0

  // 1.2 Load data
  engine.Barrier();

  // 2. Start training task
  MLTask task;
  // TODO
  task.setDataRange(data_store.size(), data_store.size());
  LOG(INFO) << "data size for node: " << data_store.size();
  std::vector<WorkerAlloc> worker_alloc;
  for (auto node : nodes) {
    worker_alloc.push_back({node.id, 20});  // node_id, worker_num
  }
  task.SetWorkerAlloc(worker_alloc);
  task.SetTables({kTableId});     // Use table 0
  task.SetLambda([kTableId, &data_store](const Info& info) {
//    LOG(INFO) << info.DebugString();
    uint32_t MAX_EPOCH = 2;
    // the maximum data range used by current worker
    DataRange data_range = info.get_data_range();
    // algorithm helper
    LogisticRegression<double> lr(&data_store, data_range, 1);
    // get all parameters keys of this data range
    std::vector<Key> keys;
    lr.get_keys(keys);

    LOG(INFO) << "Thread " << info.thread_id << ", data size: " << data_range.length << ", parameter size: " << keys.size();

    KVClientTable<double> table = info.CreateKVClientTable<double>(kTableId);

    // EPOCH
    for (int i = 0; i < MAX_EPOCH; ++i) {
      // update parameters
      std::vector<double> theta;
      table.Get(keys, &theta);
      lr.update_theta(keys, theta);
      if(info.thread_id == 100) {
        LOG(INFO) << "Current accuracy: " << lr.test_acc();
        LOG(INFO) << "Current loss: " << lr.get_loss();
      }

//      std::vector<double> grad(keys.size(), 0.0);
      std::unordered_map<Key, double> grad;

      // get one training data
      uint32_t sample_num = 0;
      while(true) {
        auto start_time = std::chrono::system_clock::now();
        int next_idx = info.next_sample();
        if (next_idx == -1) {break;}
        // add gradient to each dimension
        lr.compute_gradient(next_idx, grad);
        auto end_time = std::chrono::system_clock::now();
        //LOG(INFO) << "One sample takes " << std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        sample_num += 1;
        if (info.thread_id == 1100 && sample_num % 10000 == 0) {LOG(INFO) << "Thread " << info.thread_id << ", progress: " << float(sample_num) / data_range.length;}
        // simulate straggler
        if (info.thread_id == 1100) {
          //std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      }
      // average gradient and update to server
      if (sample_num > 0) {
        std::vector<double> grad_vec;
        for (Key k :keys) {
          auto it = grad.find(k);
          if (it != grad.end()) {
            grad_vec.push_back(it->second/sample_num);
            //grad_vec.push_back(it->second);
          } else {grad_vec.push_back(0.0);}
        }
        table.Add(keys, grad_vec);
      }
      table.Clock();
    }
  });
  auto start_time = std::chrono::system_clock::now();
  engine.Run(task);
  auto end_time = std::chrono::system_clock::now();
  LOG(INFO) << "Task completed in " << std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

  // 3. Stop
  engine.StopEverything();
  return 0;
}
