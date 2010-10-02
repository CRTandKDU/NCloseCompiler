//===--- KBCouchAgenda.cpp - The NClose interpreter agenda ----------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// This class implements a CouchDB-based Agenda for the NClose interpreter
//
//===----------------------------------------------------------------------===//
#if !defined(KBVM_INCLUDE_GUARD_CouchAgenda_HPP)
#define KBVM_INCLUDE_GUARD_CouchAgenda_HPP

#include <string>
#include <deque>

#include "../runner/KBAgenda.hpp"
#include "CouchWorkingMemory.hpp"

/** The CouchAgenda class is used to handle forward chaining in this NClose interpreter implementation.  It used the `deque' C++ conteiner.
    @brief @b Interpreter In-memory Agenda implementation (no different from Agenda)
 */
class CouchAgenda : public Agenda{
public:
  /** Constructor
   */
  CouchAgenda(){};
  CouchAgenda( const CouchAgenda& ag ) : Agenda() { goals = std::deque< std::string >(ag.goals); }
  /** Destructor
   */
  ~CouchAgenda(){};

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
  int push( std::string goal, CouchWorkingMemory *wm );
  /** Post a goal without checking it is already present in agenda
   */
  int push( std::string goal ){
    goals.push_back( goal );
    return 1;
  };

  /** @brief Printing out an agenda to a stream
   */
  friend std::ostream& operator<<(std::ostream& out, const CouchAgenda& ag) 
  {
    out << "Agenda" << std::endl;
    std::deque< std::string >::const_iterator iter;
    for( iter = ag.goals.begin(); iter != ag.goals.end(); iter++ ){
      out << *iter << std::endl;
    }
    return out;
  };

  
private:
  /** In-memory implementation of agenda */
  std::deque< std::string > goals;

  /** Set membership testing
   */
  std::deque< std::string >::iterator find( std::string h );
  
};

#endif

