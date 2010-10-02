//===--- nkb-http.cpp - The NClose Interpreter as a Service----------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// The NClose interpreter as a, HTTP server
//
//===----------------------------------------------------------------------===//

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <microhttpd.h>

#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/CommandLine.h"

#include "CouchRunServer.hpp"
#include "nkb-http.hpp"

using namespace std;
using namespace llvm;

#define PORT 8888

#define NKBNAMESIZE     128
#define POSTBUFFERSIZE  512
#define MAXNAMESIZE     128
#define MAXANSWERSIZE   512

#define GET             0
#define POST            1

static cl::opt<string> S_AssemblyFilename("nkbAssembly", cl::desc("Specify knowledge bases assembly filename (defaults to `assembly.s')."), cl::init("assembly.s"), cl::value_desc("filename"));
static cl::opt<string> S_PrologueXMLFilename("nkbPrologue", cl::desc("<XML prologue file>"), cl::init("prologue.xml"), cl::value_desc("filename"));
static cl::opt<string> S_CouchURL("nkbCouchURL", cl::desc("Couch full URL"), cl::init("http://127.0.0.1:5984"), cl::value_desc("URL"));
static cl::opt<string> S_CouchDB("nkbCouchDB", cl::desc("Couch database"), cl::init("knowcess"), cl::value_desc("database name"));
static cl::opt<string> S_CouchDoc("nkbCouchDoc", cl::desc("Couch document"), cl::init("cc619f59b60377032c97271afa44d566"), cl::value_desc("UUID"));

const char *S_FmtRestarted = "{\"session\":\"ready\", \"id\":\"%s\"}";
const char *S_FmtRunning = "{\"session\":\"running\", \"question\":\"%s\"}";
const char *S_FmtStatus = "{\"session\":\"running\", \"id\":\"%s\", \"status\":\"%d\"}";
const char *S_FmtTerminated = "{\"session\":\"done\", \"id\":\"%s\"}";

std::string S_PendingQuestion = std::string("");

/** @brief Callback question function */
void buildQuestion( std::string question ){
  char buf[MAXANSWERSIZE];
  snprintf( buf, MAXANSWERSIZE, S_FmtRunning, question.c_str() );
  S_PendingQuestion = std::string( buf );
}

/** @brief Information passed back and forth in POST requests */
struct connection_info_struct
{
  int connectiontype;
  struct MHD_PostProcessor *postprocessor;
  char answerstring[MAXANSWERSIZE];
  char CouchURL[NKBNAMESIZE];
  char CouchDB[NKBNAMESIZE];
  char CouchDOC[NKBNAMESIZE];
  char Sign[NKBNAMESIZE];
  char Value[NKBNAMESIZE];
};


const char *askpage = "<html><body>\
                       <h2>KBVM 1.0 -- Experimental</h2>  \
                       This server expects POST requests. \
                       </body></html>";

const char *errorpage =
  "<html><body>This doesn't seem to be right.</body></html>";

void display( struct connection_info_struct * c_info ){
  printf( "URL = %s\nDB = %s\nDoc = %s\nAnswer = %s\nSign = %s\nValue = %s\n",
	  c_info->CouchURL,
	  c_info->CouchDB,
	  c_info->CouchDOC,
	  c_info->answerstring,
	  c_info->Sign,
	  c_info->Value
 );
}

