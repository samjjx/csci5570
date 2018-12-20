
#include <string>
#include "base/third_party/sarray.h"
#include "lib/labeled_sample.hpp"

#ifndef CSCI5570_MF_SAMPLE_HPP
#define CSCI5570_MF_SAMPLE_HPP
namespace csci5570 {
    namespace lib {

class MFSample : public LabeledSample<std::pair<int, int>, float> {
public:
    std::string toString() {

    }
};
    }  // namespace lib
}  // namespace csci5570

#endif //CSCI5570_MF_SAMPLE_HPP
