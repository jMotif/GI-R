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
    if(it->second.size() > 1) {
      repair_digram* digram = new repair_digram( it->first, it->second.size() );
      digram_queue.enqueue(digram);
    }
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
    Rcout << " *** the initial digram instance " << std::endl;
    Rcout << " *** " << first->payload->payload << " @" << first->payload->str_index;
    Rcout << " *** " << second->payload->payload << " @" << second->payload->str_index << std::endl;
    //
    repair_rule* r = new repair_rule();
    r->id = grammar.size()+1;
    r->first = first->payload;
    r->second = second->payload;
    // r.assign_level();
    r->expanded_rule_string = *(r->first->get_expanded_string()) + " " +
      *(r->second->get_expanded_string());
      Rcout << " *** the rule: " << r->get_rule_string() << " -> " <<
        r->expanded_rule_string << std::endl;
      grammar.insert(std::pair<int, repair_rule>(r->id, *r));

    // digram occurrences processing, iterating over each position...
    while(!occurrences.empty()){

      // secure an index
      int occ = occurrences[occurrences.size()-1];
      occurrences.pop_back();
      Rcout << " *** processing an occurrence at " << occ << std::endl;

      // save the positions we work with
      repair_symbol_record* curr_sym = r0[occ];
      repair_symbol_record* next_sym = curr_sym->next;
      repair_symbol* old_first = curr_sym->payload;
      repair_symbol* old_second = next_sym->payload;
      Rcout << "  *** sym1: " << curr_sym->payload->to_string() << " sym2: "
      << next_sym->payload->to_string() << std::endl;

      // make up a guard for the rul created before
      repair_guard* guard = new repair_guard(r, occ);

      // alter the R0 by placing the guard...
      curr_sym->payload = guard;
      Rcout << "  *** placed guard at " << occ << ": " << guard->to_string() << std::endl;

      // and fixing the OLE next symbol link
      repair_symbol_record* next_not_null = next_sym->next;
      curr_sym->next = next_not_null;
      if(nullptr!=next_not_null){
        next_not_null->prev = curr_sym;
        Rcout << "  *** next not null at " << next_not_null->payload->str_index << ": "
        << next_not_null->payload->to_string() << std::endl;
      } else {
        Rcout << "  *** next is NULL" << std::endl;
      }

      // and fixing the OLE prev symbol link
      repair_symbol_record* prev_not_null = curr_sym->prev;
      curr_sym->prev = prev_not_null;
      if(nullptr!=prev_not_null){
        prev_not_null->next = curr_sym;
        Rcout << "  *** prev not null at " << prev_not_null->payload->str_index << ": "
              << prev_not_null->payload->to_string() << std::endl;
      } else {
        Rcout << "  *** prev is NULL" << std::endl;
      }

      // ### now need to fix the OLE left digram
      // ###
      if(occ > 0 && nullptr!=prev_not_null){
        Rcout << " *** fixing old left digram: ";

        std::string ole_left_digram = prev_not_null->payload->to_string() + " "
        + old_first->to_string();
        Rcout << ole_left_digram << std::endl;

        std::unordered_map<std::string, std::vector<int>>::iterator it =
          digram_table.find(ole_left_digram);
        int new_freq = it->second.size() - 1;
        Rcout << "  *** old occurrences: ";
        for (auto i = it->second.begin(); i != it->second.end(); ++i)
          Rcout << *i << ' ';
        Rcout << std::endl;

        // clean up the specific index in the occurrences array
        it->second.erase(std::remove(it->second.begin(), it->second.end(),
                   prev_not_null->payload->str_index), it->second.end());
        Rcout << "  *** new occurrences: ";
        for (auto i = it->second.begin(); i != it->second.end(); ++i)
          Rcout << *i << ' ';
        Rcout << std::endl;

        // take a look if the digram is actually the one we work with...
        if (0 == ole_left_digram.compare(entry->digram)) {
          Rcout << "  ***** the old digram is like the new one, cleaning up ..." << std::endl;
          Rcout << "  *** old ext. loop: ";
          for (auto i = occurrences.begin(); i != occurrences.end(); ++i) Rcout << *i << ' ';
          Rcout << std::endl;
          occurrences.erase(std::remove(occurrences.begin(), occurrences.end(),
                       prev_not_null->payload->str_index), occurrences.end());
          Rcout << "  *** new ext. loop: ";
          for (auto i = occurrences.begin(); i != occurrences.end(); ++i) Rcout << *i << ' ';
          Rcout << std::endl;
        }
        digram_queue.update_digram_frequency(&ole_left_digram, new_freq);

        // if it was the last entry...
        if (0 == new_freq) {
          Rcout << "  *** since new freq is 0, cleaning up ..." << std::endl;
          digram_table.erase(ole_left_digram);
          new_digrams.erase(ole_left_digram);
        }

        // and place the new digram entry
        std::string new_left_digram = prev_not_null->payload->to_string() + " " + r->get_rule_string();
        Rcout << "  *** the new left digram shall be : " << new_left_digram << std::endl;
        // see the new freq..
        // save the digram occurrence
        if (digram_table.find(new_left_digram) == digram_table.end()){
          std::vector<int> occurrences;
          occurrences.push_back(prev_not_null->payload->str_index);
          digram_table.insert(std::make_pair(new_left_digram, occurrences));
        }else{
          digram_table[new_left_digram].push_back(prev_not_null->payload->str_index);
        }

        new_digrams.emplace(new_left_digram);
      }

      // walk over the R0
      Rcout << "\nthe R0: ";
      ptr = r0[0];
      do {
        Rcout << ptr->payload->payload << " ";
        ptr = ptr->next;
      } while(ptr != nullptr);
      Rcout << std::endl;
      Rcout << "\nthe digrams table\n=================" << std::endl;
      for(std::unordered_map<std::string, std::vector<int>>::iterator it = digram_table.begin();
          it != digram_table.end(); ++it) {
        Rcout << it->first << " [";
        for (auto i = it->second.begin(); i != it->second.end(); ++i)
          Rcout << *i << ", ";
        Rcout << "]" << std::endl;
      }
      // all digrams are pushed to the queue, see those
      Rcout << "\nthe digrams queue\n=================" << std::endl;
      Rcout << digram_queue.to_string() << std::endl;

      // ### now need to fix the OLE right digram
      // ###
      repair_symbol_record* after_second = second->next;
      if(occ < r0.size() - 2 && nullptr != after_second){

        Rcout << " *** fixing old right digram: ";

        std::string ole_right_digram = next_sym->payload->to_string() + " "
        + after_second->payload->to_string();

        Rcout << ole_right_digram << std::endl;
        std::unordered_map<std::string, std::vector<int>>::iterator it =
          digram_table.find(ole_right_digram);
        int new_freq = it->second.size() - 1;
        Rcout << "  *** old occurrences: ";
        for (auto i = it->second.begin(); i != it->second.end(); ++i)
          Rcout << *i << ' ';
        Rcout << std::endl;

        // clean up the specific index in the occurrences array
        it->second.erase(std::remove(it->second.begin(), it->second.end(),
                      next_sym->payload->str_index), it->second.end());
        Rcout << "  *** new occurrences: ";
        for (auto i = it->second.begin(); i != it->second.end(); ++i)
          Rcout << *i << ' ';
        Rcout << std::endl;

        // take a look if the digram is actually the one we work with...
        if (0 == ole_right_digram.compare(entry->digram)) {
          Rcout << "  ***** the old digram is like the new one, cleaning up ..." << std::endl;
          Rcout << "  *** old ext. loop: ";
          for (auto i = occurrences.begin(); i != occurrences.end(); ++i) Rcout << *i << ' ';
          Rcout << std::endl;
          occurrences.erase(std::remove(occurrences.begin(), occurrences.end(),
                       next_sym->payload->str_index), occurrences.end());
          Rcout << "  *** new ext. loop: ";
          for (auto i = occurrences.begin(); i != occurrences.end(); ++i) Rcout << *i << ' ';
          Rcout << std::endl;
        }
        digram_queue.update_digram_frequency(&ole_right_digram, new_freq);

        // if it was the last entry...
        if (0 == new_freq) {
          Rcout << "  *** since new freq is 0, cleaning up ..." << std::endl;
          digram_table.erase(ole_right_digram);
          new_digrams.erase(ole_right_digram);
        }

        // and place the new digram entry
        std::string new_right_digram = r->get_rule_string() + " " + after_second->payload->to_string();
        Rcout << "  *** the new right digram shall be : " << new_right_digram << std::endl;
        // see the new freq..
        // save the digram occurrence
        if (digram_table.find(new_right_digram) == digram_table.end()){
          std::vector<int> occurrences;
          occurrences.push_back(curr_sym->payload->str_index);
          digram_table.insert(std::make_pair(new_right_digram, occurrences));
        }else{
          digram_table[new_right_digram].push_back(curr_sym->payload->str_index);
        }

        new_digrams.emplace(new_right_digram);

      }

      // walk over the R0
      Rcout << "\nthe R0: ";
      ptr = r0[0];
      do {
        Rcout << ptr->payload->payload << " ";
        ptr = ptr->next;
      } while(ptr != nullptr);
      Rcout << std::endl;
      Rcout << "\nthe digrams table\n=================" << std::endl;
      for(std::unordered_map<std::string, std::vector<int>>::iterator it = digram_table.begin();
          it != digram_table.end(); ++it) {
        Rcout << it->first << " [";
        for (auto i = it->second.begin(); i != it->second.end(); ++i)
          Rcout << *i << ", ";
        Rcout << "]" << std::endl;
      }
      // all digrams are pushed to the queue, see those
      Rcout << "\nthe digrams queue\n=================" << std::endl;
      Rcout << digram_queue.to_string() << std::endl;

    }
    // clean up the digram we have worked with ...
    //
    digram_table.erase(entry->digram);

    // update the priority queue with new digrams ...
    //
    int i=0;
    Rcout << "  *** new digrams size " << new_digrams.size() << " : ";
    for(std::string s : new_digrams)
      Rcout << s << ' ';
    Rcout << std::endl;

    for (std::string st : new_digrams) {
      Rcout << "  *** checking on new digram " << st  << std::endl;
      if(i>2){return res;}
      if(digram_table[st].size() > 1){
        if(digram_queue.contains_digram(&st)){
          Rcout << "gotta update ... " << std::endl;
          return res;
          digram_queue.update_digram_frequency(&st, digram_table[st].size());
        } else {
          Rcout << "adding a digram ... " << std::endl;
          repair_digram* digram = new repair_digram( st, digram_table[st].size() );
          digram_queue.enqueue(digram);
        }
      }
      i++;
      if(i>2){return res;}

    }

    entry = digram_queue.dequeue();
  }

  // walk over the R0
  Rcout << "\nthe R0: ";
  ptr = r0[0];
  do {
    Rcout << ptr->payload->payload << " ";
    ptr = ptr->next;
  } while(ptr != nullptr);
  Rcout << std::endl;
  Rcout << "\nthe digrams table\n=================" << std::endl;
  for(std::unordered_map<std::string, std::vector<int>>::iterator it = digram_table.begin();
      it != digram_table.end(); ++it) {
    Rcout << it->first << " [";
    for (auto i = it->second.begin(); i != it->second.end(); ++i)
      Rcout << *i << ", ";
    Rcout << "]" << std::endl;
  }
  // all digrams are pushed to the queue, see those
  Rcout << "\nthe digrams queue\n=================" << std::endl;
  Rcout << digram_queue.to_string() << std::endl;

  return res;
}
