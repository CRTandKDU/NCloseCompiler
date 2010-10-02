//===--- nkb-couchgtk.cpp - The GTK NClose Interpreter --------------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// A GTK front-end to the NClose Services
//
//===----------------------------------------------------------------------===//
// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------

// Xerces 3.1.1
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/parsers/AbstractDOMParser.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMImplementationRegistry.hpp>
#include <xercesc/dom/DOMLSParser.hpp>
#include <xercesc/dom/DOMException.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMText.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMError.hpp>
#include <xercesc/dom/DOMLocator.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMAttr.hpp>

// LLVM
#include "llvm/Support/raw_ostream.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Constants.h"
#include "../../llvm/include/llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Target/TargetData.h"
#include "../../llvm/include/llvm/Target/TargetSelect.h"
#include "llvm/Transforms/Scalar.h"
#include "../../llvm/include/llvm/Support/IRBuilder.h"
//#include "llvm/ModuleProvider.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"

#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Assembly/Parser.h"
#include "llvm/Support/SourceMgr.h"

// Std
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <setjmp.h>

#if defined(XERCES_NEW_IOSTREAMS)
#include <fstream>
#else
#include <fstream.h>
#endif

#include "../sample/nkb-compile.hpp"
#include "nkb-couchgtk.hpp"

using namespace std;
using namespace llvm;

// ---------------------------------------------------------------------------
//  DOMCountHandlers: Overrides of the DOM ErrorHandler interface
// ---------------------------------------------------------------------------
DOMCountErrorHandler::DOMCountErrorHandler() : fSawErrors(false){}

DOMCountErrorHandler::~DOMCountErrorHandler(){}

bool DOMCountErrorHandler::handleError(const DOMError& domError){
  fSawErrors = true;
  if (domError.getSeverity() == DOMError::DOM_SEVERITY_WARNING)
    XERCES_STD_QUALIFIER cerr << "\nWarning at file ";
  else if (domError.getSeverity() == DOMError::DOM_SEVERITY_ERROR)
    XERCES_STD_QUALIFIER cerr << "\nError at file ";
  else
    XERCES_STD_QUALIFIER cerr << "\nFatal Error at file ";
  
  XERCES_STD_QUALIFIER cerr << StrX(domError.getLocation()->getURI())
			    << ", line " << domError.getLocation()->getLineNumber()
			    << ", char " << domError.getLocation()->getColumnNumber()
			    << "\n  Message: " << StrX(domError.getMessage()) << XERCES_STD_QUALIFIER endl;
  
  return true;
}

void DOMCountErrorHandler::resetErrors(){ fSawErrors = false; }


// ---------------------------------------------------------------------------
//  
// ---------------------------------------------------------------------------


#include "CouchAgenda.hpp"
#include "CouchMemory.hpp"
#include "CouchRunServer.hpp"

const int CouchRunServer::SERVER_STOPPED;
const int CouchRunServer::SERVER_INITED;
const int CouchRunServer::SERVER_RUNNING;

/*
class MyException : public std::runtime_error {
public:
  MyException( char * txt ) : std::runtime_error( std::string(txt) ) { }
};
*/
static jmp_buf env;
static std::string S_pending;

const std::string CouchRunServer::getPending(){
  return S_pending;
}

/*------------------------------------------------------------------------------
 * GUI Defined in nkb-couchgtk.cpp
 *------------------------------------------------------------------------------
*/
extern void winQuestion( std::string question );

/*------------------------------------------------------------------------------
 * Working Memory Driver (wm_)
 *------------------------------------------------------------------------------
*/
extern "C"
int mystrcmp( char *s1, char *s2 ){
  printf( "Comparing %s, %s\n", s1, s2 );
  return strcmp( s1, s2 );
}

/** The working memory
 */
static CouchMemory *S_cmem = NULL;
static std::vector< std::string > S_Answers;

// The assign process updates the data store (working memory)
extern "C"
int wm_assign( char* txt, int val ){ S_cmem->set( txt, val ); return 1;}

