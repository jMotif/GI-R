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

  std::unordered_set<std::string> new_digrams;
  repair_digram* entry = digram_queue.dequeue();
  while (entry != nullptr) {

    std::vector<int> occurrences;
    occurrences.reserve(entry->freq);

    // the digram entry from the master table
    std::unordered_map<std::string, std::vector<int>>::iterator it =
      digram_table.find(entry->digram);
    Rcout <<"\npopped digram: " << it->first << " [";
    for (auto i = it->second.begin(); i != it->second.end(); ++i) {
      Rcout << *i << ", ";
      occurrences.push_back(*i);
    }
    Rcout << "]" << std::endl;

    // work on the NEW RULE construction
    repair_symbol_record* first = r0[it->second[0]];
    repair_symbol_record* second = r0[it->second[0]+1];
    Rcout << " *** " << first->payload->payload << " @" << first->payload->str_index;
    Rcout << " *** " << second->payload->payload << " @" << second->payload->str_index;
    //
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

    // digram occurrences processing, iterating over each position...
    while(!occurrences.empty()){

      // secure an index
      int occ = occurrences[occurrences.size()-1];
      occurrences.pop_back();

      // save the positions we work with
      repair_symbol_record* curr_sym = r0[occ];
      repair_symbol_record* next_sym = r0[occ + 1];

      // make up a guard for the rul created before
      repair_guard* g = new repair_guard(r, occ);
      repair_symbol_record* guard = new repair_symbol_record(g);

      // alter the R0 by placing the guard...
      r0[occ] = guard;

      // and fixing the OLE next symbol link
      repair_symbol_record* next_not_null = r0[occ]->next;
      guard->next = next_not_null;
      if(nullptr!=next_not_null){
        next_not_null->prev = guard;
      }

      // and fixing the OLE prev symbol link
      repair_symbol_record* prev_not_null = r0[occ]->prev;
      guard->prev = prev_not_null;
      if(nullptr!=prev_not_null){
        prev_not_null->next = guard;
      }

      // now need to fix the OLE left digram
      if(occ > 0 && nullptr!=prev_not_null){

        std::string ole_left_digram = prev_not_null->payload->to_string() + " "
        + curr_sym->payload->to_string();

        std::vector<int> digram_occurrences = digram_table[ole_left_digram];
        int new_freq = digram_occurrences.size() - 1;

        // clean up the specific index in the occurrences array
        digram_occurrences.erase(std::remove(occurrences.begin(), occurrences.end(),
                                      prev_not_null->payload->str_index), occurrences.end());

        // take a look if the digram is actually the one we work with...
        if (ole_left_digram.compare(entry->digram)) {
          occurrences.erase(std::remove(occurrences.begin(), occurrences.end(),
                       prev_not_null->payload->str_index), occurrences.end());
        }
        digram_queue.update_digram_frequency(&ole_left_digram, new_freq);

        // if it was the last entry...
        if (0 == new_freq) {
          digram_table.erase(ole_left_digram);
          new_digrams.erase(ole_left_digram);
        }

        // and place the new digram entry
        String new_left_digram = prev_not_null->payload->to_string() + " " + r->get_rule_string();
        // see the new freq..
        // save the digram occurrence
        if (digram_table.find(new_left_digram) == digram_table.end()){
          std::vector<int> occurrences;
          occurrences.push_back(token_counter - 1);
          digram_table.insert(std::make_pair(new_left_digram, occurrences));
        }else{
          digram_table[new_left_digram].push_back(token_counter - 1);
        }

        new_digrams.emplace(new_left_digram);
      }

      // now need to fix the OLE right digram
      repair_symbol_record* after_second = r0[it->second[0]+2];
      if(occ < r0.size() - 2 && nullptr != after_second){

        std::string ole_right_digram = next_sym->payload->to_string() + " "
        + after_second->payload->to_string();

        std::vector<int> digram_occurrences = digram_table[ole_right_digram];
        int new_freq = digram_occurrences.size() - 1;

        // clean up the specific index in the occurrences array
        digram_occurrences.erase(std::remove(occurrences.begin(), occurrences.end(),
                       next_sym->payload->str_index), occurrences.end());

        // take a look if the digram is actually the one we work with...
        if (ole_right_digram.compare(entry->digram)) {
          occurrences.erase(std::remove(occurrences.begin(), occurrences.end(),
                        next_sym->payload->str_index), occurrences.end());
        }
        digram_queue.update_digram_frequency(&ole_right_digram, new_freq);

        // if it was the last entry...
        if (0 == new_freq) {
          digram_table.erase(ole_right_digram);
          new_digrams.erase(ole_right_digram);
        }

        // and place the new digram entry
        String new_left_digram = prev_not_null->payload->to_string() + " " + r->get_rule_string();
        // see the new freq..
        // save the digram occurrence
        if (digram_table.find(new_left_digram) == digram_table.end()){
          std::vector<int> occurrences;
          occurrences.push_back(token_counter - 1);
          digram_table.insert(std::make_pair(new_left_digram, occurrences));
        }else{
          digram_table[new_left_digram].push_back(token_counter - 1);
        }

        new_digrams.emplace(new_left_digram);
      }

      Rcout << occ << ", ";


    }
    Rcout << std::endl;

    entry = digram_queue.dequeue();
  }
  return res;
}