/** @brief Execute NClose Service command, and uses `S_PendingQuestion' to signal and return responses in JSON format */
void do_command( struct connection_info_struct * c_info, const char *cmd ){
  char buf[MAXANSWERSIZE];
  CouchRunHttpServer *server =
    new CouchRunHttpServer( std::string(c_info->CouchURL),
			    std::string(c_info->CouchDB),
			    std::string(c_info->CouchDOC) );
  S_PendingQuestion = std::string("");
  if( 0 == strcmp( cmd, "restart" ) ){
    server->init();
    server->restart();
    snprintf( buf, MAXANSWERSIZE, S_FmtRestarted, c_info->CouchDOC );
    S_PendingQuestion = std::string( buf );
  }
  else if( 0 == strcmp( cmd, "next" ) ){
    server->next();
    if("" == S_PendingQuestion){
      snprintf( buf, MAXANSWERSIZE, S_FmtTerminated, c_info->CouchDOC );
      S_PendingQuestion = std::string( buf );
    }
  }
  else if( 0 == strcmp( cmd, "status" ) ){
    int n = server->status();
    snprintf( buf, MAXANSWERSIZE, S_FmtStatus, c_info->CouchDOC, n );
    S_PendingQuestion = std::string( buf );
  }
  else if( 0 == strcmp( cmd, "volunteer" ) ){
    if( !isalpha( c_info->Value[0] ) ){
      double d = atof( c_info->Value );
      server->volunteer( std::string(c_info->Sign), d );
    }
    else if( 0 == strcmp( "true", c_info->Value ) ){
      server->volunteer( std::string(c_info->Sign), 1 );
    }
    else if( 0 == strcmp( "false", c_info->Value ) ){
      server->volunteer( std::string(c_info->Sign), 0 );
    }
    else{
      server->volunteer( std::string(c_info->Sign), std::string(c_info->Value) );
    }
    snprintf( buf, MAXANSWERSIZE, S_FmtRestarted, c_info->CouchDOC );
    S_PendingQuestion = std::string( buf );
  }
  else if( 0 == strcmp( cmd, "suggest" ) ){
    server->suggest( std::string( c_info->Sign ) );
    snprintf( buf, MAXANSWERSIZE, S_FmtRestarted, c_info->CouchDOC );
    S_PendingQuestion = std::string( buf );
  }
  else{
  }
  delete server;
}

/** @brief Iterate through the data poset in request */
static int
iterate_post (void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
              const char *filename, const char *content_type,
              const char *transfer_encoding, const char *data, uint64_t off,
              size_t size)
{
  struct connection_info_struct *con_info = 
    (struct connection_info_struct *)coninfo_cls;

  printf( " Key = %s\n Filename = %s\n Content_type = %s\n Transfer_encoding = %s\n Data = %s\n",
	  key, filename, content_type, transfer_encoding, data );
  
  if (0 == strcmp (key, "couchurl")){
    if ((size > 0) && (size <= NKBNAMESIZE)){
      /*
      if( con_info->CouchURL ) free( con_info->CouchURL );
      con_info->CouchURL = (char *)malloc( size );
      */
      strncpy( con_info->CouchURL, data, NKBNAMESIZE );
    }
  }
  if (0 == strcmp (key, "couchdb")){
    if ((size > 0) && (size <= NKBNAMESIZE)){
      /*
      if( con_info->CouchDB ) free( con_info->CouchDB );
      con_info->CouchDB = (char *)malloc( size );
      */
      strncpy( con_info->CouchDB, data, NKBNAMESIZE );
    }
  }
  if (0 == strcmp (key, "couchdoc")){
    if ((size > 0) && (size <= NKBNAMESIZE)){
      /*
      if( con_info->CouchDOC ) free( con_info->CouchDOC );
      con_info->CouchDOC = (char *)malloc( size );
      */
      strncpy( con_info->CouchDOC, data, NKBNAMESIZE );
    }
  }
  if (0 == strcmp (key, "sign")){
    if ((size > 0) && (size <= NKBNAMESIZE)){
      strncpy( con_info->Sign, data, NKBNAMESIZE );
    }
  }
  if (0 == strcmp (key, "value")){
    if ((size > 0) && (size <= NKBNAMESIZE)){
      strncpy( con_info->Value, data, NKBNAMESIZE );
    }
  }
  else if (0 == strcmp (key, "command")){
    if ((size > 0) && (size <= MAXNAMESIZE)){
      display( con_info );
      do_command( con_info, data );
      if( "" != S_PendingQuestion ){
	strncpy( con_info->answerstring, 
		 S_PendingQuestion.c_str(), MAXANSWERSIZE );
      }
 
    }
 
    
    return MHD_NO;
  }
  
  return MHD_YES;
}

