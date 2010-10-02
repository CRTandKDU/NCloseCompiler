//===--- KBIRActionExpr.cpp - Rule RHS Generator ---- ---------------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// Generates IR code for the RHS of a NClose rule
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

#include "KBIRExpr.hpp"
#include "KBIRActionExpr.hpp"
#include "KBIRGenerator.hpp"

XERCES_CPP_NAMESPACE_USE
using namespace llvm;

// Element tags in commands
const std::string KBIRActionExpr::S_IdVar = "var";
const std::string KBIRActionExpr::S_IdConst = "const";

// Boolean true value
const std::string KBIRActionExpr::Sc_IdTrue = "true";

// Commands. 
const std::string KBIRActionExpr::Sc_OpSetB = "set-bool";
const std::string KBIRActionExpr::Sc_OpSetD = "set-num";
const std::string KBIRActionExpr::Sc_OpSetS = "set-str";


void KBIRActionExpr::gen( Function *F, Module *M, LLVMContext &Context ){
  DOMElement *node;
  DOMNode *cur;
  DOMNodeList *domCommands =
    dom->getElementsByTagName( XMLString::transcode( "command" ) );
  unsigned int i, j;
  std::string op;
  char *cst, *s;
  BasicBlock *BB = KBIRGenerator::S_Builder.GetInsertBlock();
  Value *res[2], *inst;
  std::vector<Value *> args;
  bool pos2var = false;

  KBIRGenerator::S_Builder.SetInsertPoint( BB );
  for( i = 0; i < domCommands->getLength(); i++ ){
    node = (DOMElement *)domCommands->item(i);
    op = 
      XMLString::transcode( node->getAttribute( XMLString::transcode("op") ) );
    if( 1 == S_OpCmdSet( op.c_str() ) ){
      // There are 2 arguments the first of which is a <var>
      for( j = 0, cur = node->getFirstChild(); cur; cur = cur->getNextSibling() ){
	s = XMLString::transcode(cur->getNodeName());
	// A  variable
	if( 0 == S_IdVar.compare( s ) ){
	  pos2var = (j > 0);
	  cst = 
	    XMLString::transcode(((DOMText *)(cur->getFirstChild()))->getData());
	  args.clear();
	  args.push_back( res[j++] = getOrInsertSign( cst, F, M, Context ) );
	  KBIRGenerator::S_Builder.CreateCall( M->getFunction("ag_forward"),
				args.begin(), args.end() );
	}
	else if( 0 == S_IdConst.compare( s ) ){
	  cst = 
	    XMLString::transcode(((DOMText *)(cur->getFirstChild()))->getData());
	  // Type is determined by command name
	  if( op == Sc_OpSetB ){
	    res[j++] = ( 0 == Sc_IdTrue.compare( cst ) ) ? 
	      ConstantInt::get( KBIRGenerator::intType(Context), 1) : 
	      ConstantInt::get( KBIRGenerator::intType(Context), 0) ;
	  }
	  else if( op == Sc_OpSetD ){
	    res[j++] = ConstantFP::get( Type::getDoubleTy(Context), 
					strtod( cst, NULL ) );
	  }
	  else if( op == Sc_OpSetS ){
	    res[j++] = 
	      KBIRGenerator::CreateEntryGlobalStringPtr( M, F, cst,Context);
	  }
	  else{
	    // Unknown set operator
	  }
	}
	else{
	  // Error: neither <var> nor <const>? A text node.
	}
      }
      // Code generation
      // Second argument may be a var
      
      if( pos2var ){
	args.clear();
	args.push_back( res[1] );

	if( op == Sc_OpSetB ){
	  inst = KBIRGenerator::S_Builder.CreateCall(M->getFunction("ux_question"),
				      args.begin(), args.end(),
				      "question_datastore" );
	  args.clear();
	  args.push_back( res[0] );
	  args.push_back( inst );
	  KBIRGenerator::S_Builder.CreateCall( M->getFunction("wm_assign"), 
				args.begin(), args.end(),
				"update_datastore" );
	}
	else if( op == Sc_OpSetD ){
	  inst = KBIRGenerator::S_Builder.CreateCall(M->getFunction("ux_questiond"),
				      args.begin(), args.end(),
				      "question_datastore" );
	  args.clear();
	  args.push_back( res[0] );
	  args.push_back( inst );
	  KBIRGenerator::S_Builder.CreateCall( M->getFunction("wm_assignd"), 
				args.begin(), args.end(),
				"updated_datastore" );

	}
	else if( op == Sc_OpSetS ){
	  inst = KBIRGenerator::S_Builder.CreateCall(M->getFunction("ux_questionc"),
				      args.begin(), args.end(),
				      "question_datastore" );
	  args.clear();
	  args.push_back( res[0] );
	  args.push_back( inst );
	  KBIRGenerator::S_Builder.CreateCall( M->getFunction("wm_assignc"), 
				args.begin(), args.end(),
				"updatec_datastore" );

	}
	else{
	  // Unknown set operator
	}
      } 
      // The second argument is a <const>
      else{
	args.clear();
	args.push_back( res[0] );
	args.push_back( res[1] );
	if( op == Sc_OpSetB ){
	  KBIRGenerator::S_Builder.CreateCall( M->getFunction("wm_assign"), 
				args.begin(), args.end(),
				"update_datastore" );
	}
	else if( op == Sc_OpSetD ){
	  KBIRGenerator::S_Builder.CreateCall( M->getFunction("wm_assignd"), 
				args.begin(), args.end(),
				"updated_datastore" );

	}
	else if( op == Sc_OpSetS ){
	  KBIRGenerator::S_Builder.CreateCall( M->getFunction("wm_assignc"), 
				args.begin(), args.end(),
				"updatec_datastore" );
	}
	else{
	  // Unknown set operator
	}
      }
    }
    else{
      // Other commands!
    }
  }
  return;
}
