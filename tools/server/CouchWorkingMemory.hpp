//===--- CouchWorkingMemory.cpp - The NClose interpreter -------------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// This class implements a Couch-based working memory for the NClose interpreter
//
//===----------------------------------------------------------------------===//
#if !defined(KBVM_INCLUDE_GUARD_COUCHWM_HPP)
#define KBVM_INCLUDE_GUARD_COUCHWM_HPP
#include <cstdio>
#include <ostream>
#include <iostream>
#include <string>
#include <map>
#include <curl/curl.h>
#include <jansson.h>

#include "../runner/KBWorkingMemory.hpp"
/** @brief @b Interpreter Implements working memory persistence to CouchDB datastore */
class CouchServerWorkingMemory : public WorkingMemory {
public:
  static const int WM_KNOWN = 1;
  static const int WM_UNKNOWN = 0;

  /** Constructors */
  CouchServerWorkingMemory();
  CouchServerWorkingMemory( std::string URL, std::string DB, std::string DOC );
  /** Destructor */
  ~CouchServerWorkingMemory();

  /** Check if sign is already known
      @param data The piece of evidence, known or unknwown
      @return 1 if known, 0 if unknown
  */
  const int knownp( const char *data );

  /** Assigns integer value to data
      @param data The piece of evidence
      @param val The assigned value
   */
  void set( const char *data, int val );
  /** Assigns double value to data
      @param data The piece of evidence
      @param val The assigned value
   */
  void set( const char *data, double val );
  /** Assigns string value to data
      @param data The piece of evidence
      @param val The assigned value
   */
  void set( const char *data, const char *val );

  /** Get value for an integer data
      @param data The piece of evidence
      @return  The current value
   */
  int get( char *data );
  
  /** Get value for a double-typed data
      @param data The piece of evidence
      @return The current double value
   */
  double getd( char *data );
  
  /** Get value for a string-typed data
      @param data The piece of evidence
      @return The current value
   */
  const char *getc( char *data );
  
  /** Print a working memory to an output stream */
  friend std::ostream& operator<<(std::ostream& out, const WorkingMemory& wmem);  
  /** Clear working memory */
  void clear();
  /** Retrieve last revision of working memory document from CouchDB */
  void init();
  /** Update and commit changes to working memory */
  void update();

  /** Helper to the assembly source file */
  std::string get_assembly( std::string );
  /** Helper to the prologue XML file */
  std::string get_prologue( std::string );

  static json_t *S_Root;

private:

  std::string couchURL;
  std::string couchDB;
  std::string couchDocument;
  CURL *curl;

  /** Updates CouchDB document with typed data */
  void set_json_t( const char *data, json_t *val );
  /** Reads CouchDB document for typed data */
  json_t *get_json_t( char *data );
  /** Helper to access 1st level objects in CouchDB document */
  json_t *get_helper( const char *data );
};

#endif
