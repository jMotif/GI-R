#ifndef REPAIR_h
#define REPAIR_h
//
#include <Rcpp.h>
//
using namespace Rcpp ;
// Enable C++11 via this plugin (Rcpp 0.10.3 or later)
// [[Rcpp::plugins("cpp11")]]

class repair_symbol {
public:
  std::string payload;
  int str_index;

  repair_symbol( std::string str, int index);

  bool is_guard();
  int get_level();

  ~repair_symbol();
};

#endif