extern "C"
int wm_assignh( char* txt, int val ){S_cmem->seth( txt, val ); return 1;}

extern "C"
int wm_assignd( char* txt, double val ){S_cmem->set( txt, val ); return 1;}

extern "C"
int wm_assignc( char* txt, char *val ){S_cmem->set( txt, val );return 1;}

extern "C"
int wm_known( char* txt ){return S_cmem->knownp( txt );}

extern "C"
int message( char* txt ){ printf( "KBVM> %s\n", txt ); return 1;}

/*------------------------------------------------------------------------------
 * User Interaction Driver (ux_).  Depends on wm_
 *------------------------------------------------------------------------------
*/
void suspend( char *s){S_pending = std::string( s ); longjmp( env, 1 );}

// The question process uses a data store and defaults to end user
extern "C" 
int ux_question( char* txt ){
  int ret = wm_known( txt );
  if( 0 == ret ){
    // Defaults to asking user
    //fprintf( stderr, "%s (y/n)? ", txt );
    suspend(txt);
    return 0;
  }
  else{
    //printf( "Returning %s = %d\n", txt, S_cmem->get(txt) );
    return S_cmem->get( txt );
  }
}

extern "C" 
double ux_questiond( char* txt ){
  int ret = wm_known( txt );
  if( 0 == ret ){
    // Defaults to asking user
    //fprintf( stderr, "%s (num value)? ", txt );
    suspend(txt);
    return 0.0;
  }
  else{
    return S_cmem->getd( txt );
  }
}

extern "C" 
const char *ux_questionc( char* txt ){
  int ret = wm_known( txt );
  if( 0 == ret ){
    // Defaults to asking user
    //fprintf( stderr, "%s (char value)? ", txt );
    suspend(txt);
    return S_Answers.back().c_str();
  }
  else{
    S_Answers.push_back( std::string(S_cmem->getc(txt)) );
    return S_Answers.back().c_str();
  }
}

/*------------------------------------------------------------------------------
 * Agenda (ag_)
 *------------------------------------------------------------------------------
*/
/** The Agenda.
 */
CouchExtAgenda *CouchRunServer::S_Agenda;

extern "C"
int ag_ppeval( char* txt ){
  std::string goal( txt );
  int ret = CouchRunServer::S_Agenda->push( goal, S_cmem );
  std::cout << "Agenda: Forwarding hypothesis: " << goal << std::endl;
  // std::cout << *CouchRunServer::S_Agenda;
  return ret;
}

/* JIT Compilation.
 */
static ExecutionEngine *S_TheExecutionEngine;
static FunctionPassManager *S_TheFPM;

CouchRunServer::  CouchRunServer( std::string URL, std::string DB, std::string DOC ){
  CouchRunServer::S_Agenda = new CouchExtAgenda();
  couchURL = URL; couchDB = DB; couchDoc = DOC; 
  current_state = SERVER_STOPPED;
}

CouchRunServer::~CouchRunServer(){
  if( NULL != CouchRunServer::S_Agenda ) delete CouchRunServer::S_Agenda;
}

