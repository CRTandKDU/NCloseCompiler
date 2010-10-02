//===--- KBIRRunner.cpp - The NClose IR interpreter -----------------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// This class implements a simple LLVM-based interpreter for IR generated
// by the nkb-compile tool, or by the `KBIRGenerator' class.
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

#if defined(XERCES_NEW_IOSTREAMS)
#include <fstream>
#else
#include <fstream.h>
#endif


#include "CouchAgenda.hpp"
#include "../runner/KBIRRunner.hpp"
#include "CouchRunner.hpp"
#include "CouchWorkingMemory.hpp"

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
static CouchWorkingMemory *S_wm = NULL;
static std::vector< std::string > S_Answers;

// The assign variants update the data store (working memory)
extern "C"
int wm_assign( char* txt, int val ){
  S_wm->set( txt, val );
  return 1;
}

extern "C"
int wm_assignh( char* txt, int val ){
  S_wm->set( txt, val );
  return 1;
}

extern "C"
int wm_assignd( char* txt, double val ){
  S_wm->set( txt, val );
  return 1;
}

extern "C"
int wm_assignc( char* txt, char *val ){
  S_wm->set( txt, val );
  return 1;
}

extern "C"
int wm_known( char* txt ){
  return S_wm->knownp( txt );
}

extern "C"
int message( char* txt ){
  printf( "KBVM> %s\n", txt );
  return 1;
}

/*------------------------------------------------------------------------------
 * User Interaction Driver (ux_).  Depends on wm_
 *------------------------------------------------------------------------------
*/

// The question process uses a data store and defaults to end user
extern "C" 
int ux_question( char* txt ){
  int ret = wm_known( txt );
  if( 0 == ret ){
    // Defaults to asking user
    printf( "%s (y/n)? ", txt );
    char c = getchar();
    printf( "\n" );
    ret = ('y' == c) ? 1 : 0;
    // Side-effects
    wm_assign( txt, ret );
    return ret;
  }
  else{
    printf( "Returning %s = %d\n", txt, S_wm->get(txt) );
    return S_wm->get( txt );
  }
}

extern "C" 
double ux_questiond( char* txt ){
  int ret = wm_known( txt );
  if( 0 == ret ){
    double d;
    char str[255];
    // Defaults to asking user
    printf( "%s (num value)? ", txt );
    d = strtod( gets( str ), NULL );
    // Side-effects
    S_wm->set( txt, d );
    return d;
  }
  else{
    return S_wm->getd( txt );
  }
}

extern "C" 
const char *ux_questionc( char* txt ){
  int ret = wm_known( txt );
  char ans[255];

  if( 0 == ret ){
    // Defaults to asking user
    printf( "%s (char value)? ", txt );
    gets( ans );
    // Side-effects
    S_wm->set( txt, ans );
    S_Answers.push_back( std::string(ans) );
    return S_Answers.back().c_str();
  }
  else{
    S_Answers.push_back( std::string(S_wm->getc(txt)) );
    return S_Answers.back().c_str();
  }
}

/*------------------------------------------------------------------------------
 * Agenda (ag_)
 *------------------------------------------------------------------------------
*/
/** The Agenda.
 */
CouchAgenda *CouchRunner::S_Agenda;

extern "C"
int ag_ppeval( char* txt ){
  std::string goal( txt );
  int ret = CouchRunner::S_Agenda->push( goal, S_wm );
  std::cout << "Agenda: Forwarding hypothesis: " << goal << std::endl;
  std::cout << *CouchRunner::S_Agenda;
  return ret;
}

/* JIT Compilation.
 */
static ExecutionEngine *S_TheExecutionEngine;
static FunctionPassManager *S_TheFPM;

CouchRunner::CouchRunner( std::string s, DOMDocument *domDoc ){
  fSourceFile = s; fDom = domDoc;
  CouchRunner::S_Agenda = new CouchAgenda();
  InitializeNativeTarget();
}

CouchRunner::~CouchRunner(){
  if( CouchRunner::S_Agenda ) delete CouchRunner::S_Agenda;
}

