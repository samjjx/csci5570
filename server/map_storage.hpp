#pragma once

#include "base/message.hpp"
#include "server/abstract_storage.hpp"

#include "glog/logging.h"

#include <map>

namespace csci5570 {

template <typename Val>
class MapStorage : public AbstractStorage {
 public:
  MapStorage() = default;

  virtual void SubAdd(const third_party::SArray<Key>& typed_keys, 
      const third_party::SArray<char>& vals) override {
    auto typed_vals = third_party::SArray<Val>(vals);
    CHECK_EQ(typed_keys.size(), typed_vals.size());
    for (uint32_t i = 0; i < typed_keys.size(); i++) {
        if (storage_.find(typed_keys[i]) == storage_.end()) {
          storage_[typed_keys[i]] = typed_vals[i];
        } else {
          storage_[typed_keys[i]] += typed_vals[i];
        }
    }
  }

  virtual third_party::SArray<char> SubGet(const third_party::SArray<Key>& typed_keys) override {
    third_party::SArray<Val> reply_vals(typed_keys.size());
    for (uint32_t i = 0; i < typed_keys.size(); i++) {
        auto k = typed_keys[i];
        if (storage_.find(k) == storage_.end()) {
          storage_[k] = 0;
        }
        reply_vals[i] = storage_[k];
    }
    return third_party::SArray<char>(reply_vals);
  }

  virtual void FinishIter() override {}

 private:
  std::map<Key, Val> storage_;
};

}  // namespace csci5570
