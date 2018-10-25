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

#include <boost/algorithm/string.hpp>
#include <fstream>

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

  LOG(INFO) << FLAGS_config_file;
  LOG(INFO) << FLAGS_my_id;

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
  task.SetLambda([kTableId](const Info& info) {
    LOG(INFO) << info.DebugString();

    KVClientTable<double> table = info.CreateKVClientTable<double>(kTableId);

    for (int i = 0; i < 1e2; ++i) {
      std::vector<Key> keys{1};

      std::vector<double> ret;
      table.Get(keys, &ret);
      LOG(INFO) << ret[0];

      std::vector<double> vals{0.5};
      table.Add(keys, vals);

      table.Clock();
    }
    LOG(INFO) << "Task completed.";
  });

  engine.Run(task);

  // 3. Stop
  engine.StopEverything();
  return 0;
}