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

#include "SVM.hpp"


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

  std::map<std::string, std::string> host_pairs = engine.GetHostPairs();

  lib::Parser<lib::SVMSample, DataStore> parser;
  lib::DataLoader<lib::SVMSample, DataStore> data_loader;
  engine.Barrier();

  uint32_t divider = 0;
  data_loader.load(url, hdfs_namenode, master_host, worker_host, hdfs_namenode_port, master_port, n_features,
                   parser, &data_store, id, nodes.size(), host_pairs, divider);


  // 1.1 Create table
//  const auto kTableId = engine.CreateTable<double>(ModelType::SSP, StorageType::Map, 2);  // table 0
  const auto kTableId = engine.CreateTable<double>(ModelType::BSP, StorageType::Map);  // table 0

  // 1.2 Load data
  engine.Barrier();

  // 2. Start training task
  MLTask task;
  task.setDataRange(data_store.size(), divider);
  LOG(INFO) << "data size for node: " << data_store.size();
  if (node->id == 0)
    LOG(INFO) << "Job start running.";
  std::vector<WorkerAlloc> worker_alloc;
  for (auto node : nodes) {
    worker_alloc.push_back({node.id, 20});  // node_id, worker_num
  }


  task.SetWorkerAlloc(worker_alloc);
  task.SetTables({kTableId});     // Use table 0
  task.SetLambda([kTableId, &data_store](const Info& info) {
//    LOG(INFO) << info.DebugString();

      // the maximum data range used by current worker
      DataRange data_range = info.get_data_range();

      SVM<double> svm(data_store);

      KVClientTable<double> table = info.CreateKVClientTable<double>(kTableId);


      for (int i = 0; i < 10e2; ++i) {
        // parameters from server
        auto batch = svm.get_batch(500);

        std::vector<double> theta;
        std::vector<Key> keys;
        svm.get_keys(batch, keys);
        table.Get(keys, &theta);
        LOG(INFO) << "Batch Size\t" << batch.size();
        auto res =  svm.compute_gradients(batch, keys, theta, 0.1);
        auto correct_ratio = svm.correct_rate(batch, keys, theta);

        table.Add(keys, res);
        LOG(INFO) << "Correct Ratio\t" << correct_ratio;
        table.Clock();
      }
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