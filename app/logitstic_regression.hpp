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

float get_random()
{
    static std::default_random_engine e;
    static std::uniform_real_distribution<> dis(0, 1); // rage 0 - 1
    return dis(e);
}

template <typename T>
class LogisticRegression {
public:
  LogisticRegression(DataStore* data_store, DataRange range, float learning_rate=0.001)
    : data_store_(data_store), range_(range), learning_rate_(learning_rate) {
    float pos = 0;
    // initialize theta
    for (uint32_t i = range_.start; i < range_.end; i++) {
      if (data_store_->at(i).y_ > 0) {pos+=1;}
      for(auto& col : data_store_->at(i).x_) {
        theta_[col.first] = 0.5;
        //theta_[col.first] = get_random();
      }
    }
    //pos_ratio = pos/range.length;
    //LOG(INFO) << "possitive ratio: " << pos/range.length;
    pos_ratio = 0.5;
  }

  double predict(const lib::SVMSample& sample) {
    double z = 0;
    for(auto& col : sample.x_) {
      Key key = col.first;
      double x = col.second;
      //LOG(INFO) << "x: " << x << " theta: " << (double)theta_[key] << " x * theta: " << x * theta_[key];
      z += x * theta_[key];
    }
    //LOG(INFO) << z;
    //double logit = 1.0 / (1 + std::exp(-z));
    //return std::max(1e-9, std::min(1-1e-9, logit));
    return 1.0 / (1 + std::exp(-z));
  }

  double get_loss() {
    double loss = 0;
    for (uint32_t i = range_.start; i < range_.end; i++) {
      int y = data_store_->at(i).y_ <= 0 ? 0 : 1;
      float sample_weight = 1;
      if (y==1) {sample_weight = (1-pos_ratio)/pos_ratio;}
      else {sample_weight = pos_ratio/(1-pos_ratio);}
      double pred = predict(data_store_->at(i));
      loss += sample_weight * (-1 * y * std::log(pred) - (1 - y) * std::log(1 - pred));
    }
    return loss/range_.length;
  }

  /**
   * compute gradient of one sample
   * @param row
   * @param grad
   */
  void compute_gradient(uint32_t i, std::unordered_map<Key, T>& grad) {
    const lib::SVMSample& row = data_store_->at(i);
//    for(auto& g : grad_) {
//      g.second = 0.0;
//    }
    // NOTICE that row.y_ maybe +1/-1
    auto y = row.y_ <= 0 ? 0 : 1;
    float sample_weight = 1;
    if (y==1) {sample_weight = (1-pos_ratio)/pos_ratio;}
    else {sample_weight = pos_ratio/(1-pos_ratio);}

    /*
    auto z = 0;
    for(auto& col : row.x_) {
      Key key = col.first;
      auto x = col.second;
      z += x * theta_[key];
    }
    auto g = 1 / (1 + std::exp(-z));
    */
    auto g = predict(row);
    for(auto& col : row.x_) {
      Key key = col.first;
      auto x = col.second;
      T grad_in_dim = -learning_rate_ * x * (g - y);
      grad_in_dim *= sample_weight;
      if (grad.find(key) != grad.end()) {
        grad[key] += grad_in_dim;
      } else {
        grad[key] = grad_in_dim;
      }
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
    float avg_z = 0;
    for (uint32_t i = range_.start; i < range_.end; i++) {
      // NOTICE that row.y_ maybe +1/-1
      int y = data_store_->at(i).y_ <= 0 ? 0 : 1;
      double z = 0;
      for(auto& col : data_store_->at(i).x_) {
        Key key = col.first;
        float x = col.second;
        z += x * theta_[key];
      }
      avg_z += z;
      double g = 1 / (1 + std::exp(-z));
      int y_pred = g <= 0.5 ? 0 : 1;
      if (y_pred == y) {correct++;}
      total++;
      //if (z > 0) {
      //LOG(INFO) << z << "," << g << "," << y_pred << "," << y;
      //}
    }
    if (total == 0) {
      return -1;
    }
    LOG(INFO) << avg_z/total;
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
  float pos_ratio;
};


#endif //CSCI5570_LOGITSTIC_REGRESSION_HPP