DOMDocument *CouchRunServer::parsePrologue( std::string S_PrologueXMLFilename ){
  bool recognizeNEL = false;
  char localeStr[64];
  memset(localeStr, 0, sizeof localeStr);

  // Initialize the XML4C system
  try {
    if (strlen(localeStr)) {
	  XMLPlatformUtils::Initialize(localeStr);
    }
    else{
	  XMLPlatformUtils::Initialize();
    }
    
    if (recognizeNEL){
      XMLPlatformUtils::recognizeNEL(recognizeNEL);
    }
  }
  catch (const XMLException& toCatch){
    XERCES_STD_QUALIFIER cerr << "Error during initialization! :\n"
			      << StrX(toCatch.getMessage()) << XERCES_STD_QUALIFIER endl;
    return NULL;
  }

  // Instantiate the DOM parser.
  static const XMLCh gLS[] = { chLatin_L, chLatin_S, chNull };
  DOMImplementation *impl = 
    DOMImplementationRegistry::getDOMImplementation(gLS);
  DOMLSParser *parser = 
    ((DOMImplementationLS*)impl)->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
  DOMConfiguration  *config = parser->getDomConfig();
  // enable datatype normalization - default is off
  config->setParameter(XMLUni::fgDOMDatatypeNormalization, true);

  // And create our error handler and install it
  DOMCountErrorHandler errorHandler;
  config->setParameter(XMLUni::fgDOMErrorHandler, &errorHandler);

  //
  //  Get the starting time and kick off the parse of the indicated
  //  file. Catch any exceptions that might propogate out of it.
  //
  unsigned long duration;
  const char *xmlFile = 0;
  bool more = true;
  XERCES_STD_QUALIFIER ifstream fin;

  // Later possibly a list of prologue files rather than a single one
  while (more){
    xmlFile = S_PrologueXMLFilename.c_str();
    more = false;
  }
      
  // Parse XML prologue file
  // Reset error count first
  errorHandler.resetErrors();

  bool errorOccurred = false;

  try{
    // reset document pool
    parser->resetDocumentPool();
      
    const unsigned long startMillis = XMLPlatformUtils::getCurrentMillis();
    XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *doc = parser->parseURI(xmlFile);
    const unsigned long endMillis = XMLPlatformUtils::getCurrentMillis();
    duration = endMillis - startMillis;
    // Print out the stats that we collected and time taken.
    XERCES_STD_QUALIFIER cout << "Parsed " 
			      << xmlFile << ": " << duration << " ms."
			      << XERCES_STD_QUALIFIER endl;
    return doc;
  }
  catch (const XMLException& toCatch){
    XERCES_STD_QUALIFIER cerr << "\nError during parsing: '" << xmlFile << "'\n"
			      << "Exception message is:  \n"
			      << StrX(toCatch.getMessage()) << "\n" 
                              << XERCES_STD_QUALIFIER endl;
    errorOccurred = true;
    //    continue;
  }
  catch (const DOMException& toCatch){
    const unsigned int maxChars = 2047;
    XMLCh errText[maxChars + 1];
      
    XERCES_STD_QUALIFIER cerr << "\nDOM Error during parsing: '" 
			      << xmlFile << "'\n"
			      << "DOMException code is:  " 
			      << toCatch.code 
			      << XERCES_STD_QUALIFIER endl;
      
    if (DOMImplementation::loadDOMExceptionMsg(toCatch.code, errText, maxChars))
      XERCES_STD_QUALIFIER cerr << "Message is: " 
				<< StrX(errText) 
				<< XERCES_STD_QUALIFIER endl;
      
    errorOccurred = true;
    //    continue;
  }
  catch (...){
    XERCES_STD_QUALIFIER cerr << "\nUnexpected exception during parsing: '" << xmlFile << "'\n";
    errorOccurred = true;
    //    continue;
  }

  return NULL;
}

const int CouchRunServer::status(){ 
  CouchMemory *cmem = new CouchMemory( couchURL, couchDB, couchDoc );
  CouchExtAgenda *agenda = new CouchExtAgenda();
  int n, p;
  cmem->getAgenda( agenda );
  p = (NULL == agenda) ? 0 : agenda->size();
  n = cmem->getWMSize();
  std::cout << "Agenda Size: " << p << std::endl;
  std::cout << "WM Size: " << n << std::endl;
  if( 0 == n && p >= 0 ){ n = SERVER_INITED; }
  else if( 0 != n && p > 0 ){ n = SERVER_RUNNING; }
  else if( 0 != n && 0 == p ){ n = SERVER_STOPPED; }
  else{ n = -1; }
  delete agenda;
  delete cmem;
  return n;
}

void CouchRunServer::getEncyclopedia( std::map<std::string, std::string> *map_h, std::map<std::string, std::string> *map_s ){
  CouchMemory *cmem = new CouchMemory( couchURL, couchDB, couchDoc );
  cmem->getEncyclopedia( map_h, map_s );
  delete cmem;
}

