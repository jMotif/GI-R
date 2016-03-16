#include <repair.h>
using namespace Rcpp ;

// constructor
//
repair_digram_record::repair_digram_record( repair_digram* drecord ) {
  payload = drecord;
  next = nullptr;
  prev = nullptr;
}

// destructor
//
repair_digram_record::~repair_digram_record(){

}
