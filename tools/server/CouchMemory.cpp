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
#include <cstdio>
#include <sys/stat.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>

#include "CouchWorkingMemory.hpp"
#include "CouchMemory.hpp"

const int CouchServerMemory::getWMSize(){
  int ret = 0;
  CouchServerWorkingMemory::S_Root = NULL;
  init();
  if( CouchServerWorkingMemory::S_Root ){
    json_t *wm = json_object_get( CouchServerWorkingMemory::S_Root, "wm" );
    ret = json_object_size( wm );
  }
  json_decref( CouchServerWorkingMemory::S_Root );
  return ret;
}

void CouchServerMemory::getAgenda( CouchServerAgenda *agenda ){
  CouchServerWorkingMemory::S_Root = NULL;
  init();
  if( CouchServerWorkingMemory::S_Root ){
    json_t *hypos = json_object_get( CouchServerWorkingMemory::S_Root, "agenda" );
    if( json_is_array( hypos ) ){
      unsigned int i, n;
      n = json_array_size( hypos );
      for( i = 0; i < n; i++ ){
	agenda->push( std::string( json_string_value( json_array_get( hypos, i ) ) ) );
      }
    }
  }  
  json_decref( CouchServerWorkingMemory::S_Root );
}

void CouchServerMemory::setAgenda( CouchServerAgenda *ag ){
  CouchServerAgenda *agenda = new CouchServerAgenda( *ag );
  CouchServerWorkingMemory::S_Root = NULL;
  init();
  if( CouchServerWorkingMemory::S_Root ){
    std::cerr << "Preparing to update\n";
    json_t *hypos = json_array();
    while( !agenda->empty() ){
      json_array_append_new( hypos, json_string( agenda->pop().c_str() ) );
    }
    json_object_set_new( CouchServerWorkingMemory::S_Root, "agenda", hypos );
    update();
    json_decref( hypos );
  }
  json_decref( CouchServerWorkingMemory::S_Root );
  delete agenda;
}


