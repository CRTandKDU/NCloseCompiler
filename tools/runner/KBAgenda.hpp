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
#if !defined(KBVM_INCLUDE_GUARD_KBAgenda_HPP)
#define KBVM_INCLUDE_GUARD_KBAgenda_HPP

#include <string>
#include <deque>

#include "KBWorkingMemory.hpp"

/** The Agenda class is used for handling forward chaining in this NClose algorithm implementation.  Each goal evaluation function collects further goals for later evaluation as sign values are collected.  This simple implementation uses a deque.
    @brief @b Interpreter Agenda basic in-memory implementation
 */
class Agenda {
public:
  /** Constructors   */
  Agenda(){};
  Agenda( const Agenda& ag ){ goals = std::deque< std::string >(ag.goals); }
  /** Destructor   */
  ~Agenda(){};

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
  int push( std::string goal, WorkingMemory *wm );

  /** Printing out an agenda to a stream
   */
  friend std::ostream& operator<<(std::ostream& out, const Agenda& ag) 
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

