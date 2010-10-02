//===--- CouchMemory.cpp - The NClose interpreter -------------------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// This class implements a Couch-based memory for the NClose interpreter
//
//===----------------------------------------------------------------------===//
#if !defined(KBVM_INCLUDE_GUARD_COUCHMEM_HPP)
#define KBVM_INCLUDE_GUARD_COUCHMEM_HPP
#include <cstdio>
#include <ostream>
#include <iostream>
#include <string>
#include <map>
#include <curl/curl.h>
#include <jansson.h>

#include "../runner/KBWorkingMemory.hpp"
#include "CouchWorkingMemory.hpp"
#include "CouchAgenda.hpp"

/** @brief @b Interpreter Persists agenda, working memory and encyclopedia to CouchDB datastore */
class CouchHttpMemory: public CouchHttpWorkingMemory {
public:
  /** Constructors */
  CouchHttpMemory() : CouchHttpWorkingMemory(){};
  CouchHttpMemory( std::string URL, std::string DB, std::string DOC ):
    CouchHttpWorkingMemory(  URL,  DB,  DOC ){};
  /** Destructor */
  ~CouchHttpMemory(){};

  /** Assigns boolean value to sign
      @param data The piece of evidence
      @param val The assigned value (1 for true, 0 for false )
   */
  void sets( const char *data, int val );

  /** Assigns double value to sign
      @param data The piece of evidence
      @param val The assigned value
   */
  void sets( const char *data, double val );
  /** Assigns string value to sign
      @param data The piece of evidence
      @param val The assigned value
   */
  void sets( const char *data, const char *val );

  /** Assigns boolean value to hypothesis
      @param data The piece of evidence
      @param val The assigned value (1 for true, 0 for false )
   */
  void seth( const char *data, int val );

  /** Get the latest agenda from Couch document */
  void getAgenda( CouchHttpAgenda *agenda );
  /** Updates Couch document with current agenda */
  void setAgenda( CouchHttpAgenda *agenda );

  /** Get the current working memory size */
  const int getWMSize();

  /** Get the current encyclopedia 
      @param map_h On entry an empty map for hypothesis/value pairs
      @param map_s On entry an empty map for signs/value pairs
   */
  void getEncyclopedia( std::map<std::string, std::string> *map_h, 
			std::map<std::string, std::string> *map_s );
private:
  /** Updates CouchDB document with inferred hypothesis */
  void set_json_t_hypo( const char *data, json_t *val );
  /** Updates CouchDB document with signs asked for */
  void set_json_t_sign( const char *data, json_t *val );
};

#endif



