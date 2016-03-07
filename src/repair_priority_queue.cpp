#include <repair.h>
using namespace Rcpp ;


repair_digram* repair_priority_queue::enqueue(repair_digram* digram) {

  if (nodes.find(digram->digram) != nodes.end()) {
    throw std::range_error("Inadmissible value, key exists...");
    return nullptr;
  }

  // create a new node
  repair_pqueue_node nn(digram);

  // place it into the queue if it's empty
  if (nullptr == head) {
      head = &nn;
  }

  // if new node has lesser or equal than head frequency... this going to be the new head
  else if (nn.payload->freq >= head->payload->freq) {
    head->prev = &nn;
    nn.next = head;
    head = &nn;
  }
  // in all other cases find a good place in the existing queue, starting from the head
  else {
    repair_pqueue_node* curr_node = head;
    while (nullptr != curr_node->next) {
      //
      // if current node has lesser or equal frequency... it should be after nn
      //
      // remember we are pointing on the second node inside this loop or onto second to tail
      // node
      //
      if (nn.payload->freq >= curr_node->payload->freq) {
        repair_pqueue_node* prev_node = curr_node->prev;
          prev_node->next = &nn;
          nn.prev = prev_node;
          curr_node->prev = &nn;
          nn.next = curr_node;
          break; // the element has been placed
        }
        curr_node = curr_node->next;
      }
      // check if loop was broken by condition, not by placement
      if (nullptr == curr_node->next) {
        // so, currentNode points on the tail...
        if (nn.payload->freq >= curr_node->payload->freq) {
          // insert just before...
          repair_pqueue_node* prev_node = curr_node->prev;
          prev_node->next = &nn;
          nn.prev = prev_node;
          curr_node->prev = &nn;
          nn.next = curr_node;
        }
        else {
          // or make a new tail
          nn.prev = curr_node;
          curr_node->next = &nn;
        }
      }

  }
  // also save the element in the index store
  nodes.emplace(nn.payload->digram, &nn);
  return nn.payload;
}

repair_digram* repair_priority_queue::dequeue() {
  if(NULL != head){
    nodes.erase(head->payload->digram);
    repair_pqueue_node* res = head;
    head = head->next;
    if(NULL != head){
      head->prev = NULL;
    }
    return res->payload;
  }
  return nullptr;
}

std::vector<repair_digram> repair_priority_queue::to_array() {
  std::vector<repair_digram> res;
  repair_pqueue_node* cp = head;
  while(NULL != cp->next){
    res.push_back(*cp->payload);
  }
  return res;
}
