#pragma once

#include <functional>
#include <string>

#include "boost/utility/string_ref.hpp"

#include "parser.hpp"

#include "boost/utility/string_ref.hpp"
#include "glog/logging.h"

#include "base/serialization.hpp"
#include "io/coordinator.hpp"
#include "io/hdfs_assigner.hpp"
#include "io/hdfs_file_splitter.hpp"
#include "io/line_input_format.hpp"

namespace csci5570 {
namespace lib {

// template <typename Sample, template <typename> typename DataStore<Sample>>
template <typename Sample, typename DataStore>
class AbstractDataLoader {
 public:
  /**
   * Load samples from the url into datastore
   *
   * @param url          input file/directory
   * @param n_features   the number of features in the dataset
   * @param parse        a parsing function
   * @param datastore    a container for the samples / external in-memory storage abstraction
   */
  template <typename Parse>  // e.g. std::function<Sample(boost::string_ref, int)>
  static void load(std::string url, int n_features, Parse parse, DataStore* datastore, uint32_t id) {
    // 1. Connect to the data source, e.g. HDFS, via the modules in io
    // 2. Extract and parse lines
    // 3. Put samples into datastore

    std::string hdfs_namenode = "proj10";
    int hdfs_namenode_port = 9000;
    int master_port = 19817;  // use a random port number to avoid collision with other users
    zmq::context_t zmq_context(1);
    int proc_id = getpid();  // the actual process id, or you can assign a virtual one, as long as it is distinct
    std::string master_host = "proj10";  // change to the node you are actually using
    std::string worker_host = "proj10";  // change to the node you are actually using

    Coordinator coordinator(proc_id, worker_host, &zmq_context, master_host, master_port);
    coordinator.serve();
    int num_threads = 1;
    int second_id = 0;
    LineInputFormat infmt(url, num_threads, second_id, &coordinator, worker_host, hdfs_namenode,
                          hdfs_namenode_port);

    boost::string_ref record;
    bool success = true;
    int count = 0;
    while (true) {
      success = infmt.next(record);
      if (success == false)
        break;
      Sample s = parse.parse_libsvm(record, n_features);
      ++count;
      if (count > n_features)
        break;
    }
  }
};  // class AbstractDataLoader

}  // namespace lib
}  // namespace csci5570
