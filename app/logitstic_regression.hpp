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

using namespace Eigen;
using namespace csci5570;
using DataStore = std::vector<lib::SVMSample>;

template <typename T>
class LogisticRegression {
public:
  LogisticRegression(DataStore* data_store, float learning_rate=0.01)
    : data_store_(data_store), learning_rate_(learning_rate) {
    // initialize theta
    for(auto& row : (*data_store_)) {
      for(auto& col : row.x_) {
        theta_[col.first] = 0.0;
        grad_[col.first] = 0.0;
      }
    }
  }

  void compute_gradient(std::vector<T>& grad) {
    for(auto& g : grad_) {
      g.second = 0.0;
    }
    for (auto& row : (*data_store_)) {
      // NOTICE that row.y_ is +1/-1
      auto y = row.y_ < 0 ? 0 : 1;
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
    }
    for(auto& g : grad_) {
      grad.push_back(g.second);
    }
//    Matrix<T, Dynamic, 1> Z = (*X_) * theta_;
//    Matrix<T, Dynamic, 1> G = Z.unaryExpr([](T z){
//      return 1 / (1 + std::exp(-z));
//    });
//    grad = -learning_rate_ * X_->transpose() * (G - (*Y_));
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
    for (auto& row : (*data_store_)) {
      // NOTICE that row.y_ is +1/-1
      auto y = row.y_ < 0 ? 0 : 1;
      auto z = 0;
      for(auto& col : row.x_) {
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
  float learning_rate_;
};


#endif //CSCI5570_LOGITSTIC_REGRESSION_HPP
