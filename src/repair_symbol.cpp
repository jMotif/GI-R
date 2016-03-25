#include <repair.h>
using namespace Rcpp ;

// constructor
//
repair_symbol::repair_symbol(const std::string str, int index ) {
  payload = str;
  str_index = index;
}

// rule level, always 0 for the symbol
//
int repair_symbol::get_level() {
  return 0;
}

