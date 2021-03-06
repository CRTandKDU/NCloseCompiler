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

bool member( const char *data, const json_t *array ){
  unsigned int i, n = json_array_size( array );
  for( i = 0; i < n; i++ ){
    if( 0 == strcmp( data, json_string_value(json_array_get(array,i)) ) ){
      return true;
    }
  }
  return false;
}

void CouchHttpMemory::set_json_t_hypo( const char *data, json_t *val ){
  CouchHttpWorkingMemory::S_Root = NULL;
  init();
  if( CouchHttpWorkingMemory::S_Root ){
    json_t *wm = json_object_get( CouchHttpWorkingMemory::S_Root, "wm" );
    json_t *hypos = json_object_get( CouchHttpWorkingMemory::S_Root, "hypotheses" );
    if( !json_is_null(wm) && !json_is_null(hypos) &&
	json_is_object(wm) && json_is_array(hypos) ){
      json_object_set_new( wm, data, val );
      if( !member( data, hypos ) )
	json_array_append_new( hypos, json_string(data) );
      update();
    }
  }
  json_decref( CouchHttpWorkingMemory::S_Root );
}

void CouchHttpMemory::set_json_t_sign( const char *data, json_t *val ){
  CouchHttpWorkingMemory::S_Root = NULL;
  init();
  if( CouchHttpWorkingMemory::S_Root ){
    json_t *wm = json_object_get( CouchHttpWorkingMemory::S_Root, "wm" );
    json_t *signs = json_object_get( CouchHttpWorkingMemory::S_Root, "signs" );
    if( !json_is_null(wm) && !json_is_null(signs) &&
	json_is_object(wm) && json_is_array(signs) ){
      json_object_set_new( wm, data, val );
      if( !member( data, signs ) )
	json_array_append_new( signs, json_string(data) );
      update();
    }
  }
  json_decref( CouchHttpWorkingMemory::S_Root );
}

void CouchHttpMemory::seth( const char *data, int val ){
  printf( "Assign boolean value %d to hypothesis %s.\n", val, data );
  set_json_t_hypo( data, json_integer( val ) );
}

void CouchHttpMemory::sets( const char *data, int val ){
  printf( "Assign boolean value %d to sign %s.\n", val, data );
  set_json_t_sign( data, json_integer( val ) );
}

void CouchHttpMemory::sets( const char *data, double val ){
  printf( "Assign value %f to sign %s.\n", val, data );
  set_json_t_sign( data, json_real( val ) );
}

void CouchHttpMemory::sets( const char *data, const char *val ){
  printf( "Assign value %s to sign %s.\n", val, data );
  set_json_t_sign( data, json_string( val ) );
}

const int CouchHttpMemory::getWMSize(){
  int ret = 0;
  CouchHttpWorkingMemory::S_Root = NULL;
  init();
  if( CouchHttpWorkingMemory::S_Root ){
    json_t *wm = json_object_get( CouchHttpWorkingMemory::S_Root, "wm" );
    ret = json_object_size( wm );
  }
  json_decref( CouchHttpWorkingMemory::S_Root );
  return ret;
}

void CouchHttpMemory::getEncyclopedia( std::map<std::string, std::string> *map_h, std::map<std::string, std::string> *map_s ){
  unsigned int i, n;
  json_t *wm_elt;
  std::string s;

  CouchHttpWorkingMemory::S_Root = NULL;
  init();
  if( CouchHttpWorkingMemory::S_Root ){
    json_t *wm = json_object_get( CouchHttpWorkingMemory::S_Root, "wm" );
    json_t *hypos = json_object_get( CouchHttpWorkingMemory::S_Root, "hypotheses" );
    json_t *signs = json_object_get( CouchHttpWorkingMemory::S_Root, "signs" );
    if( json_is_object(wm) && 
	json_is_array( hypos ) && json_is_array(signs) ){
      // Map hypotheses
      n = json_array_size( hypos );
      for( i = 0; i < n; i++ ){
	wm_elt = 
	  json_object_get( wm, json_string_value( json_array_get(hypos,i) ) );
	(*map_h)[ std::string(json_string_value( json_array_get(hypos,i) ) ) ] =
	  1 == json_integer_value( wm_elt ) ? std::string("TRUE") : std::string("FALSE");
      }
      // Map signs
      n = json_array_size( signs );
      for( i = 0; i < n; i++ ){
	wm_elt = 
	  json_object_get( wm, json_string_value( json_array_get(signs,i) ) );
	if( json_is_integer(wm_elt) ){
	  s = (1 == json_integer_value( wm_elt ) ) ? std::string("TRUE") 
	    : std::string("FALSE");
	}
	else if( json_is_real(wm_elt) ){
	  char buf[9];
	  snprintf( buf, 9, "%.2f", json_real_value( wm_elt ) );
	  s = std::string( buf );
	}
	else{
	  s = std::string( json_string_value( wm_elt ) );
	}
	(*map_s)[ std::string(json_string_value( json_array_get(signs,i) ) ) ] = s;
      }
    }
  }  
  json_decref( CouchHttpWorkingMemory::S_Root );
}
  


void CouchHttpMemory::getAgenda( CouchHttpAgenda *agenda ){
  CouchHttpWorkingMemory::S_Root = NULL;
  init();
  if( CouchHttpWorkingMemory::S_Root ){
    json_t *hypos = json_object_get( CouchHttpWorkingMemory::S_Root, "agenda" );
    if( json_is_array( hypos ) ){
      unsigned int i, n;
      n = json_array_size( hypos );
      for( i = 0; i < n; i++ ){
	agenda->push( std::string( json_string_value( json_array_get( hypos, i ) ) ) );
      }
    }
  }  
  json_decref( CouchHttpWorkingMemory::S_Root );
}

void CouchHttpMemory::setAgenda( CouchHttpAgenda *ag ){
  CouchHttpAgenda *agenda = new CouchHttpAgenda( *ag );
  CouchHttpWorkingMemory::S_Root = NULL;
  init();
  if( CouchHttpWorkingMemory::S_Root ){
    std::cerr << "Preparing to update\n";
    json_t *hypos = json_array();
    while( !agenda->empty() ){
      json_array_append_new( hypos, json_string( agenda->pop().c_str() ) );
    }
    json_object_set_new( CouchHttpWorkingMemory::S_Root, "agenda", hypos );
    update();
    json_decref( hypos );
  }
  json_decref( CouchHttpWorkingMemory::S_Root );
  delete agenda;
}


