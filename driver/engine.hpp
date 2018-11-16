#pragma once

#include <vector>
#include <algorithm>
#include <math.h>

#include "base/abstract_partition_manager.hpp"
#include "base/range_partition_manager.hpp"
#include "base/hash_partition_manager.hpp"
#include "server/abstract_model.hpp"
#include "server/consistency/ssp_model.hpp"
#include "server/consistency/asp_model.hpp"
#include "server/consistency/bsp_model.hpp"
#include "server/abstract_storage.hpp"
#include "server/map_storage.hpp"
#include "base/node.hpp"
#include "comm/mailbox.hpp"
#include "comm/sender.hpp"
#include "driver/ml_task.hpp"
#include "driver/simple_id_mapper.hpp"
#include "driver/worker_spec.hpp"
#include "driver/work_assigner.hpp"
#include "server/server_thread.hpp"
#include "worker/abstract_callback_runner.hpp"
#include "worker/app_blocker.hpp"
#include "worker/worker_helper_thread.hpp"

namespace csci5570 {

enum class ModelType { SSP, BSP, ASP };
enum class StorageType { Map };  // May have Vector

class Engine {
 public:
  /**
   * Engine constructor
   *
   * @param node     the current node
   * @param nodes    all nodes in the cluster
   */
  Engine(const Node& node, const std::vector<Node>& nodes) : node_(node), nodes_(nodes) {
    callback_runner_ = std::move(std::unique_ptr<AbstractCallbackRunner>(new AppBlocker()));
  }
  /**
   * The flow of starting the engine:
   * 1. Create an id_mapper and a mailbox
   * 2. Start Sender
   * 3. Create ServerThreads and WorkerThreads
   * 4. Register the threads to mailbox through ThreadsafeQueue
   * 5. Start the communication threads: bind and connect to all other nodes
   *
   * @param num_server_threads_per_node the number of server threads to start on each node
   */
  void StartEverything(int num_server_threads_per_node = 1);
  void CreateIdMapper(int num_server_threads_per_node = 1);
  void CreateMailbox();
  void StartServerThreads();
  void StartWorkerThreads();
  void StartMailbox();
  void StartSender();

  /**
   * The flow of stopping the engine:
   * 1. Stop the Sender
   * 2. Stop the mailbox: by Barrier() and then exit
   * 3. The mailbox will stop the corresponding registered threads
   * 4. Stop the ServerThreads and WorkerThreads
   */
  void StopEverything();
  void StopServerThreads();
  void StopWorkerThreads();
  void StopSender();
  void StopMailbox();

  /**
   * Synchronization barrier for processes
   */
  void Barrier();
  /**
   * Create the whole picture of the worker group, and register the workers in the id mapper
   *
   * @param worker_alloc    the worker allocation information
   */
  WorkerSpec AllocateWorkers(const std::vector<WorkerAlloc>& worker_alloc);

