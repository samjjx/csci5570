//
// Created by samjjx on 2018/12/20.
//

#ifndef CSCI5570_SVM_HPP
#define CSCI5570_SVM_HPP

#include "base/magic.hpp"
#include <vector>
#include <Eigen/Dense>
#include <cmath>
#include "lib/svm_sample.hpp"

using namespace Eigen;
using namespace csci5570;
using DataStore = std::vector<lib::SVMSample>;


template <typename T>
class SVM {

public:
    SVM(DataStore* data_store)
    :data_store_(data_store) {
      for(auto& row : (*data_store_)) {
        for(auto& col : row.x_) {
          theta_[col.first] = 0.0;
          grad_[col.first] = 0.0;
        }
      }
    }
    std::vector<double> compute_gradients(const std::vector<lib::SVMSample>& samples, const std::vector<Key>& keys,
                                          const std::vector<double>& vals, double alpha) {
      std::vector<double> deltas(keys.size(), 0.);
      for (auto sample : samples) {
        auto& x = sample.x_;
        double y = sample.y_;
        double predict = 0.;
        int idx = 0;
        for (auto& field : x) {
          while (keys[idx] < field.first)
            ++idx;
          predict += vals[idx] * field.second;
        }
        predict += vals.back();
        int predictLabel;
        if (predict >= 0) {
          predictLabel = 1;
        } else {
          predictLabel = -1;
        }

        idx = 0;
        for (auto& field : x) {
          while (keys[idx] < field.first)
            ++idx;
          deltas[idx] += alpha * field.second * (y - predictLabel);
        }
        deltas[deltas.size() - 1] += alpha * (y - predictLabel);
      }
      return deltas;
    }

    double correct_rate(const std::vector<lib::SVMSample>& samples, const std::vector<Key>& keys,
                        const std::vector<double>& vals) {
      int total = samples.size();
      double n = 0;
      for (auto sample : samples) {
        auto& x = sample.x_;
        double y = sample.y_;
        double predict = 0.;
        int idx = 0;
        for (auto& field : x) {
          while (keys[idx] < field.first)
            ++idx;
          predict += vals[idx] * field.second;
        }
        predict += vals.back();
        int predict_;
        if (predict >= 0) {
          predict_ = 1;
        } else {
          predict_ = -1;
        }
        if (predict_ == y) {
          n++;
        }
      }
      double result = n / total;
      return result;
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
};


#endif //CSCI5570_SVM_HPP
