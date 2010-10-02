//===--- CouchWM.cpp - CouchDB as WM --------------------------------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// This class implements CouchDB-based working memory
//
//===----------------------------------------------------------------------===//

#include <cstdio>
#include <sys/stat.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>

#include "CouchWorkingMemory.hpp"

#define BUFSIZE 1024

const int CouchServerWorkingMemory::WM_UNKNOWN;
const int CouchServerWorkingMemory::WM_KNOWN;
json_t *CouchServerWorkingMemory::S_Root = NULL;

static char *S_buf = NULL;

//===----------------------------------------------------------------------===//
//
// Callback functions for CURLOPT read/write functions
//
//===----------------------------------------------------------------------===//
static size_t S_read_f( void *ptr, size_t size, size_t nmemb, void *userdata){
  size_t len = nmemb*size;
  //  printf( "In curl read function\n" );
  memcpy( ptr, userdata, len );
  return len;
}

static size_t S_write_f( void *ptr, size_t size, size_t nmemb, void *userdata){
  json_t *root;
  json_error_t error;
  size_t len = nmemb*size;

  S_buf = (char *)realloc( S_buf, len + 1 );
  memcpy( (void *)S_buf, ptr, len );
  S_buf[len] = '\0';
  //printf( "Received: %s\n", S_buf );

  root = json_loads(S_buf, &error);
  if(!root){
    fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
  }
  else{
    CouchServerWorkingMemory::S_Root = root;
    json_t *version;
    version = json_object_get( root, "wm" );
    if( json_is_null( version ) ){
      fprintf( stderr, "Working Memory is NULL!\n" );
    }
    else if( json_is_object( version ) ){
      /*
      char *s = json_dumps( version, 0 );
      printf( "Working Memory:\n%s\n", s );
      free( s );
      */
    }
  }

  free( S_buf );
  S_buf = NULL;

  return len;
}

//===----------------------------------------------------------------------===//
//
// Construction and initialization
//
//===----------------------------------------------------------------------===//
CouchServerWorkingMemory::CouchServerWorkingMemory(){
  couchURL = std::string( "http://127.0.0.1:5984" );
  couchDB = std::string( "knowcess" );
  couchDocument = std::string( "cc619f59b60377032c97271afa44d566" );
  curl = curl_easy_init();
}

CouchServerWorkingMemory::CouchServerWorkingMemory( std::string URL, std::string DB, std::string DOC ) {
  couchURL = URL;
  couchDB = DB;
  couchDocument = DOC;
  curl = curl_easy_init();
}


CouchServerWorkingMemory::~CouchServerWorkingMemory(){
  if( curl ) curl_easy_cleanup(curl);
}

void CouchServerWorkingMemory::clear(){
  // Get the latest revision of the document
  CouchServerWorkingMemory::S_Root = NULL;
  init();
  // Update it with an empty WM
  if( CouchServerWorkingMemory::S_Root ){  
    json_object_set_new( CouchServerWorkingMemory::S_Root, "agenda", json_array() );

    json_t *wm = json_object_get( CouchServerWorkingMemory::S_Root, "wm" );
    if( json_is_null( wm ) ){
      fprintf( stderr, "Eror: Working Memory is NULL!\n" );
    }
    else if( json_is_object( wm ) ){
      // Print current state
      char *s = json_dumps( wm, 0 );
      // printf( "Working Memory:\n%s\n", s );
      free( s );
      // Clear
      json_object_set_new( CouchServerWorkingMemory::S_Root, "wm", json_object() );
      update();
    }
  }
  json_decref( CouchServerWorkingMemory::S_Root );
}

//===----------------------------------------------------------------------===//
//
// curl-mediated communication with the CouchDB server
//
//===----------------------------------------------------------------------===//
void CouchServerWorkingMemory::init(){
  if( curl ){
    char text[ BUFSIZE ];
    curl_easy_reset( curl );

    // Full URL to document
    snprintf( text, BUFSIZE, "%s/%s/%s", couchURL.c_str(), couchDB.c_str(), couchDocument.c_str() );
    curl_easy_setopt(curl, CURLOPT_URL, text );

    // curl options
    struct curl_slist *slist = NULL;
    slist = curl_slist_append(slist, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &S_write_f );


    // Call CouchDB
    curl_easy_perform(curl);

    // Clean-up
    curl_slist_free_all(slist);
  }
}

void CouchServerWorkingMemory::update(){
  if( curl ){
    char text[ BUFSIZE ];
    char *s;
    curl_easy_reset( curl );

    // Full URL to document
    snprintf( text, BUFSIZE, "%s/%s/%s", couchURL.c_str(), couchDB.c_str(), couchDocument.c_str() );
    curl_easy_setopt(curl, CURLOPT_URL, text );

    // curl options
    struct curl_slist *slist = NULL;
    slist = curl_slist_append(slist, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    
    s = json_dumps( CouchServerWorkingMemory::S_Root, 0 );
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L );
    curl_easy_setopt(curl, CURLOPT_INFILESIZE, (long)strlen(s) );
    curl_easy_setopt(curl, CURLOPT_READDATA, s); 
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, &S_read_f );

    //printf( "Data: %s\n", s );

    // Call CouchDB
    curl_easy_perform(curl);

    // Clean-up
    free( s );
    curl_slist_free_all(slist);
  }
}

