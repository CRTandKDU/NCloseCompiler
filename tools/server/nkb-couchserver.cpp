//===--- nkb-couchserver.cpp - The NClose IR runner --- -------------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// The NClose interpreter as a server
//
//===----------------------------------------------------------------------===//
// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/CommandLine.h"


#include "CouchRunServer.hpp"
#include "nkb-couchserver.hpp"

using namespace std;

static cl::opt<string> S_AssemblyFilename("nkbAssembly", cl::desc("Specify knowledge bases assembly filename (defaults to `assembly.s')."), cl::init("assembly.s"), cl::value_desc("filename"));
static cl::opt<string> S_PrologueXMLFilename("nkbPrologue", cl::desc("<XML prologue file>"), cl::init("prologue.xml"), cl::value_desc("filename"));
static cl::opt<string> S_CouchURL("nkbCouchURL", cl::desc("Couch full URL"), cl::init("http://127.0.0.1:5984"), cl::value_desc("URL"));
static cl::opt<string> S_CouchDB("nkbCouchDB", cl::desc("Couch database"), cl::init("knowcess"), cl::value_desc("database name"));
static cl::opt<string> S_CouchDoc("nkbCouchDoc", cl::desc("Couch document"), cl::init("cc619f59b60377032c97271afa44d566"), cl::value_desc("UUID"));
static cl::opt<string> S_Message (cl::Positional, cl::desc("<message>"), cl::Required);
cl::list<string>  S_Argv(cl::ConsumeAfter, cl::desc("<message arguments>..."));

int main( int argc, char* argv[] ){
  cl::ParseCommandLineOptions(argc, argv);
  // We need exception handling in LLVM compiled code.
  // Pending question returns control to interpreter via throwing exceptions.
  llvm::JITExceptionHandling = true;

  CouchRunService *server = 
    new CouchRunService( S_CouchURL, S_CouchDB, S_CouchDoc );
  
  if( 0 == S_Message.compare( "restart" ) ){
    server->restart();
  }
  else if( 0 == S_Message.compare( "status" ) ){
    int state = server->status();
    std::cout << "Status: " << state << std::endl;
  }
  else if( 0 == S_Message.compare( "next" ) ){
    server->next();
  }
  else if( 0 == S_Message.compare( "suggest" ) ){
    if( 1 != S_Argv.size() ){
      std::cerr << "Usage: nkb-couchserver suggest <hypo>\n";
      return 1;
    }
    server->suggest( std::string( S_Argv[0] ) );
  }
  else if( 0 == S_Message.compare( "volunteer" ) ){
    if( 2 != S_Argv.size() ){
      std::cerr << "Usage: nkb-couchserver volunteer <sign> <value>\n";
      return 1;
    }
    std::string sign( S_Argv[0] );
    std::string val( S_Argv[1] );
    char c = val[0];
    if(  0 == val.compare( "true" ) ){
      server->volunteer( sign, 1 );
    }
    else if(  0 == val.compare( "false" ) ){
      server->volunteer( sign, 0 );
    }
    else if( isalpha( c ) ){
      server->volunteer( sign, val );
    }
    else{
      double d = atof( val.c_str() );
      server->volunteer( sign, d );
    }
  }
  else{
  }
  
  delete server;
  return 0;
}
