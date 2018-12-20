#include <cmath>
#include <vector>
#include <iostream>
#include <random>
#include <thread>
#include <chrono>
#include <Eigen/Dense>
#include "gflags/gflags.h"
#include "glog/logging.h"

#include "driver/engine.hpp"
#include "lib/abstract_data_loader.hpp"
#include "lib/labeled_sample.hpp"
#include "lib/parser.hpp"
//#include "app/logitstic_regression.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/range/irange.hpp>
#include <fstream>
#include <lib/svm_loader.hpp>
//#include "lib/svm_sample.hpp"
#include "lib/mf_sample.hpp"
#include <utility>
#include <time.h>

using namespace csci5570;
using DataStore = std::vector<lib::MFSample>;

DEFINE_string(config_file, "", "The config file path");
DEFINE_string(my_id, "", "Local node id");
DEFINE_string(input, "", "The hdfs input url");

/*--------------my functions used in ml start-----------------*/
class MF{


public:
// 用之前要reserve一下vec 在调用构造函数之前就要定vec的长度，估计是490000+17770 左右这么多- -
    MF(std::vector<std::pair<int,std::vector<float>>> &vec, DataStore* data_store, DataRange range, float learning_rate, int latent_factor, float lambda, int k)
    : data_store_(data_store), range_(range), learning_rate_(learning_rate), latent_factor_(latent_factor), lambda_(lambda), k_(k) {

        ran_initialization();

        std::pair<int, std::vector<float>> vec_user;
        std::pair<int, std::vector<float>> vec_item;

        int user_id = 0;
        int item_id = 0;

        for (uint32_t i = range_.start; i < range_.end; i++) {

            lib::MFSample temp = data_store_->at(i);

            user_id = data_store_->at(i).x_.first + 17770;
            item_id = data_store_->at(i).x_.second;

            vec_item.first = item_id;
            vec_user.first = user_id;

            for ( int i = 0; i < latent_factor_; i++ )
            {

                vec_item.second.push_back(rand()%100/(double)101);
                vec_user.second.push_back(rand()%100/(double)101);
            }
            vec[item_id] = vec_item;
            vec[user_id] = vec_user;
        }

    }

    void ran_initialization()
    {
        srand(time(0));
    }

    int get_k()
    {
        return k_;
    }

    float predict_ratings(std::pair<int, std::vector<float>> &user, std::pair<int, std::vector<float>> &item)
    // vec 存所有的，add和push的时候可以先把非零元素删掉再弄- -
    {
        float pre_ratings = 0;
        for ( int i = 0; i < latent_factor_; i++ )
        {
            pre_ratings += user.second[i] * item.second[i];
        }
        return pre_ratings;
    }

    int get_latent_factor()
    {
        return this->latent_factor_;
    }

    float get_learning_rate()
    {
        return this->learning_rate_;
    }

    float get_lambda()
    {
        return this->lambda_;
    }

    void unzip_para( std::vector<float> &input, std::pair<int, std::vector<float>> &output, int id)
    // 把拉平的index换回来 - -
    {
        output.first = id;
        output.second = input;
    }

private:
    DataStore* data_store_;
    int latent_factor_;
    DataRange range_;
    float lambda_;
    float learning_rate_;
    int k_; // 拉平系数- -

};


