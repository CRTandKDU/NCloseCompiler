#include "KBWorkingMemory.hpp"
#include <cstdio>

const int WorkingMemory::WM_UNKNOWN;
const int WorkingMemory::WM_KNOWN;

const int WorkingMemory::knownp( const char *data ){
  printf( "Known variable %s?\n", data );
  if( wm_i.end() != wm_i.find( std::string( data ) ) ){
    return WM_KNOWN;
  }
  if( wm_d.end() != wm_d.find( std::string( data ) ) ){
    return WM_KNOWN;
  }
  if( wm_c.end() != wm_c.find( std::string( data ) ) ){
    return WM_KNOWN;
  }
  return WM_UNKNOWN;
}

void WorkingMemory::set( const char *data, int val ){
  printf( "Assign value %d to variable %s.\n", val, data );
  wm_i[ std::string(data) ] = val;
}

void WorkingMemory::set( const char *data, double val ){
  printf( "Assign value %f to variable %s.\n", val, data );
  wm_d[ std::string(data) ] = val;
}

void WorkingMemory::set( const char *data, char *val ){
  printf( "Assign value %s to variable %s.\n", val, data );
  wm_c[ std::string(data) ] = std::string(val);
}

int WorkingMemory::get( char *data ){
  return wm_i[ std::string(data) ];
}

double WorkingMemory::getd( char *data ){
  return wm_d[ std::string(data) ];
}

const char * WorkingMemory::getc( char *data ){
  return wm_c[ std::string(data) ].c_str();
}
