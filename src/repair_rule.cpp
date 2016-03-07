#include <repair.h>
#include <Rcpp.h>
using namespace Rcpp;

repair_rule::repair_rule(){
  id = -1;
  rule_use = 0;
};

repair_rule::repair_rule(int r_id, std::string rule_str, std::string expanded_rule_str){
  id = r_id;
  rule_use = 0;
  rule_string = rule_str;
  expanded_rule_string = expanded_rule_str;
};

std::string repair_rule::get_rule_string(){
  std::stringstream ss;
  ss << id;
  return "R" + ss.str();
};
