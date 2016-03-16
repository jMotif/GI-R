#include <repair.h>
#include <Rcpp.h>
using namespace Rcpp;

repair_guard::repair_guard(){
};

repair_guard::repair_guard(repair_rule rule, int idx){
  r = rule;
  payload = r.get_rule_string();
  str_index = idx;
};

// this is overriden in other classes
//
bool repair_guard::is_guard() {
  return true;
}