void CouchRunServer::init(){
  CouchMemory *cmem = new CouchMemory( couchURL, couchDB, couchDoc );
  cmem->clear();
  // Parse the session prologue XML file
  std::string fPrologueFile = cmem->get_prologue( std::string( "session.xml" ) );
  fDom = parsePrologue( fPrologueFile );
  if( NULL == fDom ){
    return;
  }

  // Prepares LLVM JIT compiler for later
  InitializeNativeTarget();

  delete cmem;
}


void CouchRunServer::restart(){
  CouchMemory *cmem = new CouchMemory( couchURL, couchDB, couchDoc );
  // Inital setup of agenda and working memory
  unsigned int i;
  cmem->clear();
  S_cmem = cmem;

  // Load assembly source into a new module
  std::string error;
  SMDiagnostic Err;
  std::string fSourceFile = cmem->get_assembly( std::string("assembly.s") );
  std::auto_ptr<Module> theM( ParseAssemblyFile( fSourceFile, 
						 Err, 
						 getGlobalContext() ));
  std::cout << "KBVM v1.0 (experimental)\nServer\n";  
  std::cout << "Parsing " << fSourceFile << "\n";
  try{
    std::string VerifErr;
    if (verifyModule( *theM.get(), ReturnStatusAction, &VerifErr)) {
      errs() << fSourceFile
	     << ": assembly parsed, but does not verify as correct!\n";
      errs() << VerifErr;
      return;
    }
  }
  catch(...){
    std::cerr << "Verification of module failed!\n";
    return;
  }

  // Create the JIT.
  S_TheExecutionEngine = ExecutionEngine::create(theM.get());

  //ExistingModuleProvider OurModuleProvider( M );
  FunctionPassManager OurFPM( theM.get() );
  // Set up the optimizer pipeline.  
  // Start with registering info about how the
  // target lays out data structures.
  OurFPM.add(new TargetData(*S_TheExecutionEngine->getTargetData()));
  // Promote allocas to registers.
  OurFPM.add(createPromoteMemoryToRegisterPass());
  // Do simple "peephole" optimizations and bit-twiddling optzns.
  OurFPM.add(createInstructionCombiningPass());
  // Reassociate expressions.
  OurFPM.add(createReassociatePass());
  // Eliminate Common SubExpressions.
  OurFPM.add(createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  OurFPM.add(createCFGSimplificationPass());
  // Set the global so the code gen can use this.
  S_TheFPM = &OurFPM;
  // (Re)Validate forwarding function
  S_TheFPM->run( *theM->getFunction("ag_forward") );

  CouchRunServer::S_Agenda->reset();

  typedef void (*FFwrdPtr)( const char* );
  std::string sign, val;
  Function *FFwrd = S_TheExecutionEngine->FindFunctionNamed( "ag_forward" );
  FFwrdPtr FFwrdP = reinterpret_cast<FFwrdPtr>( S_TheExecutionEngine->getPointerToFunction(FFwrd) );
  DOMElement *elt;

  DOMNodeList *domSuggests = 
    fDom->getElementsByTagName( XMLString::transcode("suggest") );
  for( i = 0; i < domSuggests->getLength(); i++ ){
    CouchRunServer::S_Agenda->push( std::string(XMLString::transcode( ((DOMText *)(domSuggests->item(i)->getFirstChild()))->getData() )), S_cmem );
  }

  DOMNodeList *domVolunteers = 
    fDom->getElementsByTagName( XMLString::transcode("volunteer") );
  for( i = 0; i < domVolunteers->getLength(); i++ ){
    elt = (DOMElement *)(domVolunteers->item(i));
    sign = std::string( XMLString::transcode( ((DOMText *)(elt->getElementsByTagName(XMLString::transcode("var"))->item(0)->getFirstChild()))->getData() ) );
    val = std::string( XMLString::transcode( ((DOMText *)(elt->getElementsByTagName(XMLString::transcode("const"))->item(0)->getFirstChild()))->getData() ) );
    // TODO: update with type information
    if( std::string("true") == val ){
      cmem->set( sign.c_str(), (int)1 );
    }
    else if( std::string("false") == val ){
      cmem->set( sign.c_str(), (int)0 );
    }
    else if( isalpha( *(val.c_str()) ) ){
      cmem->set( sign.c_str(), val.c_str() );
    }
    else{
      double d = strtod( val.c_str(), NULL );
      cmem->set( sign.c_str(), d );
    }
    // Update agenda as if forwarded from execution
    FFwrdP( sign.c_str() );
  }

  cmem->setAgenda( CouchRunServer::S_Agenda );

  // Clean-up
  delete cmem;
  S_cmem = NULL;
  current_state = SERVER_INITED;
}

void CouchRunServer::next(){
  // Prepares LLVM JIT and pass manager
  InitializeNativeTarget();
  LLVMContext &Context = getGlobalContext();  
  
  CouchMemory *cmem = 
    new CouchMemory( couchURL, couchDB, couchDoc );

  std::string fSourceFile = cmem->get_assembly( std::string("assembly.s") );

  // Load assembly source into a new module
  std::string error;
  SMDiagnostic Err;
  std::auto_ptr<Module> theM( ParseAssemblyFile( fSourceFile, 
						 Err, 
						 Context ));
  std::cout << "KBVM v1.0 (experimental)\nServer\n";  
  std::cout << "Parsing " << fSourceFile << "\n";
  try{
    std::string VerifErr;
    if (verifyModule( *theM.get(), ReturnStatusAction, &VerifErr)) {
      errs() << fSourceFile
	     << ": assembly parsed, but does not verify as correct!\n";
      errs() << VerifErr;
      return;
    }
  }
  catch(...){
    std::cerr << "Verification of module failed!\n";
    return;
  }

  // Create the JIT.
  S_TheExecutionEngine = ExecutionEngine::create(theM.get());
  IRBuilder<>  theBuilder( Context );

  //ExistingModuleProvider OurModuleProvider( M );
  FunctionPassManager OurFPM( theM.get() );
      
  // Set up the optimizer pipeline.  
  // Start with registering info about how the
  // target lays out data structures.
  OurFPM.add(new TargetData(*S_TheExecutionEngine->getTargetData()));
  // Promote allocas to registers.
  OurFPM.add(createPromoteMemoryToRegisterPass());
  // Do simple "peephole" optimizations and bit-twiddling optzns.
  OurFPM.add(createInstructionCombiningPass());
  // Reassociate expressions.
  OurFPM.add(createReassociatePass());
  // Eliminate Common SubExpressions.
  OurFPM.add(createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  OurFPM.add(createCFGSimplificationPass());
  // Set the global so the code gen can use this.
  S_TheFPM = &OurFPM;

  // Cache Agenda from CouchDB persistent datastore
  S_cmem = cmem;
  CouchRunServer::S_Agenda->reset();
  S_cmem->getAgenda( CouchRunServer::S_Agenda );
  std::cout << "Session Started\n" << *CouchRunServer::S_Agenda;

  {
    // Signature of goal function calls
    typedef int (*PFN)();
    
    // The `entry' function repeatedly JIT-compiles the top goal of the agenda 
    std::string hypo;
    std::vector<Value *> args;
    Function *Main;
    Value *res, *call;
    int state;

    while( !CouchRunServer::S_Agenda->empty() ){
      // Get top priority goal in agenda
      hypo = std::string( CouchRunServer::S_Agenda->top() );
      std::cout << hypo << " for evaluation\n";
      // If goal is unknown, emit code for its evaluation
      if( WorkingMemory::WM_UNKNOWN == S_cmem->knownp( hypo.c_str() ) ){
	// Clean up function from previous iteration, if present
	if( NULL != (Main = theM->getFunction("entry")) ){
	  Main->eraseFromParent();
	}
	
	// Create entry block of goal funciton
	Main = cast<Function>( theM->getOrInsertFunction( "entry", Type::getInt32Ty(Context), (Type *)0) );
	BasicBlock *EB = BasicBlock::Create(Context, "EntryBlock", Main);
	theBuilder.SetInsertPoint(EB);
	call = theBuilder.CreateCall(theM->getFunction( hypo ),
				     args.begin(),
				     args.end(), "suggest");
	res = 
	  theBuilder.CreateIntCast( call, 
				    Type::getInt32Ty(Context), 
				    false, "cast_tmp" );
	//	theBuilder.CreateRet( res == NULL ? ConstantInt::get(Type::getInt32Ty(Context), 2) : res );
	theBuilder.CreateRet( res );
	// Runs all passes on this function
	S_TheFPM->run( *Main );
	// (Re)Launch session by calling it
	//void *FPtr = S_TheExecutionEngine->getPointerToFunction(Main);
	PFN FP = reinterpret_cast<PFN>( S_TheExecutionEngine->getPointerToFunction(Main) );
	// If an interactive question arises, an exception is thrown
	/*
	try{
	  int val = FP();
	  // Evaluation has completed w/o interactions, pop agenda
	  (void)CouchRunServer::S_Agenda->pop();
	  S_cmem->setAgenda( CouchRunServer::S_Agenda );
	  std::cout << hypo << " evaluated: " << val << "\n";
	}
	catch( MyException& exc ){
	  // Interactive question occurred.  Persist agenda, clean up and exit
	  current_state = SERVER_RUNNING;
	  S_cmem->setAgenda( CouchRunServer::S_Agenda );
	  std::cout << "Session suspended on: "<< exc.what()  << "\n";
	  delete cmem;
	  S_cmem = NULL;
	  return;
	}
	catch( ... ){
	  std::cerr << "Unknown Exception!\n";
	  delete cmem;
	  S_cmem = NULL;
	  return;
	}
	*/
	state = setjmp( env );
	// printf( "State is %d\n", state );
	if( state ){
	  // Interactive question occurred.  Persist agenda, clean up and exit
	  current_state = SERVER_RUNNING;
	  S_cmem->setAgenda( CouchRunServer::S_Agenda );
	  std::cout << "Session suspended on: " << S_pending <<  "\n";
	  delete cmem;
	  S_cmem = NULL;
	  winQuestion( S_pending );
	  return;
	}
	else{
	  int val = FP();
	  // Evaluation has completed w/o interactions, pop agenda
	  (void)CouchRunServer::S_Agenda->pop();
	  S_cmem->setAgenda( CouchRunServer::S_Agenda );
	  std::cout << hypo << " evaluated: " << val << "\n";
	}
      }
      else{
	// Top goal in agenda is known, pop it
	(void)CouchRunServer::S_Agenda->pop();
	S_cmem->setAgenda( CouchRunServer::S_Agenda );
	std::cout << "Popped known: " << hypo << std::endl;
      }
    }
    // Agenda is empty, session is terminated: stop server
    current_state = SERVER_STOPPED;
    S_cmem->setAgenda( CouchRunServer::S_Agenda );
    std::cout << "Session terminated.\n";
  }

  delete cmem;
  S_cmem = NULL; 
}
  
void CouchRunServer::suggest( std::string hypo ){
  CouchMemory *cmem = new CouchMemory( couchURL, couchDB, couchDoc );
  CouchExtAgenda *agenda = new CouchExtAgenda();
  cmem->getAgenda( agenda );
  agenda->push( hypo );
  cmem->setAgenda( agenda );
  delete agenda;
  delete cmem;
}

void CouchRunServer::volunteer( std::string sign, int val_bool ){
  CouchMemory *cmem = new CouchMemory( couchURL, couchDB, couchDoc );
  cmem->sets( sign.c_str(), val_bool );
  delete cmem;
}

void CouchRunServer::volunteer( std::string sign, double val_num ){
  CouchMemory *cmem = new CouchMemory( couchURL, couchDB, couchDoc );
  cmem->sets( sign.c_str(), val_num );
  delete cmem;
}

void CouchRunServer::volunteer( std::string sign, std::string val_str ){
  CouchMemory *cmem = new CouchMemory( couchURL, couchDB, couchDoc );
  cmem->sets( sign.c_str(), val_str.c_str() );
  delete cmem;
}

