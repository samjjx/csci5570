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

  /*
   * Begin IO config
   */
//  std::string url = "hdfs:///datasets/classification/a9";
  std::string url = FLAGS_input;
  std::string hdfs_namenode = "proj10";                        // Do not change
  std::string master_host = node->hostname;  // Set to worker name
  std::string worker_host = node->hostname;  // Set to worker name
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

//  lib::SVMSample sample;
//  sample.x_ = std::vector<std::pair<int, int>>({{0, 2}, {3, 1}});
//  sample.y_ = -1;
//  data_store.push_back(sample);
//  lib::SVMSample sample1;
//  sample1.x_ = std::vector<std::pair<int, int>>({{2, 2}, {3, 1}});
//  sample1.y_ = 1;
//  data_store.push_back(sample1);

  Engine engine(*node, nodes);

  // 1. Start system
  engine.StartEverything();

  // 1.1 Create table
  const auto kTableId = engine.CreateTable<double>(ModelType::ASP, StorageType::Map);  // table 0

  // 1.2 Load data
  engine.Barrier();

  // 2. Start training task
  MLTask task;
  task.setDataSize(data_store.size());
  std::vector<WorkerAlloc> worker_alloc;
  for (auto node : nodes) {
    worker_alloc.push_back({node.id, 1});  // node_id, worker_num
  }
  task.SetWorkerAlloc(worker_alloc);
  task.SetTables({kTableId});     // Use table 0
  task.SetLambda([kTableId, &data_store](const Info& info) {
    LOG(INFO) << info.DebugString();
    uint32_t MAX_EPOCH = 10e2;
    // the maximum data range used by current worker
    DataRange data_range = info.get_data_range();
    // algorithm helper
    LogisticRegression<double> lr(&data_store, data_range, 0.00001);
    // get all parameters keys of this data range
    std::vector<Key> keys;
    lr.get_keys(keys);
    LOG(INFO) << "parameter size: " << keys.size();

    KVClientTable<double> table = info.CreateKVClientTable<double>(kTableId);

    // EPOCH
    for (int i = 0; i < MAX_EPOCH; ++i) {
      // update parameters
      std::vector<double> theta;
      table.Get(keys, &theta);
      lr.update_theta(keys, theta);
      if(i % 5 == 0) {
        LOG(INFO) << "Current accuracy: " << lr.test_acc();
        LOG(INFO) << "Current loss: " << lr.get_loss();
      }

      lib::SVMSample sample;
      std::vector<double> grad;

      // get one training data
      uint32_t sample_num = 0;
      while(true) {
        uint32_t next_idx = info.next_sample();
        if (next_idx == -1) {break;}
        // add gradient to each dimension
        lr.compute_gradient(next_idx, grad);
        sample_num += 1;
      }
      // average gradient and update to server
      if (sample_num > 0) {
        for (auto j = 0; j < grad.size(); j++) {
          grad[i] /= sample_num;
        }
        table.Add(keys, grad);
      }

      table.Clock();
    }
    // print theta
    /*
    std::vector<double> theta;
    table.Get(keys, &theta);
    std::stringstream ss;
    for(auto t : theta) {
      ss << t;
      ss << " ";
    }
    LOG(INFO) << ss.str();
    */
    LOG(INFO) << "Task completed.";
  });

  engine.Run(task);

  // 3. Stop
  engine.StopEverything();
  return 0;
}
