#ifndef REPAIR_h
#define REPAIR_h
//
#include <Rcpp.h>
#include <cstddef>
//
using namespace Rcpp ;
// Enable C++11 via this plugin (Rcpp 0.10.3 or later)
// [[Rcpp::plugins("cpp11")]]

// the basic token
//
class repair_symbol {
public:
  std::string payload;
  int str_index;
  repair_symbol() {
    str_index = -1;
  };
  repair_symbol(const std::string str, int index);
  virtual bool is_guard(){
    return false;
  }
  int get_level();
  std::string to_string(){
    return payload;
  }
};

// the symbol (token) wrapper for the string data structure0
//
class repair_symbol_record {
public:
  repair_symbol* payload;
  repair_symbol_record* prev;
  repair_symbol_record* next;
  repair_symbol_record( repair_symbol* symbol );
};

class repair_rule {
public:
  int id;
  int rule_use;
  repair_symbol* first;
  repair_symbol* second;
  std::string expanded_rule_string;
  std::vector<int> occurrences;
  repair_rule(){
    id = -1; rule_use = 0; first = nullptr; second = nullptr;
  };
  std::string get_rule_string();
};

class repair_guard: public repair_symbol {
public:
  repair_rule* r;
  repair_guard();
  repair_guard(repair_rule* rule, int idx){
    r = rule;
    payload = r->get_rule_string();
    str_index = idx;
  }
  bool is_guard(){
    return true;
  }
  std::string get_expanded_string(){
    return r->expanded_rule_string;
  }
  std::string to_string(){
    return r->get_rule_string();
  }
};

class repair_digram {
public:
  std::string digram;
  int freq;
  repair_digram(const std::string str, int index);
};

class repair_pqueue_node {
public:
  repair_pqueue_node* prev;
  repair_pqueue_node* next;
  repair_digram* payload;
  repair_pqueue_node() {
    payload = nullptr;
    prev = nullptr;
    next = nullptr;
  }
  repair_pqueue_node(repair_digram* d) {
    payload = d;
    prev = nullptr;
    next = nullptr;
  }
};

class repair_priority_queue {
public:
  repair_pqueue_node* queue_head;
  std::unordered_map<std::string, repair_pqueue_node*> nodes;
  repair_priority_queue() {
    queue_head = nullptr;
    std::unordered_map<std::string, repair_pqueue_node*> nodes;
  }
  repair_digram* enqueue( repair_digram* digram );
  repair_digram* dequeue();
  repair_digram* peek();
  repair_digram* get(std::string *digram_string);
  repair_digram* update_digram_frequency(std::string *digram_string, int new_value);
  bool contains_digram(std::string *digram_string);
  std::vector<repair_digram> to_array();
  void remove_node(repair_pqueue_node* node);
  std::string to_string();
};

int _count_spaces(std::string *s);

#endif
