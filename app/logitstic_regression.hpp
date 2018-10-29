//
// Created by jennings on 2018/10/26.
//

#ifndef CSCI5570_LOGITSTIC_REGRESSION_HPP
#define CSCI5570_LOGITSTIC_REGRESSION_HPP
#include <vector>
#include <Eigen/Dense>
#include <cmath>

using namespace Eigen;

template <typename T, uint32_t D>
class LogisticRegression {
public:
  LogisticRegression(float learning_rate) : learning_rate_(learning_rate) {
    theta_ = Matrix<T, D, 1>::Zero();
  }

  void set_data(Matrix<T, Dynamic, D>* X, Matrix<T, Dynamic, 1>* Y) {
    X_ = X;
    Y_ = Y;
  }

  void compute_gradient(Matrix<T, D, 1>& grad) {
    Matrix<T, Dynamic, 1> Z = (*X_) * theta_;
    Matrix<T, Dynamic, 1> G = Z.unaryExpr([](T z){
      return 1 / (1 + std::exp(-z));
    });
    grad = -learning_rate_ * X_->transpose() * (G - (*Y_));
  }

  void update_theta(Matrix<T, D, 1> theta) {
    theta_ = theta;
  }

  void get_theta(Matrix<T, D, 1>& theta) {
    theta = theta_;
  }

private:
  Matrix<T, Dynamic, D>* X_;
  Matrix<T, D, 1> theta_;
  Matrix<T, Dynamic, 1>* Y_;
  float learning_rate_;
};


#endif //CSCI5570_LOGITSTIC_REGRESSION_HPP
