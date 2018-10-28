#pragma once
#include "boost/algorithm/string/finder.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/utility/string_ref.hpp"
#include "boost/algorithm/string/classification.hpp"
#include "lib/svm_sample.hpp"

namespace csci5570 {
namespace lib {

template <typename Sample, typename DataStore>
class Parser {
 public:
  /**
   * Parsing logic for one line in file
   *
   * @param line    a line read from the input file
   */
  static Sample parse_libsvm(boost::string_ref line, int n_features) {
    // check the LibSVM format and complete the parsing
    // hints: you may use boost::tokenizer, std::strtok_r, std::stringstream or any method you like
    // so far we tried all the tree and found std::strtok_r is fastest :)
    Sample sample = SVMSample();

    using V = std::vector<boost::string_ref>;
    using S = boost::string_ref;

    S label;

    size_t p;
    // get label
    p = line.find_first_of(" ");
    label = line.substr(0, p);
    sample.y_ = std::stoi(label.to_string());


    line.remove_prefix(p+1);

    while(true){
      p = line.find_first_of(" ");
      if (p == S::npos) {
        break;
      }
      S data = line.substr(0, p);
      size_t p1;

      p1 = data.find_first_of(";");
      auto index = std::stoi(data.substr(0,p1).to_string());
      data.remove_prefix(p1+1);
      auto value = std::stoi(data.to_string());
      line.remove_prefix(p+1);
      sample.x_.push_back(std::make_pair(index,value));
    }
    return sample;
  }

  static Sample parse_mnist(boost::string_ref line, int n_features) {
    // check the MNIST format and complete the parsing
  }

  // You may implement other parsing logic

};  // class Parser

}  // namespace lib
}  // namespace csci5570
