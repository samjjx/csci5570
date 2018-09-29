//
// Created by jennings on 2018/9/28.
//
#ifndef CSCI5570_SERVER_THREAD_GROUP_HPP
#define CSCI5570_SERVER_THREAD_GROUP_HPP

#include "base/threadsafe_queue.hpp"
#include "server/server_thread.hpp"

namespace csci5570 {

class ServerThreadGroup {
    public:
    typedef std::vector<ServerThread*>::iterator iterator;
    ServerThreadGroup(std::vector<uint32_t> server_id_vec, ThreadsafeQueue<Message>* reply_queue)
        : reply_queue_(reply_queue) {
        for (auto id : server_id_vec) {
            server_threads.push_back(new ServerThread(id));
        }
    }

    iterator begin() {
        return server_threads.begin();
    }

    iterator end() {
        return server_threads.end();
    }

    ThreadsafeQueue<Message>* GetReplyQueue() {
        return reply_queue_;
    }

    private:
    std::vector<ServerThread*> server_threads;
    ThreadsafeQueue<Message>* reply_queue_;
};

} // namespace csci5570
#endif //CSCI5570_SERVER_THREAD_GROUP_HPP
