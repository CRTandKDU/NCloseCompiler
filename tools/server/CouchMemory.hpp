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

/** @brief @b Interpreter Implements agenda and working memory persistence to CouchDB */
class CouchServerMemory: public CouchServerWorkingMemory {
public:
  /** Constructors */
  CouchServerMemory() : CouchServerWorkingMemory(){};
  CouchServerMemory( std::string URL, std::string DB, std::string DOC ):
    CouchServerWorkingMemory(  URL,  DB,  DOC ){};
  /** Destructor */
  ~CouchServerMemory(){};

  /** Get the latest agenda from Couch document */
  void getAgenda( CouchServerAgenda *agenda );
  /** Updates Couch document with current agenda */
  void setAgenda( CouchServerAgenda *agenda );
  /** Get the current working memory size */
  const int getWMSize();
};

#endif



