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

void CouchServerAgenda::reset(){ goals.clear(); }
bool CouchServerAgenda::empty() { return goals.empty(); }

std::string CouchServerAgenda::pop(){
  std::string s = goals.front();
  goals.pop_front();
  return s;
}

std::string CouchServerAgenda::top(){
  return goals.front();
}

int CouchServerAgenda::push( std::string goal, CouchServerWorkingMemory *wm ){
  // Postpone evaluation of unknown hypotheses only.
  if( CouchServerWorkingMemory::WM_KNOWN == wm->knownp( goal.c_str() ) )
    return 0;
  // Check for redundancy
  if( goals.end() == find( goal ) ){
    goals.push_back( goal );
    return 1;
  }
  return 0;
}

std::deque< std::string >::iterator CouchServerAgenda::find( std::string h ){
  std::deque< std::string >::iterator it = goals.begin();
  while( it != goals.end() ){
    if( h == *it ) return it;
    it++;
  }
  return goals.end();
}