void CouchRunner::runIR( CouchWorkingMemory *wm ){
  /*
  Module *theM;
  theM = ParseBitcodeFile( MemoryBuffer::getFile("out.bc"), 
			   getGlobalContext(),
			   &error );
  */

  // Loads intermediate representation source
  std::string error;
  SMDiagnostic Err;
  std::auto_ptr<Module> theM(ParseAssemblyFile( fSourceFile, Err, getGlobalContext() ));
  std::cout << "KBVM v1.0 (experimental)\nIntermediate Representation Interpreter\n";  
  std::cout << "Parsing " << fSourceFile << "\n";
  // Verifies module
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
  // Prepares to execute
  S_wm = wm;

  IRBuilder<>  theBuilder( getGlobalContext() );
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

  // Inital setup of the agenda
  unsigned int i;
  CouchRunner::S_Agenda->reset();

  // Initial setup of the working memory
  S_TheFPM->run( *theM->getFunction("ag_forward") );

  std::string sign, val;
  Function *FFwrd = S_TheExecutionEngine->FindFunctionNamed( "ag_forward" );
  void *FFwrdPtr = S_TheExecutionEngine->getPointerToFunction(FFwrd);
  void (*FFwrdP)(const char*) = (void (*)( const char*))FFwrdPtr;
  DOMElement *elt;

  DOMNodeList *domSuggests = 
    fDom->getElementsByTagName( XMLString::transcode("suggest") );
  for( i = 0; i < domSuggests->getLength(); i++ ){
    CouchRunner::S_Agenda->push( std::string(XMLString::transcode( ((DOMText *)(domSuggests->item(i)->getFirstChild()))->getData() )), S_wm );
  }

  DOMNodeList *domVolunteers = 
    fDom->getElementsByTagName( XMLString::transcode("volunteer") );
  for( i = 0; i < domVolunteers->getLength(); i++ ){
    elt = (DOMElement *)(domVolunteers->item(i));
    sign = std::string( XMLString::transcode( ((DOMText *)(elt->getElementsByTagName(XMLString::transcode("var"))->item(0)->getFirstChild()))->getData() ) );
    val = std::string( XMLString::transcode( ((DOMText *)(elt->getElementsByTagName(XMLString::transcode("const"))->item(0)->getFirstChild()))->getData() ) );
    // TODO: update with type information
    if( std::string("true") == val ){
      S_wm->set( sign.c_str(), (int)1 );
    }
    else if( std::string("false") == val ){
      S_wm->set( sign.c_str(), (int)0 );
    }
    else if( isalpha( *(val.c_str()) ) ){
      S_wm->set( sign.c_str(), val.c_str() );
    }
    else{
      double d = strtod( val.c_str(), NULL );
      S_wm->set( sign.c_str(), d );
    }
    // Update agenda as if forwarded from execution
    FFwrdP( sign.c_str() );
  }


  std::cout << "Session Started\n" << *S_wm << *CouchRunner::S_Agenda;

  {
    // Creates the `entry' function that sets up the Agenda, which
    // also serves as continuation for the postponed calls
    std::vector<Value *> args;
    Function *Main;


    while( !CouchRunner::S_Agenda->empty() ){

      // Erase previous entry function if present
      if( NULL != (Main = theM->getFunction("entry")) ){
	Main->eraseFromParent();
      }
      // Creates code block
      Main = cast<Function>(
			    theM->getOrInsertFunction( "entry", 
						       Type::getInt32Ty(getGlobalContext()), 
						       (Type *)0)
			    );
      BasicBlock *EB = BasicBlock::Create(getGlobalContext(), 
					  "EntryBlock", Main);
      theBuilder.SetInsertPoint(EB);
      // Loops through the current agenda
      Value *res = NULL, *call;
      std::string hypo;

      // Copies agenda to current Agenda
	
      while( !CouchRunner::S_Agenda->empty() ){
	hypo = CouchRunner::S_Agenda->pop();
	if( WorkingMemory::WM_UNKNOWN == S_wm->knownp( hypo.c_str() ) ){
	  call = theBuilder.CreateCall(theM->getFunction( hypo ),
				       args.begin(),
				       args.end(), "suggest");
	  res = 
	    theBuilder.CreateIntCast( call, 
				      Type::getInt32Ty(getGlobalContext()), 
				      false, "cast_tmp" );
	}
      }
      theBuilder.CreateRet( res == NULL ? 
			    ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 2) : 
			    res );
      // Check it
      S_TheFPM->run( *Main );
      // Run it
      Function *F = S_TheExecutionEngine->FindFunctionNamed( "entry" );
      void *FPtr = S_TheExecutionEngine->getPointerToFunction(F);
      typedef int (*PFN)();
      PFN FP = reinterpret_cast<PFN>( FPtr );
      //      int (*FP)() = (int (*)())FPtr;
      printf( "Result = %d\n", FP() );
      std::cout << *S_wm << std::endl;
      std::cout << *CouchRunner::S_Agenda << std::endl;
    }  
    S_wm = NULL;
  }
}
