#include "server/util/pending_buffer.hpp"

namespace csci5570 {

std::vector<Message> PendingBuffer::Pop(const int clock) {
  auto it = buffer_.find(clock);
  std::vector<Message> msgs;
  if (it != buffer_.end()) {
    msgs = it->second;
    buffer_.erase(it);
  }
  return msgs;
}

void PendingBuffer::Push(const int clock, Message& msg) {
  if (buffer_.find(clock) == buffer_.end()) {
    buffer_[clock] = std::vector<Message>();
  }
  buffer_[clock].push_back(msg);
}

int PendingBuffer::Size(const int progress) {
  auto it = buffer_.find(progress);
  if (it == buffer_.end()) {return 0;}
  return buffer_[progress].size();
}

}  // namespace csci5570
