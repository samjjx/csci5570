#pragma once
#include "boost/algorithm/string/finder.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/utility/string_ref.hpp"
#include "boost/algorithm/string/classification.hpp"
#include<boost/tokenizer.hpp>
#include "lib/svm_sample.hpp"
#include "glog/logging.h"
#include "lib/mf_sample.hpp"

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
    boost::char_separator<char> sep(" "); 
    boost::tokenizer<boost::char_separator<char> > tok(line, sep);
    uint32_t i = 0;
    for(auto beg=tok.begin(); beg!=tok.end();++beg,i++){
       if (i != 0) {
         boost::char_separator<char> sep(":");
         boost::tokenizer<boost::char_separator<char> > tokens(*beg, sep);
         auto col_it = tokens.begin();
         uint32_t key = std::stoi(*col_it);
         col_it++;
         float value = std::atof((*col_it).c_str());
         sample.x_.push_back(std::make_pair(key,value));
       } else {sample.y_ = std::stoi(*beg);}
    }

    return sample;
  }

    static Sample parse_libsvm(boost::string_ref line)
    {
      Sample sample = MFSample();

      boost::char_separator<char> sep("/t");
      boost::tokenizer<boost::char_separator<char> > tokens(line, sep);
      auto iter = tokens.begin();
//
//
//    boost::tokenizer<boost::char_separator<char>> Tok;
//    boost::char_separator<char> sep ("\t");
//    Tok tok(line, sep);
//    auto iter = tok.begin();
//
//
//

      sample.x_.first = std::stoi(*iter);
      iter++;
      sample.x_.second = std::stoi(*iter);
      iter++;
      sample.y_ = std::stof(*iter);

      return sample;
    }


  static Sample parse_mnist(boost::string_ref line, int n_features) {
    // check the MNIST format and complete the parsing
  }

  // You may implement other parsing logic

};  // class Parser

}  // namespace lib
}  // namespace csci5570