  /**
   * Create the partitions of a model on the local servers
   * 1. Assign a table id (incremental and consecutive)
   * 2. Register the partition manager to the model
   * 3. For each local server thread maintained by the engine
   *    a. Create a storage according to <storage_type>
   *    b. Create a model according to <model_type>
   *    c. Register the model to the server thread
   *
   * @param partition_manager   the model partition manager
   * @param model_type          the consistency of model - bsp, ssp, asp
   * @param storage_type        the storage type - map, vector...
   * @param model_staleness     the staleness for ssp model
   * @return                    the created table(model) id
   */
  template <typename Val>
  uint32_t CreateTable(std::unique_ptr<AbstractPartitionManager>&& partition_manager, ModelType model_type,
                       StorageType storage_type, int model_staleness = 0) {
    // TODO: support vector storage
    // 1. Assign a table id (incremental and consecutive)
    uint32_t model_id = model_count_++;
    // 2. Register the partition manager to the model
    RegisterPartitionManager(model_id, std::move(partition_manager));
    // 3. Register model for each local server thread
    using StoragePtr = std::unique_ptr<AbstractStorage>;
    using ModelPtr = std::unique_ptr<AbstractModel>;
    for (auto& server_thread : server_thread_group_) {
      StoragePtr storage;
      ModelPtr model;
      ThreadsafeQueue<Message>* reply_queue = sender_->GetMessageQueue();
      switch(storage_type) {
        case StorageType::Map:
          storage = std::move(static_cast<StoragePtr>(new MapStorage<Val>()));
          break;
        default:
          storage = std::move(static_cast<StoragePtr>(new MapStorage<Val>()));
          break;
      }
      switch(model_type) {
        case ModelType::SSP:
          model = std::move(static_cast<ModelPtr>(
                                    new SSPModel(model_id, std::move(storage), model_staleness, reply_queue)));
          break;
        case ModelType::ASP:
          model = std::move(static_cast<ModelPtr>(
                                    new ASPModel(model_id, std::move(storage), reply_queue)));
          break;
        case ModelType::BSP:
          model = std::move(static_cast<ModelPtr>(
                                    new BSPModel(model_id, std::move(storage), reply_queue)));
          break;
        default:
          model = std::move(static_cast<ModelPtr>(
                                    new SSPModel(model_id, std::move(storage), model_staleness, reply_queue)));
          break;
      }
      server_thread->RegisterModel(model_id, std::move(model));
    }

    return model_id;
  }

  /**
   * Create the partitions of a model on the local servers using a default partitioning scheme
   * 1. Create a default partition manager
   * 2. Create a table with the partition manager
   *
   * @param model_type          the consistency of model - bsp, ssp, asp
   * @param storage_type        the storage type - map, vector...
   * @param model_staleness     the staleness for ssp model
   * @return                    the created table(model) id
   */
  template <typename Val>
  uint32_t CreateTable(ModelType model_type, StorageType storage_type, int model_staleness = 0) {
    std::vector<uint32_t> server_ids = id_mapper_->GetAllServerThreads();
    std::unique_ptr<AbstractPartitionManager> pm(new HashPartitionManager(server_ids));
    return CreateTable<Val>(std::move(pm), model_type, storage_type, model_staleness);
  }

  /**
   * Reset workers in the specified model so that each model knows the workers with the right of access
   */
  void InitTable(uint32_t table_id, const std::vector<uint32_t>& worker_ids);

  /**
   * Run the task
   *
   * After starting the system, the engine run a task by starting the prescribed threads to run UDF
   *
   * @param task    the task to run
   */
  void Run(const MLTask& task);

  /**
   * Returns the server thread ids
   */
  std::vector<uint32_t> GetServerThreadIds() { return id_mapper_->GetAllServerThreads(); }

  /**
   * Returns the worker helper thread of the worker user thread
   * @param thread_id   worker user thread id
   * @return
   */
  WorkerHelperThread* GetHelperOfWorker(uint32_t thread_id);

  /**
   * Return the helpee node id of current node in round-robin
   * @return
   */
  uint32_t GetHelpeeNode();

  uint32_t GetHelpeeThreadId();

 private:
  /**
   * Register partition manager for a model to the engine
   *
   * @param table_id            the model id
   * @param partition_manager   the partition manager for the specific model
   */
  void RegisterPartitionManager(uint32_t table_id, std::unique_ptr<AbstractPartitionManager>&& partition_manager);

  std::map<uint32_t, std::unique_ptr<AbstractPartitionManager>> partition_manager_map_;
  // nodes
  Node node_;
  std::vector<Node> nodes_;
  // mailbox
  std::unique_ptr<SimpleIdMapper> id_mapper_;
  std::unique_ptr<Mailbox> mailbox_;
  std::unique_ptr<Sender> sender_;
  // worker elements
  std::unique_ptr<AbstractCallbackRunner> callback_runner_;
//  std::unique_ptr<WorkerThread> worker_thread_;
  std::vector<std::unique_ptr<WorkerHelperThread>> worker_thread_group_;
  // server elements
  std::vector<std::unique_ptr<ServerThread>> server_thread_group_;
  size_t model_count_ = 0;
};

}  // namespace csci5570
