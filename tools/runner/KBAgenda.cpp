//===--- KBAgenda.cpp - The NClose interpreter ----------------------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// This class implements a simple Agenda for the NClose algorithm
//
//===----------------------------------------------------------------------===//

#include "KBAgenda.hpp"

void Agenda::reset(){ goals.clear(); }
bool Agenda::empty() { return goals.empty(); }

std::string Agenda::pop(){
  std::string s = goals.front();
  goals.pop_front();
  return s;
}

std::string Agenda::top(){
  return goals.front();
}

int Agenda::push( std::string goal, WorkingMemory *wm ){
  // Postpone evaluation of unknown hypotheses only.
  if( WorkingMemory::WM_KNOWN == wm->knownp( goal.c_str() ) )
    return 0;
  // Check for redundancy
  if( goals.end() == find( goal ) ){
    goals.push_back( goal );
    return 1;
  }
  return 0;
}

std::deque< std::string >::iterator Agenda::find( std::string h ){
  std::deque< std::string >::iterator it = goals.begin();
  while( it != goals.end() ){
    if( h == *it ) return it;
    it++;
  }
  return goals.end();
}

