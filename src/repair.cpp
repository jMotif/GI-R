#include <repair.h>
using namespace Rcpp;

//' Runs RePair algorithm on the string.
//'
//' @param str the input string.
//' @useDynLib GI
//' @export
//' @references  N.J. Larsson and A. Moffat. Offline dictionary-based compression.
//' In Data Compression Conference, 1999.
//' @examples
//' str_to_repair_grammar("abc abc cba cba bac xxx abc abc cba cba bac")
// [[Rcpp::export]]
std::unordered_map<int, std::string> str_to_repair_grammar(CharacterVector str) {

  std::unordered_map<int, std::string> res;
  std::unordered_map<int, repair_rule> grammar;

  // convert the string
  std::string s = Rcpp::as<std::string>(str);

  // define the objects we are working with
  std::string delimiter = " ";
  s.append(delimiter);

  // global to the function data structure
  std::vector<repair_symbol_record*> r0; // this is the R0 tokens sequence
  r0.reserve(_count_spaces(&s));

  // the queue
  std::unordered_map<std::string, std::vector<int>> digram_table;

  // tokenizer variables and counters
  std::string old_token;
  std::string token;
  int token_counter = 0;
  int pos = 0;
  while ((pos = s.find(delimiter)) != std::string::npos) {

    // extract the token
    token = s.substr(0, pos);
    Rcout << "current token: " << token << std::endl;

    // create the symbol
    repair_symbol* rs = new repair_symbol( token, token_counter );
    repair_symbol_record* rec = new repair_symbol_record( rs ); // wrap the record
    r0.emplace_back( rec ); // place into the work string

    if(token_counter > 0){

      // create the digram
      std::string d_str = old_token + " " + token;
      // Rcout << " . new digram: " << d_str << std::endl;

      // save the digram occurrence
      if (digram_table.find(d_str) == digram_table.end()){
        std::vector<int> occurrences;
        occurrences.push_back(token_counter - 1);
        digram_table.insert(std::make_pair(d_str, occurrences));
      }else{
        digram_table[d_str].push_back(token_counter - 1);
      }

      r0[token_counter - 1]->next = rec;
      rec->prev = r0[token_counter - 1];
    }

    // do the input string housekeeping
    s.erase(0, pos + delimiter.length());
    old_token = token;
    token_counter++;
  }

  // walk over the R0
  Rcout << "\nthe R0: ";
  repair_symbol_record* ptr = r0[0];
  do {
    Rcout << ptr->payload->payload << " ";
    ptr = ptr->next;
  } while(ptr != nullptr);
  Rcout << std::endl;

  // all digrams are accounted for... print their state
  Rcout << "\nthe digrams table\n=================" << std::endl;
  for(std::unordered_map<std::string, std::vector<int>>::iterator it = digram_table.begin();
      it != digram_table.end(); ++it) {
    Rcout << it->first << " [";
    for (auto i = it->second.begin(); i != it->second.end(); ++i)
      Rcout << *i << ", ";
    Rcout << "]" << std::endl;
  }

  // populate the priority queue and the index -> digram record map
  //
  Rcout << "\npopulating the queue\n=================" << std::endl;
  repair_priority_queue digram_queue;
  for(std::unordered_map<std::string, std::vector<int>>::iterator it = digram_table.begin();
      it != digram_table.end(); ++it) {
    repair_digram* digram = new repair_digram( it->first, it->second.size() );
    digram_queue.enqueue(digram);
  }

  // all digrams are pushed to the queue, see those
  Rcout << "\nthe digrams queue\n=================" << std::endl;
  Rcout << digram_queue.to_string() << std::endl;

  repair_digram* entry = digram_queue.dequeue();
  while (entry != nullptr) {
    std::unordered_map<std::string, std::vector<int>>::iterator it =
      digram_table.find(entry->digram);
    Rcout <<"\npopped digram: " << it->first << " [";
    for (auto i = it->second.begin(); i != it->second.end(); ++i)
      Rcout << *i << ", ";
    Rcout << "]" << std::endl;

    repair_symbol_record* first = r0[it->second[0]];
    repair_symbol_record* second = r0[it->second[0]+1];
    Rcout << " *** " << first->payload->payload << " @" << first->payload->str_index;
    Rcout << " *** " << second->payload->payload << " @" << second->payload->str_index;

    // create a new rule using the diram data
    repair_rule* r = new repair_rule();
    r->id = grammar.size()+1;
    r->first = first->payload;
    r->second = second->payload;
    // r.assign_level();
    r->expanded_rule_string = *(r->first->get_expanded_string()) + " " +
      *(r->second->get_expanded_string());
      Rcout << " *** rule: " << r->get_rule_string() << " -> " <<
        r->expanded_rule_string << std::endl;
      grammar.insert(std::pair<int, repair_rule>(r->id, *r));

    // digram occurrences processing...


    entry = digram_queue.dequeue();
  }
  return res;
}
