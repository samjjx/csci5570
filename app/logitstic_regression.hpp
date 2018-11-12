//
// Created by jennings on 2018/10/26.
//

#ifndef CSCI5570_LOGITSTIC_REGRESSION_HPP
#define CSCI5570_LOGITSTIC_REGRESSION_HPP

#include "base/magic.hpp"
#include <vector>
#include <Eigen/Dense>
#include <cmath>
#include "lib/svm_sample.hpp"
#include "driver/work_assigner.hpp"

using namespace Eigen;
using namespace csci5570;
using DataStore = std::vector<lib::SVMSample>;

template <typename T>
class LogisticRegression {
public:
  LogisticRegression(DataStore* data_store, DataRange range, float learning_rate=0.001)
    : data_store_(data_store), range_(range), learning_rate_(learning_rate) {
    // initialize theta
    for (uint32_t i = range_.start; i < range_.end; i++) {
      for(auto& col : data_store_->at(i).x_) {
        theta_[col.first] = 0.0;
      }
    }
  }

  double predict(const lib::SVMSample& sample) {
    double z = 0;
    for(auto& col : sample.x_) {
      Key key = col.first;
      auto x = col.second;
      z += x * theta_[key];
    }
    return 1.0 / (1 + std::exp(-z));
  }

  double get_loss() {
    double loss = 0;
    for (uint32_t i = range_.start; i < range_.end; i++) {
      int y = data_store_->at(i).y_ <= 0 ? 0 : 1;
      double pred = predict(data_store_->at(i));
      loss += (-1 * y * std::log(pred) - (1 - y) * std::log(1 - pred));
    }
    return loss/range_.length;
  }

  /**
   * compute gradient of one sample
   * @param row
   * @param grad
   */
  void compute_gradient(uint32_t i, std::vector<T>& grad) {
    const lib::SVMSample& row = data_store_->at(i);
    for(auto& g : grad_) {
      g.second = 0.0;
    }
    // NOTICE that row.y_ maybe +1/-1
    auto y = row.y_ <= 0 ? 0 : 1;
    auto z = 0;
    for(auto& col : row.x_) {
      Key key = col.first;
      auto x = col.second;
      z += x * theta_[key];
    }
    auto g = 1 / (1 + std::exp(-z));
    for(auto& col : row.x_) {
      Key key = col.first;
      auto x = col.second;
      grad_[key] += -learning_rate_ * x * (g - y);
    }
    for(auto& g : grad_) {
      grad.push_back(g.second);
    }
  }

  void update_theta(const std::vector<Key>& keys, const std::vector<T>& vals) {
    uint32_t size = keys.size();
    for(uint32_t i = 0; i < size; i++) {
      if (theta_.find(keys[i]) == theta_.end()) {continue;}
      theta_[keys[i]] = vals[i];
    }
  }

  float test_acc() {
    float correct = 0;
    uint32_t total = 0;
    for (uint32_t i = range_.start; i < range_.end; i++) {
      // NOTICE that row.y_ maybe +1/-1
      auto y = data_store_->at(i).y_ <= 0 ? 0 : 1;
      auto z = 0;
      for(auto& col : data_store_->at(i).x_) {
        Key key = col.first;
        auto x = col.second;
        z += x * theta_[key];
      }
      auto g = 1 / (1 + std::exp(-z));
      auto y_pred = g <= 0.5 ? 0 : 1;
      if (y_pred == y) {correct++;}
      total++;
    }
    if (total == 0) {
      return -1;
    }
    return correct / total;
  }

  void get_keys(std::vector<Key>& keys) {
    for(auto& kv : theta_) {
      keys.push_back(kv.first);
    }
  }

private:
  DataStore* data_store_;
  std::map<Key, T> theta_;
  std::map<Key, T> grad_;
  DataRange range_;
  float learning_rate_;
};


#endif //CSCI5570_LOGITSTIC_REGRESSION_HPP