/** @brief Create response from string and return it to the browser */
static int
send_page (struct MHD_Connection *connection, const char *page)
{
  int ret;
  struct MHD_Response *response;
  response =
    MHD_create_response_from_data (strlen (page), (void *) page, MHD_NO,
                                   MHD_NO);
  if (!response)
    return MHD_NO;
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);
  return ret;
}

/** @brief The HTTP request handling entry point */
static int
answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
  // First called with a NULL connection info
  if (NULL == *con_cls){
    struct connection_info_struct *con_info;
    con_info = 
      (struct connection_info_struct *)malloc (sizeof (struct connection_info_struct));
    if (NULL == con_info) return MHD_NO;

    con_info->answerstring[0] = 0;
    if (0 == strcmp (method, "POST")){
      con_info->postprocessor =
	MHD_create_post_processor (connection, POSTBUFFERSIZE,
				   iterate_post, (void *) con_info);  
      if (NULL == con_info->postprocessor){
	free (con_info);
	return MHD_NO;
      }
      /*
	con_info->CouchURL = (char *)malloc( S_CouchURL.size() );
	con_info->CouchDB = (char *)malloc( S_CouchDB.size() );
	con_info->CouchDOC = (char *)malloc( S_CouchDoc.size() );
      */
      strncpy( con_info->CouchURL, S_CouchURL.c_str(), MAXNAMESIZE );
      strncpy( con_info->CouchDB, S_CouchDB.c_str(), MAXNAMESIZE );
      strncpy( con_info->CouchDOC, S_CouchDoc.c_str(), MAXNAMESIZE );
      con_info->Sign[0] = con_info->Value[0] = (char)0;
      con_info->connectiontype = POST;
    }
    else
      con_info->connectiontype = GET;
    
    *con_cls = (void *) con_info;
    
    return MHD_YES;
  }

  // Process requests
  if (0 == strcmp (method, "GET")){
    return send_page (connection, askpage);
  }

  if (0 == strcmp (method, "POST")){
    struct connection_info_struct *con_info = 
      (struct connection_info_struct *)*con_cls;
    
    if (*upload_data_size != 0){
      MHD_post_process (con_info->postprocessor, upload_data,
			*upload_data_size);
      *upload_data_size = 0;
      
      return MHD_YES;
    }
    else if ( 0 != con_info->answerstring[0] ){   
      return send_page (connection, con_info->answerstring);
    }
  }
  
  return send_page (connection, errorpage);
}

/** @brief The cleanup function */
static void
request_completed (void *cls, struct MHD_Connection *connection,
                   void **con_cls, enum MHD_RequestTerminationCode toe)
{
  struct connection_info_struct *con_info = 
    (struct connection_info_struct *)*con_cls;

  if (NULL == con_info)
    return;

  if (con_info->connectiontype == POST){
    MHD_destroy_post_processor (con_info->postprocessor);
    /*
    if (con_info->CouchURL) free (con_info->CouchURL);
    if (con_info->CouchDB ) free (con_info->CouchDB);
    if (con_info->CouchDOC) free (con_info->CouchDOC);
    if (con_info->Sign ) free (con_info->Sign);
    if (con_info->Value) free (con_info->Value);
    */
  }

  free (con_info);
  *con_cls = NULL;
}

int
main ( int argc, char *argv[] )
{
  struct MHD_Daemon *daemon;
  cl::ParseCommandLineOptions(argc, argv);

  daemon = 
    MHD_start_daemon (
		      MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
		      &answer_to_connection, NULL,
		      MHD_OPTION_NOTIFY_COMPLETED, request_completed, NULL,
		      MHD_OPTION_CONNECTION_MEMORY_LIMIT, 2*32*1024,
		      MHD_OPTION_END);

  if (NULL == daemon)
    return 1;

  printf( "KBVM v1.0 -- Http Server.\n(Press <Return> to exit.)\n" );

  getchar ();

  MHD_stop_daemon (daemon);
  return 0;
}
