#include <repair.h>
using namespace Rcpp ;

// constructor
//
repair_symbol_record::repair_symbol_record( repair_symbol symbol ) {
  payload = symbol;
}

// destructor
//
repair_symbol_record::~repair_symbol_record(){

}
