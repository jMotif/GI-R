#include <repair.h>
using namespace Rcpp ;

// constructor
//
repair_symbol::repair_symbol(const std::string str, int index ) {
  payload = str;
  str_index = index;
}

// this is overriden in other classes
//
bool repair_symbol::is_guard() {
  return false;
}

// rule level, always 0 for the symbol
//
int repair_symbol::get_level() {
  return 0;
}

// destructor
//
repair_symbol::~repair_symbol(){

}
