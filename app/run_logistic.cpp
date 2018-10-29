#include <cmath>

#include <iostream>
#include <random>
#include <thread>

#include "gflags/gflags.h"
#include "glog/logging.h"

#include "driver/engine.hpp"
//#include "lib/abstract_data_loader.hpp"
//#include "lib/labeled_sample.hpp"
//#include "lib/parser.hpp"
#include "app/logitstic_regression.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/range/irange.hpp>
#include <fstream>
#include <lib/svm_loader.hpp>

using namespace csci5570;

using Sample = double;
using DataStore = std::vector<Sample>;

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

  /*
   * Begin IO config
   */
//  std::string url = "hdfs:///datasets/classification/a9";
  std::string url = FLAGS_input;
  std::string hdfs_namenode = "proj10";                        // Do not change
  std::string master_host = "proj10";  // Set to worker name
  std::string worker_host = "proj10";  // Set to worker name
  int hdfs_namenode_port = 9000;
  int master_port = 19817;  // use a random port number to avoid collision with other users
  const uint32_t n_features = 100;
  /*
   * End IO config
   */


  lib::Parser<lib::SVMSample, DataStore> parser;
  lib::DataLoader<lib::SVMSample, DataStore> data_loader;
  data_loader.load(url, hdfs_namenode, master_host, worker_host, hdfs_namenode_port, master_port, n_features,
                                parser, &data_store);

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

  // 1.1 Create table
  const auto kTableId = engine.CreateTable<double>(ModelType::ASP, StorageType::Map);  // table 0

  // 1.2 Load data
  engine.Barrier();

  // 2. Start training task
  MLTask task;
  task.SetWorkerAlloc({{0, 3}, {1, 2}, {2, 1}});  // node_id, worker_num
  task.SetTables({kTableId});     // Use table 0
  task.SetLambda([kTableId, &data_store, n_features](const Info& info) {
    LOG(INFO) << info.DebugString();
    // algorithm helper
    const float learning_rate = 0.1;
    LogisticRegression<double, n_features> lr(learning_rate);
    // key for parameters
    std::vector<Key> keys;
    // init data
    const uint32_t SAMPLE_NUM = data_store.size();
    Matrix<double, Dynamic, n_features> x(SAMPLE_NUM, n_features);
    Matrix<double, Dynamic, 1> y(SAMPLE_NUM, 1);
    for(uint32_t i = 0; i < SAMPLE_NUM; i++) {
      for(uint32_t j = 0; j < n_features; j++) {
        x(i, j) = static_cast<double>(data_store[i].x_[j].second);
        if (keys.size() <= n_features) {
          keys.push_back(static_cast<Key>(data_store[i].x_[j].first));
        }
      }
      y(i, 0) = static_cast<double>(data_store[i].y_);
    }
    lr.set_data(&x, &y);
    Matrix<double, n_features, 1> grad;
    Matrix<double, n_features, 1> theta;

    KVClientTable<double> table = info.CreateKVClientTable<double>(kTableId);

    for (int i = 0; i < 10; ++i) {
      // parameters from server
      std::vector<double> ret;
      table.Get(keys, &ret);
      for (uint32_t j = 0; j < n_features; j++) {theta(j, 0) = ret[j];}
      lr.update_theta(theta);
      lr.compute_gradient(grad);
      std::vector<double> vals;
      for(uint32_t j = 0; j < n_features; j++) {vals.push_back(grad(j, 0));}
      table.Add(keys, vals);
      table.Clock();
    }
//    LOG(INFO) << theta;
    LOG(INFO) << "Task completed.";
  });

  engine.Run(task);

  // 3. Stop
  engine.StopEverything();
  return 0;
}
