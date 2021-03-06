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
#if !defined(KBVM_INCLUDE_GUARD_CouchServerAgenda_HPP)
#define KBVM_INCLUDE_GUARD_CouchServerAgenda_HPP

#include <string>
#include <deque>

#include "../runner/KBAgenda.hpp"
#include "CouchWorkingMemory.hpp"

/** The Agenda class is used for handling forward chaining in this NClose algorithm implementation.  Each goal evaluation function collects further goals for later evaluation as sign values are collected.  This simple implementation uses a deque.
    @brief @b Interpreter Agenda persistence in CouchDB datastore
 */
class CouchServerAgenda : public Agenda{
public:
  /** Constructor
   */
  CouchServerAgenda(){};
  CouchServerAgenda( const CouchServerAgenda& ag ) : Agenda() { goals = std::deque< std::string >(ag.goals); }
  /** Destructor
   */
  ~CouchServerAgenda(){};

  /** Empty the agenda
   */
  void reset();
  /** Test for emptiness
   */
  bool empty();
  /** Get and pop out top goal
   */
  std::string pop();
  /** Get top goal
   */
  std::string top();
  /** Post a goal for deferred evaluation
   */
  int push( std::string goal, CouchServerWorkingMemory *wm );
  /** Post a goal without checking presence
   */
  int push( std::string goal ){
    goals.push_back( goal );
    return 1;
  };
  /** Returns number of hypos in agenda
   */
  const int size(){ return goals.size(); };

  /** Printing out an agenda to a stream
   */
  friend std::ostream& operator<<(std::ostream& out, const CouchServerAgenda& ag) 
  {
    out << "Agenda" << std::endl;
    std::deque< std::string >::const_iterator iter;
    for( iter = ag.goals.begin(); iter != ag.goals.end(); iter++ ){
      out << *iter << std::endl;
    }
    return out;
  };

  
private:
  std::deque< std::string > goals;

  /** Set membership testing
   */
  std::deque< std::string >::iterator find( std::string h );
  
};

#endif

