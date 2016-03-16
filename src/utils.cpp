#include <Rcpp.h>
#include <repair.h>
using namespace Rcpp;

int _count_spaces(std::string *s) {
  int count = 0;
  for (int i = 0; i < s->size(); i++)
    if (s->at(i) == ' ') count++;
    return count;
}
