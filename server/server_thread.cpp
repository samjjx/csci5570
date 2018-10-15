#include "server/server_thread.hpp"

#include "glog/logging.h"

namespace csci5570 {

void ServerThread::RegisterModel(uint32_t model_id, std::unique_ptr<AbstractModel>&& model) {
    models_[model_id] = std::move(model);
}

AbstractModel* ServerThread::GetModel(uint32_t model_id) {
    auto it = models_.find(model_id);
    if (it != models_.end()) {
        return it->second.get();
    } else {
        return nullptr;
    }
}

uint32_t ServerThread::GetServerId() {
    return GetId();
}

void ServerThread::Main() {
    while(true) {
        Message msg;
        work_queue_.WaitAndPop(&msg);
        // is this necessary?
//        if (msg.meta.recver != id_) {
//            return;
//        }
        if (msg.meta.flag == Flag::kExit) {
            LOG(INFO) << "server thread exit";
            break;
        }
        if (msg.meta.flag == Flag::kClock) {
            uint32_t model_id = msg.meta.model_id;
            auto* model = GetModel(model_id);
            model->Clock(msg);
        }
        if (msg.meta.flag == Flag::kAdd) {
            uint32_t model_id = msg.meta.model_id;
            auto* model = GetModel(model_id);
            model->Add(msg);
        }
        if (msg.meta.flag == Flag::kGet) {
            uint32_t model_id = msg.meta.model_id;
            auto* model = GetModel(model_id);
            model->Get(msg);
        }
        if (msg.meta.flag == Flag::kResetWorkerInModel) {
            auto* model = GetModel(msg.meta.model_id);
            model->ResetWorker(msg);
        }
    }
}

}  // namespace csci5570