/*--------------my functions used in ml end-----------------*/

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
    using DataStore = std::vector<lib::MFSample>;
    DataStore* data_store;
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

    /*
     * Begin IO config
     */

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

    lib::Parser<lib::MFSample, DataStore> parser; // parser should be override --updated in lib/MFSample
    lib::DataLoader<lib::MFSample, DataStore> data_loader;
    data_loader.load(url, hdfs_namenode, master_host, worker_host, hdfs_namenode_port, master_port, n_features,
                     parser, &data_store, id, nodes.size(), help_host);

    data_loader.load(url, hdfs_namenode, master_host, worker_host, hdfs_namenode_port, master_port, n_features,
                     parser, &data_store_backup, id, nodes.size(), help_host, true);

    LOG(INFO) << "Store Size\t" << data_store.size() << "\tBackup Size\t" <<data_store_backup.size();


    // 1. Start system
    engine.StartEverything();

    // 1.1 Create table
    const auto kTableId = engine.CreateTable<double>(ModelType::BSP, StorageType::Map);  // table 0

    // 1.2 Load data
    engine.Barrier();

    // 2. Start training task
    MLTask task;

    task.setDataRange(data_store.size(), data_store.size());
    LOG(INFO) << "data size for node: " << data_store.size();
    std::vector<WorkerAlloc> worker_alloc;
    for (auto node : nodes) {
        worker_alloc.push_back({node.id, 20});  // node_id, worker_num
    }
    task.SetWorkerAlloc(worker_alloc);
    task.SetTables({kTableId});     // Use table 0
    task.SetLambda([kTableId, &data_store](const Info& info)

    {
        DataRange data_range = info.get_data_range();

//        480189
        int num = 497961; // 480189 + 17770 + 2reserved
        std::vector<std::pair<int, std::vector<float>>> all;
        all.reserve(num);
        MF mf( &all, &data_store, data_range, 0.01, 10, 0.1, 2 );

        KVClientTable<float> table = info.CreateKVClientTable<float>(kTableId);

        int max_epoch = 10000;

        int user_id = 0;
        int item_id = 0;
        float ratings = 0;
        float pre_ratings = 0;
        float error = 0;
        double RMSE = 0;

        std::pair<int, std::vector<float>> user;
        std::pair<int, std::vector<float>> item;
        std::vector<int> Get_keys;

        std::vector<int> upadate_keys;
        std::vector<float> update_values;
        float value = 0;
        int k = mf.get_k();
        float learning_rate = mf.get_learning_rate();
        float lambda = mf.get_lambda();


        for ( int i = 0; i < max_epoch; i++ )
        {

            while( true )
            {
                int next_index = info.next_sample();

                if ( next_index == -1 )
                    break;

                user_id = data_store->at(next_index).x_.first;
                item_id = data_store->at(next_index).x_.second;
                ratings = data_store->at(next_index).y_;
                user.first = user_id;
                item.first = item_id;

                if ( i == 0 )
                {
                    for( int j = 0; j < mf.get_latent_factor(); j++ )
                    {
                        upadate_keys.push_back(user.first * k + j);
                        value = learning_rate * ( error * item.second[j] - lambda * user.second[j]); // update value for user
                        update_values.push_back(value);
                    }
                    table.Add(upadate_keys,update_values);
                    upadate_keys.clear();
                    update_values.clear();

                    for( int j = 0; j < mf.get_latent_factor(); j++ )
                    {
                        upadate_keys.push_back(item.first * k + j);
                        value = learning_rate * ( error * user.second[j] - lambda * item.second[j]); // update value for item
                        update_values.push_back(value);
                    }
                    table.Add(upadate_keys,update_values);
                    upadate_keys.clear();
                    update_values.clear();


                    all.clear();
                }


                for( int j = 0; j < mf.get_latent_factor(); j++ )
                {
                    Get_keys.push_back(user_id * k + j);
//                    value = learning_rate * ( error * item.second[j] - lamda * user.second[j]); // get value for user
//                    update_values.push_back(value);
                }
                table.Get(Get_keys,&(user.second));//
                Get_keys.clear();

                for( int j = 0; j < mf.get_latent_factor(); j++ )
                {
                    Get_keys.push_back(item_id * k + j);
//                    value = learning_rate * ( error * user.second[j] - lamda * item.second[j]); // update value for user
//                    update_values.push_back(value);
                }
                table.Get(Get_keys,&(item.second));
                Get_keys.clear();

//
//                user = all[user_id];
//                item = all[item_id];
//



                pre_ratings = mf.predict_ratings(&user, &item);
                error = ratings - pre_ratings;
                RMSE += error * error;

                for( int j = 0; j < mf.get_latent_factor(); j++ )
                {
                    upadate_keys.push_back(user.first * k + j);
                    value = learning_rate * ( error * item.second[j] - lamda * user.second[j]); // update value for user
                    update_values.push_back(value);
                }
                table.Add(upadate_keys,update_values);
                upadate_keys.clear();
                update_values.clear();

                for( int j = 0; j < mf.get_latent_factor(); j++ )
                {
                    upadate_keys.push_back(item.first * k + j);
                    value = learning_rate * ( error * user.second[j] - lamda * item.second[j]); // update value for user
                    update_values.push_back(value);
                }
                table.Add(upadate_keys,update_values);
                upadate_keys.clear();
                update_values.clear();

            }

            RMSE = RMSE / data_range.length;
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