//===----------------------------------------------------------------------===//
//
// Presence/absence in working memory
//
//===----------------------------------------------------------------------===//
const int CouchServerWorkingMemory::knownp( const char *data ){
  // printf( "Known variable %s?\n", data );
  CouchServerWorkingMemory::S_Root = NULL;
  init();
  if( CouchServerWorkingMemory::S_Root ){
    json_t *wm = json_object_get( CouchServerWorkingMemory::S_Root, "wm" );
    json_t *sign;
    if( json_is_null( wm ) ){
      fprintf( stderr, "Eror: Working Memory is NULL!\n" );
    }
    else if( json_is_object( wm ) ){
      sign = json_object_get( wm, data );
      if( sign ){
	json_decref( CouchServerWorkingMemory::S_Root );
	return WM_KNOWN;
      }
    }   
  }
  json_decref( CouchServerWorkingMemory::S_Root );
  return WM_UNKNOWN;
}

//===----------------------------------------------------------------------===//
//
// Set commands
//
//===----------------------------------------------------------------------===//
void CouchServerWorkingMemory::set_json_t( const char *data, json_t *val ){
  CouchServerWorkingMemory::S_Root = NULL;
  init();
  if( CouchServerWorkingMemory::S_Root ){
    json_t *wm = json_object_get( CouchServerWorkingMemory::S_Root, "wm" );
    if( json_is_null( wm ) ){
      fprintf( stderr, "Eror: Working Memory is NULL!\n" );
    }
    else if( json_is_object( wm ) ){
      json_object_set_new( wm, data, val );
      update();
      json_decref( CouchServerWorkingMemory::S_Root );
      return;
    }   
  }
  json_decref( CouchServerWorkingMemory::S_Root );
}

void CouchServerWorkingMemory::set( const char *data, int val ){
  printf( "Assign value %d to variable %s.\n", val, data );
  set_json_t( data, json_integer( val ) );
}

void CouchServerWorkingMemory::set( const char *data, double val ){
  printf( "Assign value %f to variable %s.\n", val, data );
  set_json_t( data, json_real( val ) );
}

void CouchServerWorkingMemory::set( const char *data, const char *val ){
  printf( "Assign value %s to variable %s.\n", val, data );
  set_json_t( data, json_string( val ) );
}

//===----------------------------------------------------------------------===//
//
// Get commands
//
//===----------------------------------------------------------------------===//
json_t *CouchServerWorkingMemory::get_helper( const char *data ){
  json_t *field = NULL;
  CouchServerWorkingMemory::S_Root = NULL;
  init();
  if( CouchServerWorkingMemory::S_Root ){
    field = json_object_get( CouchServerWorkingMemory::S_Root, data );
  }
  return field;
}

std::string CouchServerWorkingMemory::get_assembly( std::string def ){
  json_t *field = get_helper( "assembly" );
  std::string str = field && json_is_string(field) ? 
    std::string( json_string_value(field) ) :
    def;
  json_decref( CouchServerWorkingMemory::S_Root );
  return str;
}

std::string CouchServerWorkingMemory::get_prologue( std::string def ){
  json_t *field = get_helper( "prologue" );
  std::string str = field && json_is_string(field) ? 
    std::string( json_string_value(field) ) :
    def;
  json_decref( CouchServerWorkingMemory::S_Root );
  return str;
}

json_t *CouchServerWorkingMemory::get_json_t( char *data ){
  CouchServerWorkingMemory::S_Root = NULL;
  init();
  if( CouchServerWorkingMemory::S_Root ){
    json_t *wm = json_object_get( CouchServerWorkingMemory::S_Root, "wm" );
    json_t *sign;
    if( json_is_null( wm ) ){
      fprintf( stderr, "Eror: Working Memory is NULL!\n" );
    }
    else if( json_is_object( wm ) ){
      sign = json_object_get( wm, data );
      if( sign ){
	//	json_decref( CouchServerWorkingMemory::S_Root );
	return sign;
      }
    }   
  }
  //json_decref( CouchServerWorkingMemory::S_Root );
  return NULL;
}

int CouchServerWorkingMemory::get( char *data ){
  json_t *sign = get_json_t( data );
  int ret = (sign && json_is_integer(sign)) ? json_integer_value(sign) : 0;
  json_decref( CouchServerWorkingMemory::S_Root );
  return ret;
}

double CouchServerWorkingMemory::getd( char *data ){
  json_t *sign = get_json_t( data );
  double ret = (sign && json_is_real(sign)) ? json_real_value(sign) : 0.0;
  json_decref( CouchServerWorkingMemory::S_Root );
  return ret;
}

const char * CouchServerWorkingMemory::getc( char *data ){
  json_t *sign = get_json_t( data );
  std::string s(  (sign && json_is_string(sign)) ? 
		  json_string_value(sign) : "" );
  // std::cout << "Got " << s << "\n";
  json_decref( CouchServerWorkingMemory::S_Root );
  return s.c_str();
}

std::ostream& operator<<(std::ostream& out, const CouchServerWorkingMemory& wmem){
  return out;
}
