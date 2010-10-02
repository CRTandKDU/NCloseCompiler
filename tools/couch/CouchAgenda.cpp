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

#include "CouchAgenda.hpp"

void CouchAgenda::reset(){ goals.clear(); }
bool CouchAgenda::empty() { return goals.empty(); }

std::string CouchAgenda::pop(){
  std::string s = goals.front();
  goals.pop_front();
  return s;
}

std::string CouchAgenda::top(){
  return goals.front();
}

int CouchAgenda::push( std::string goal, CouchWorkingMemory *wm ){
  // Postpone evaluation of unknown hypotheses only.
  if( CouchWorkingMemory::WM_KNOWN == wm->knownp( goal.c_str() ) )
    return 0;
  // Check for redundancy
  if( goals.end() == find( goal ) ){
    goals.push_back( goal );
    return 1;
  }
  return 0;
}

std::deque< std::string >::iterator CouchAgenda::find( std::string h ){
  std::deque< std::string >::iterator it = goals.begin();
  while( it != goals.end() ){
    if( h == *it ) return it;
    it++;
  }
  return goals.end();
}

