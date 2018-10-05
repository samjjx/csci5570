#pragma once
#include "boost/algorithm/string/finder.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/utility/string_ref.hpp"
#include "boost/algorithm/string/classification.hpp"

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

    using V = std::vector<boost::string_ref>;
    V index, value;
    using S = boost::string_ref;

    S label;

    size_t p;
    // get label
    p = line.find_first_of(" ");
    label = line.substr(0, p);

    if (p == S::npos) {
      return NULL;
    }

    line.remove_prefix(p+1);

    while(true){
      p = line.find_first_of(" ");
      if (p == S::npos) {
        break;
      }
      S data = line.substr(0, p);
      size_t p1;

      p1 = data.find_first_of(";");
      index.push_back(data.substr(0,p1));
      data.remove_prefix(p1+1);
      value.push_back(data);


      line.remove_prefix(p+1);
    }

  }

  static Sample parse_mnist(boost::string_ref line, int n_features) {
    // check the MNIST format and complete the parsing
  }

  // You may implement other parsing logic

};  // class Parser

}  // namespace lib
}  // namespace csci5570
