//
// Created by samjjx on 2018/10/3.
//

#include <thread>

#include "boost/utility/string_ref.hpp"
#include "glog/logging.h"
#include "gtest/gtest.h"

#include "lib/abstract_data_loader.hpp"

namespace csci5570 {

    void Testing(){
      lib::AbstractDataLoader<int,int> dataLoader;
    }
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  FLAGS_stderrthreshold = 0;
  FLAGS_colorlogtostderr = true;
  csci5570::Testing();

}

