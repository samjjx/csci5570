//
// Created by samjjx on 2018/12/17.
//

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

#include <boost/algorithm/string.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/range/irange.hpp>
#include <fstream>
#include <lib/svm_loader.hpp>


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

std::vector<double> compute_gradients(const std::vector<lib::SVMSample>& samples, const std::vector<Key>& keys,
                                      const std::vector<double>& vals, double alpha) {
  std::vector<double> deltas(keys.size(), 0.);
  for (auto sample : samples) {
    auto& x = sample.x_;
    double y = sample.y_;
    double predict = 0.;
    int idx = 0;
    for (auto& field : x) {
      while (keys[idx] < field.first)
        ++idx;
      predict += vals[idx] * field.second;
    }
    predict += vals.back();
    int predictLabel;
    if (predict >= 0) {
      predictLabel = 1;
    } else {
      predictLabel = -1;
    }

    idx = 0;
    for (auto& field : x) {
      while (keys[idx] < field.first)
        ++idx;
      deltas[idx] += alpha * field.second * (y - predictLabel);
    }
    deltas[deltas.size() - 1] += alpha * (y - predictLabel);
  }
  return deltas;
}

double correct_rate(const std::vector<lib::SVMSample>& samples, const std::vector<Key>& keys,
                    const std::vector<double>& vals) {
  int total = samples.size();
  double n = 0;
  for (auto sample : samples) {
    auto& x = sample.x_;
    double y = sample.y_;
    double predict = 0.;
    int idx = 0;
    for (auto& field : x) {
      while (keys[idx] < field.first)
        ++idx;
      predict += vals[idx] * field.second;
    }
    predict += vals.back();
    int predict_;
    if (predict >= 0) {
      predict_ = 1;
    } else {
      predict_ = -1;
    }
    if (predict_ == y) {
      n++;
    }
  }
  double result = n / total;
  return result;
}

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  FLAGS_stderrthreshold = 0;
  FLAGS_colorlogtostderr = true;
  using DataStore = std::vector<lib::SVMSample>;
  DataStore data_store;
  DataStore data_store_backup;

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
  std::string help_host = nodes[engine.GetHelpeeNode()].hostname;


  // 1. Start system
  engine.StartEverything();

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

  lib::Parser<lib::SVMSample, DataStore> parser;
  lib::DataLoader<lib::SVMSample, DataStore> data_loader;
  engine.Barrier();

  data_loader.load(url, hdfs_namenode, master_host, worker_host, hdfs_namenode_port, master_port, n_features,
                   parser, &data_store, id, nodes.size(), help_host, false);


  // 1.1 Create table
//  const auto kTableId = engine.CreateTable<double>(ModelType::SSP, StorageType::Map, 2);  // table 0
  const auto kTableId = engine.CreateTable<double>(ModelType::BSP, StorageType::Map);  // table 0

  // 1.2 Load data
  engine.Barrier();

  // 2. Start training task
  MLTask task;

  task.setDataRange(data_store.size(), data_store.size()/2);
  std::vector<WorkerAlloc> worker_alloc;
  for (auto node : nodes) {
    worker_alloc.push_back({node.id, 2});  // node_id, worker_num
  }
  task.SetWorkerAlloc(worker_alloc);
  task.SetTables({kTableId});     // Use table 0

  task.SetLambda([kTableId, &data_store](const Info& info) {
//    LOG(INFO) << info.DebugString();

      // the maximum data range used by current worker
      DataRange data_range = info.get_data_range();
      std::vector<Key> keys;

      for(auto s: data_store){


      }
      std::vector<lib::SVMSample> data_sample;

      std::map<Key, double> theta_;
      std::map<Key, double> grad_;

      KVClientTable<double> table = info.CreateKVClientTable<double>(kTableId);

      // algorithm helper
  });
  auto start_time = std::chrono::system_clock::now();
  engine.Run(task);
  auto end_time = std::chrono::system_clock::now();
  LOG(INFO) << "Task completed in " << std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();


  // 3. Stop
  engine.StopEverything();

  return 0;
}