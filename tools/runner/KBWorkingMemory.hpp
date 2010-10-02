//===--- KBWorkingMemory.cpp - The NClose interpreter WM -------------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// This class implements a default working memory for the NClose interpreter
//
//===----------------------------------------------------------------------===//
#if !defined(KBVM_INCLUDE_GUARD_WM_HPP)
#define KBVM_INCLUDE_GUARD_WM_HPP
#include <cstdio>
#include <ostream>
#include <iostream>
#include <string>
#include <map>

/** The working memory keeps track of signs and their values at runtime.  It is queried from the conditions of rules and from the agenda (in the case of hypotheses before they are effectively posted).  Its structure is linked to the patterns of test operators in LHSes, i.e. the data type system.  For instance, a RDBMS-based working memory would require some form of SQL expressions in the rules LHSes.  In the default implementation -- which has no support for objects and classes -- there are three scalar types: boolean, float and strings.  Each type comes with its set of type-specific test operators. (Note: the implementation needs a plug-in mechanism for making it easy to expand the LHS language with new types and operators.) In order to keep the default implementation simple, each type has its own sub-working-memory, each one implemented as a map from data to (typed) value.
    @brief @b Interpreter Basic in-memory working memory implementation
 */ 
class WorkingMemory{
public:
  static const int WM_KNOWN = 1;
  static const int WM_UNKNOWN = 0;

  /** @brief  Constructors */
  WorkingMemory(){};
  /** Destructor */
  ~WorkingMemory(){};

  /** @brief Query for known data.
      @param data The piece of evidence to be queried for
      @return 1 if known, 0 if unknown
  */
  const int knownp( const char *data );

  /** @brief Assigns integer value to boolean sign, including goals
      @param data The piece of evidence
      @param val The assigned value
   */
  void set( const char *data, int val );
  /** @brief Assigns double value to numerical sign
      @param data The piece of evidence
      @param val The assigned value
   */
  void set( const char *data, double val );
  /** @brief Assigns string value to string sign
      @param data The piece of evidence
      @param val The assigned value
   */
  void set( const char *data, char *val );
  

  /** @brief Get value for an integer data
      @param data The piece of evidence
      @return  The current value
   */
  int get( char *data );
  
  /** @brief Get value for a double-typed data
      @param data The piece of evidence
      @return The current double value
   */
  double getd( char *data );
  
  /** @brief Get value for a string-typed data
      @param data The piece of evidence
      @return The current value
   */
  const char *getc( char *data );
  
  /** @brief Print a working memory to an output stream */
  friend std::ostream& operator<<(std::ostream& out, const WorkingMemory& wmem) // output
  {
    std::map< std::string, int > mem = wmem.wm_i;
    std::map< std::string, int >::const_iterator iter;
    out << "Working Memory.  Current state, Booleans:" << std::endl;
    for( iter = mem.begin(); iter != mem.end(); iter++ ){
      out << iter->first << ": " << iter->second << std::endl;
    }

    std::map< std::string, double > memd = wmem.wm_d;
    std::map< std::string, double >::const_iterator iterd;
    out << "Working Memory.  Current state, Numbers:" << std::endl;
    for( iterd = memd.begin(); iterd != memd.end(); iterd++ ){
      out << iterd->first << ": " << iterd->second << std::endl;
    }

    std::map< std::string, std::string > memc = wmem.wm_c;
    std::map< std::string, std::string >::const_iterator iterc;
    out << "Working Memory.  Current state, Chars:" << std::endl;
    for( iterc = memc.begin(); iterc != memc.end(); iterc++ ){
      out << iterc->first << ": " << iterc->second << std::endl;
    }
    return out;
  };
  
  /** The "integer" sub working memory as a map */
  std::map< std::string, int > wm_i;
  /** The "double" sub working memory as a map */
  std::map< std::string, double > wm_d;  
  /** The "string" sub working memory as a map */
  std::map< std::string, std::string > wm_c;

};

#endif
