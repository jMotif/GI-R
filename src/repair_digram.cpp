#include <repair.h>
using namespace Rcpp ;

// constructor
//
repair_digram::repair_digram( const std::string str, int index ) {
 digram = str;
 freq = index;
};
