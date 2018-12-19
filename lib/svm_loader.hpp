//
// Created by samjjx on 2018/10/28.
//

#ifndef CSCI5570_SVM_LOADER_HPP
#define CSCI5570_SVM_LOADER_HPP

#include <thread>
#include <vector>
#include "base/serialization.hpp"
#include "boost/utility/string_ref.hpp"
#include "io/coordinator.hpp"
#include "io/hdfs_assigner.hpp"
#include "io/hdfs_file_splitter.hpp"
#include "io/line_input_format.hpp"
#include "lib/abstract_data_loader.hpp"
#include "lib/labeled_sample.hpp"

#include "glog/logging.h"
#include "lib/parser.hpp"

namespace csci5570 {
    namespace lib {

        template<typename Sample, typename DataStore>
        class DataLoader : public AbstractDataLoader<Sample, DataStore> {
        public:
            template <typename Parse>  // e.g. std::function<Sample(boost::string_ref, int)>
            static void load(std::string url, std::string hdfs_namenode, std::string master_host, std::string worker_host,
                             int hdfs_namenode_port, int master_port, int n_features, Parse parse, DataStore* datastore, uint32_t id, int total_nodes,
                             uint32_t& divide_idx) {
              // 1. Connect to the data source, e.g. HDFS, via the modules in io
              // 2. Extract and parse lines
              // 3. Put samples into datastore
              LOG(INFO) << "URL:" << url;
//              std::string hdfs_namenode = "proj10";
//              int hdfs_namenode_port = 9000;
//              int master_port = 19817;  // use a random port number to avoid collision with other users
              zmq::context_t zmq_context(1);
//              std::string master_host = "proj10";  // change to the node you are actually using
//              std::string worker_host = "proj10";  // change to the node you are actually using

              std::thread master_thread;
              // 1. Spawn the HDFS block assigner thread on the master

              // 2. Prepare meta info for the master and workers
              int proc_id = getpid();  // the actual process id, or you can assign a virtual one, as long as it is distinct

              Coordinator coordinator(proc_id, worker_host, &zmq_context, master_host, master_port);
              coordinator.serve();
              LOG(INFO) << "Coordinator begins serving";

              if(worker_host == master_host) {
                master_thread = std::thread([&zmq_context, master_port, hdfs_namenode_port, hdfs_namenode, total_nodes] {
                    HDFSBlockAssigner hdfs_block_assigner(hdfs_namenode, hdfs_namenode_port, &zmq_context, master_port, total_nodes);
                    hdfs_block_assigner.Serve();
                });
              }

              std::thread worker_thread([url, hdfs_namenode_port, hdfs_namenode, &coordinator, worker_host, parse, &datastore,n_features, id, &divide_idx] {
                  int num_threads = 1;
                  int second_id = 1;

                  LOG(INFO) << "Line input start to prepare";
                  LineInputFormat infmt(url, num_threads, second_id, &coordinator, worker_host, hdfs_namenode,
                                        hdfs_namenode_port);
                  LOG(INFO) << "Line input is well prepared";

                  boost::string_ref record;
                  // divider
                  uint32_t count = 0;
                  while (infmt.next(record)) {
                    Sample s = parse.parse_libsvm(record, n_features);
                    datastore->push_back(s);
                    ++count;
                  }
                  divide_idx = count;
                  // Notify master that the worker finished loading its data
                  BinStream finish_signal;
                  finish_signal << worker_host << second_id;
                  coordinator.notify_master(finish_signal, 302);

                  // start loading backup data
                  while (infmt.next(record)) {
                    Sample s = parse.parse_libsvm(record, n_features);
                    datastore->push_back(s);
                  }
                  // Notify master that the worker wants to exit
                  BinStream exit_signal;
                  exit_signal << worker_host << second_id;
                  coordinator.notify_master(exit_signal, 300);
              });
              if(worker_host == master_host)
                master_thread.join();

              worker_thread.join();
            }
        };  // Class DataLoader
    }  // namespace lib
}  // namespace csci5570

#endif //CSCI5570_SVM_LOADER_HPP
