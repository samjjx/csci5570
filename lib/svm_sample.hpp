//
// Created by samjjx on 2018/10/28.
//
#include <string>
#include "base/third_party/sarray.h"
#include "lib/labeled_sample.hpp"

#ifndef CSCI5570_SVM_SAMPLE_HPP
#define CSCI5570_SVM_SAMPLE_HPP
namespace csci5570 {
    namespace lib {

class SVMSample : public LabeledSample<std::vector<std::pair<int, int>>, int> {
public:
    std::string toString() {

    }
};
    }  // namespace lib
}  // namespace csci5570

#endif //CSCI5570_SVM_SAMPLE_HPP
