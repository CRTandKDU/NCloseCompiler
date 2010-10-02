//===--- KBIRGenerator.cpp - The NClose IR generator ----------------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// This class implements a simple LLVM-based IR generator for NClose KBs
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
#include "KBIRConditionExpr.hpp"
#include "KBIRActionExpr.hpp"
#include "KBIRGenerator.hpp"

/*
  NOTES: @author jmc@neurondata.org
  1) This experimental NClose knowledge-base compiler uses LLVM to generate IR code from an XML-coded source set of rules.  The document type definition for the XML representation is in `kbml.dtd'.  A `KBIRGenerator' object wraps up a LLVM Module (M), in which it generates the intermediate representation.  Its `generateIR' method basically fills in this module with IR code representing the rule set backward chaining execution. 

  In terms of design choices: each hypothesis maps to a function which "ors" function calls to individual rule-mapping functions.  The template rule function goes in sequence through each test in the LHS, returning `false' immediately if it fails, until the last test and then returns `true' and triggers the RHS.  The hypothesis function assigns the proper value to the then fully evaluated hypothesis.

  In tests and at the entry points of an hypothesis function, the engine needs to check whether the queried data is known or not.  (This in order to avoid asking questions twice, or re-evaluating rules already fired.)  Accordingly the API to the working memory is simplified to these three calls:

  - int known( char *data ): testing for a known/unknown variable

and for each of the basic scalar types in NClose (bool, num and string): 

  - int question( char *data ): triggers a query to the WM to retrieve the value defaulting to a form of user interaction if unknown
  - int assign( char *data, int *value ): updates  the WM

  Support for the agenda is provided outside the LLVM IR code.  However it requires information on forward links -- which hypotheses to be posted to the agenda when a sign is encountered during the inference process -- to be compiled into the assembly to be used at runtime.  Within each LLVM module a specific `ag_forward' function is compiled which given a sign as input repeatedly calls an externally linked C function to post an hypothesis to the current agenda.  The LLVM IR code for this function is generated as the last step before outputing the assembly code. 

  2) The `KBIRGenerator' object could also supports evaluation of the compiled knowledge base through the LLVM JIT compiler which execute IR code on the fly.  The `runIR' method starts the engine. [Update: this code has been moved to ../runner in `nkb-knowcess.cpp'

  In order to run properly though, the engine requires a proper WM.  This is where options are open and flexibility is important with respect to implementation choices.  The current implementation of the above (simple) WM API relies on a `WorkingMemory' object which wraps up the data store (be it memory, database or remote server, in single or multiple sources, with flat or object-oriented types).  The basic object supports `knownp', `get' and `set' methods capturing the WM API simple design.  (See `KBWorkingMemory.cpp' for a default implementation using C++ maps.)  It is used as a static variable in this implementation for experimentation.

  Other important considerations for derived subprojects.  Extending the LHS and RHS vocabularies with external syntaxes or obtject types.  As an example, we would like LHS tests and RHS commands be expressed in Javascript with proper transmission of variables (signs) binding from LHS to RHS.  (The V8 Javascript Engine from Google would provide an excellent workbench.)
  As another example of this extension or plug-in architure idea, we would like to be able to extend the current LHS/RHS expressions with a full-fledged object system à la Nexpert Object.  Even better, the pug-in architecture would allow experimenting with different object systems. (See: NClosEmacs, http://code.google.com/p/nclosemacs/ See: http://en.wikipedia.org/wiki/The_Art_of_the_Metaobject_Protocol The Art of the Meta-Object Protocol, for inspiration.)

*/

using namespace llvm;


/** The static LLVM IRBuilder */
IRBuilder<>  KBIRGenerator::S_Builder( getGlobalContext() );
/** Pointers to constants */
Value * KBIRGenerator::S_One = 
  ConstantInt::get(Type::getInt8Ty(getGlobalContext()), 1);
Value * KBIRGenerator::S_Zero = 
  ConstantInt::get(Type::getInt8Ty(getGlobalContext()), 0);

// Sign identifiers.  See: `kbml.dtd'
static std::string S_IdVar( "var" );
static std::string S_IdConst( "const" );

/** The metadata map from hypothesis names to their global string implement */
std::map< std::string, Value * > KBIRGenerator::S_HypoGlobales;
/** The metadat map from signs to their global string implementation */
std::map< std::string, Value * > KBIRGenerator::S_SignGlobales;


/*------------------------------------------------------------------------------
 * Inlines to navigate DOM subtree rooted at the `dom' DOMElement 
 *------------------------------------------------------------------------------
*/
#define XML_CHILD_TEXT(ELTNAME) (XMLString::transcode(((DOMText *)(dom->getElementsByTagName(XMLString::transcode((ELTNAME)))->item(0)->getFirstChild()))->getData()))
#define XML_CHILD(ELTNAME) (dom->getElementsByTagName(XMLString::transcode((ELTNAME)))->item(0))
#define XML_2NDCHILD(ELTNAME) (dom->getElementsByTagName(XMLString::transcode((ELTNAME)))->item(1))

#define STRINGSIZE 255


/*------------------------------------------------------------------------------
 * Parse classes for IR code generation
 *------------------------------------------------------------------------------
*/ 


/** The Rule Expression class.  A Condition-Action-Hypothesis basic expression.
    @brief @b Compiler Code generation for a single rule
 */
class KBIRRuleExpr : public KBIRExpr{
public:
  KBIRRuleExpr( DOMElement *elt ) : KBIRExpr( elt) { F = NULL; };
  ~KBIRRuleExpr(){};

  void gen( Function *F, Module *M, LLVMContext &Context );

  /** Returns the LLVM function implementing the rule evaluation.
      @return A LLVM Function
  */
  Function *getF() { return F; };

  /** Returns the knowledge base hypothesis of the rule; note that several rules may of course lead to the same hypothesis (in one or several knowlegde bases).
      @return The text of the hypothesis as parsed from the XML knowlege base
  */
  char *getHypo() { return XML_CHILD_TEXT("hypothesis"); };
  
  /** A internal counter for rules leading to the same hypothesis. */
  static int count;

private:
  Function *F;
};

int KBIRRuleExpr::count = 0;

void KBIRRuleExpr::gen( Function *Ignore, Module *M, LLVMContext &Context ){
  std::string s( getHypo() );
  char sindex[6];

  sprintf( sindex, "-R%d", count++ );
  s += std::string(sindex),
    // std::cout << s << "\n";
  F =
    cast<Function>(M->getOrInsertFunction( s.c_str(), 
					   Type::getInt8Ty(Context), 
					   (Type *)0));

  // Add a basic block to the function.
  BasicBlock *BB = BasicBlock::Create(Context, "EntryBlock", F);
  KBIRGenerator::S_Builder.SetInsertPoint(BB);

  // Allocates local logic accumulator for result
  /*
  AllocaInst *Alloca = CreateEntryBlockAlloca(F, std::string("acc"), Context);
  
  KBIRGenerator::S_Builder.CreateStore( S_One, Alloca );

  KBIRGenerator::S_Builder.CreateMul( S_Zero, KBIRGenerator::S_Builder.CreateLoad(Alloca), "MulAcc");
  */
  KBIRConditionExpr *cond = 
    new KBIRConditionExpr( (DOMElement *)XML_CHILD("condition") );
  cond->gen( F, M, Context );
  
  // Execute the RHS
  if( NULL != XML_CHILD("action") ){
    KBIRActionExpr *action =
      new KBIRActionExpr( (DOMElement *)XML_CHILD("action") );
    action->gen( F, M, Context );
    delete action;
  }

  // The condition IR generation has previoulsy created the `return false;'
  KBIRGenerator::S_Builder.CreateRet( KBIRGenerator::S_One );
  delete cond;
}




Value *KBIRExpr::getOrInsertSign( char *cst, Function *F, Module *M, LLVMContext &Context ){
  Value *arg;
  std::string sign( cst );
  if( KBIRGenerator::S_SignGlobales.end() == KBIRGenerator::S_SignGlobales.find( sign ) ){
    arg = KBIRGenerator::S_SignGlobales[sign] 
        = KBIRGenerator::CreateEntryGlobalStringPtr( M, F, cst,Context);
  }
  else{
    arg = KBIRGenerator::S_SignGlobales[sign];
  }
  return arg;
}


/*------------------------------------------------------------------------------
 * Constructor
 *------------------------------------------------------------------------------
*/
/** Constructor
    @param doc A DOM Document as returned by the Xerces DOM parser
*/
KBIRGenerator::KBIRGenerator( DOMDocument *doc ){ 
  domDoc = doc; count = 0; M = NULL;
  InitializeNativeTarget();
}

KBIRGenerator::KBIRGenerator(){ 
  count = 0; M = NULL;
  InitializeNativeTarget();
}


KBIRGenerator::~KBIRGenerator(){ 
  std::cout << "Released Intermediate Representation Generator.\n";
  if( M ) delete M;
}

void KBIRGenerator::setDoc( DOMDocument *doc ){
  domDoc = doc;
}

/*------------------------------------------------------------------------------
 * IR Generation
 *------------------------------------------------------------------------------
*/

void KBIRGenerator::genFwrdFunction(){
  std::map < std::string, std::map< std::string, int > >::const_iterator it;
  std::map < std::string, int >::const_iterator ith;
  std::map < std::string, Value * >::const_iterator its;

  /*
  for( it = fFwrdLinks.begin(); it != fFwrdLinks.end(); ++it ){
    std::cout << it->first << "\n";
    for( ith = it->second.begin(); ith != it->second.end(); ++ith ){
      std::cout << "\t" << ith->first << "\n";
    }
  }
  */

  LLVMContext &Context = getGlobalContext();  
  BasicBlock *BB, *retBB_T, *nextBB;
  Function *F, *cmpF, *ppeF;
  std::vector<Value *> callArgs;
  Value *x, *inst, *isZero;

  // Setup
  cmpF = M->getFunction( "strcmp" );
  ppeF = M->getFunction( "ag_ppeval" );
  F = M->getFunction( "ag_forward" );
  x = F->arg_begin();
  BB = BasicBlock::Create(Context,"EntryBlock", F);
  S_Builder.SetInsertPoint( BB );
  // Iterate on signs
  for( it = fFwrdLinks.begin(); it != fFwrdLinks.end(); ++it ){
    //    std::cout << it->first << ": " << S_SignGlobales[it->first] << "\n";
    callArgs.clear();
    // Compares strings (sign names)
    callArgs.push_back( x );
    callArgs.push_back( S_SignGlobales[ it->first ] );
    inst = S_Builder.CreateCall( cmpF, callArgs.begin(), callArgs.end(), "compare" );
    inst = S_Builder.CreateIntCast( inst, Type::getInt8Ty(Context), false, "cast" );
    isZero = S_Builder.CreateICmpEQ( inst, S_Zero, "eq");
    // Next blocks to chain to
    nextBB = BasicBlock::Create( Context, "chain", F );
    retBB_T = BasicBlock::Create( Context, (it->first).c_str(), F );
    S_Builder.CreateCondBr( isZero, retBB_T, nextBB );
    S_Builder.SetInsertPoint( retBB_T );
    // Sequentially push forward hypotheses on the agenda
    for( ith = it->second.begin(); ith != it->second.end(); ++ith ){
      //std::cout << "\t" << ith->first << "\n";
      callArgs.clear();
      callArgs.push_back( S_HypoGlobales[ ith->first ] );
      inst = 
	S_Builder.CreateCall( ppeF, callArgs.begin(), callArgs.end(), 
			      "postpone_eval" );
      // Ignore result
    }
    S_Builder.CreateRetVoid();
    // Move insertion point to next test
    S_Builder.SetInsertPoint( nextBB );
  }
  S_Builder.CreateRetVoid();
}

void KBIRGenerator::genHypoFunction( std::map < std::string, std::vector< Function *> >::const_iterator iter ){
  LLVMContext &Context = getGlobalContext();  

  Value *arg, *res, *isPos, *g;
  Function *F;
  std::vector< Function * >::const_iterator func;

  if( (F = M->getFunction( iter->first )) ){
    /*
    std::cout << "Generating HypoFunction: " << iter->first << std::endl;
    for( func = iter->second.begin(); func < iter->second.end(); func++ ){
      std::cout << (*func)->getNameStr() << "\n";
    }
    */
    std::cout << "( ";

    std::vector<Value *> args_T, args_F;
    AllocaInst *Alloca = 
      KBIRGenerator::CreateEntryBlockAlloca(F, std::string("acc"), Context);
    BasicBlock *retBB_T = BasicBlock::Create(Context,"return_true", F);
    BasicBlock *retBB_F = BasicBlock::Create(Context,"return_false", F);
    char FName[ STRINGSIZE ];

    arg = S_HypoGlobales[ iter->first ];
    S_Builder.SetInsertPoint( retBB_T );
    args_T.push_back( arg );
    args_T.push_back( ConstantInt::get(intType(Context), 1) );
    S_Builder.CreateCall( M->getFunction("wm_assignh"), args_T.begin(), args_T.end(), "update_datastore" );
    S_Builder.CreateRet( S_One );

    S_Builder.SetInsertPoint( retBB_F );
    args_F.push_back( arg );
    args_F.push_back( ConstantInt::get(intType(Context), 0) );
    S_Builder.CreateCall( M->getFunction("wm_assignh"), args_F.begin(), args_F.end(), "update_datastore" );
    S_Builder.CreateRet( S_Zero );

    S_Builder.SetInsertPoint( &F->getEntryBlock() );
    S_Builder.CreateStore( S_Zero, Alloca );
    for( func = iter->second.begin(); func < iter->second.end(); func++ ){
      res = S_Builder.CreateCall( *func, "RuleFunction" );
      res = S_Builder.CreateIntCast( res, Type::getInt8Ty(Context), false, 
				   "cast_val" );

      // Trace each rule for a given hypo as if they were signs
     
      (void *)strcpy( FName, (*func)->getName().data() );
      g = KBIRGenerator::CreateEntryGlobalStringPtr( M, F, FName, Context);
      std::cout << FName << " ";
      args_F.clear();
      args_F.push_back( g );
      args_F.push_back( S_Builder.CreateIntCast( res, intType(Context), false, "recast" )  );
      S_Builder.CreateCall( M->getFunction("wm_assign"), 
			    args_F.begin(), args_F.end(), "update_datastore" );

      // End of trace

      res = S_Builder.CreateAdd( S_Builder.CreateLoad(Alloca), res, "AddAccumul" );
      S_Builder.CreateStore( res, Alloca );
    }
    std::cout << ")" << std::endl;

    isPos = S_Builder.CreateICmpUGT( S_Builder.CreateLoad(Alloca), S_Zero, "if_cond");
    S_Builder.CreateCondBr( isPos, retBB_T, retBB_F );
  }
}

void KBIRGenerator::collectFwrdLinks( DOMElement* dom ){
  // Narrow to condition
  DOMElement *LHS =
    (DOMElement *)(dom->getElementsByTagName( XMLString::transcode("condition") )->item(0));
  DOMNodeList *vars = LHS->getElementsByTagName( XMLString::transcode("var") );
  XMLSize_t i;
  std::string sHypo( XML_CHILD_TEXT("hypothesis") );
  //  std::cout << XML_CHILD_TEXT("hypothesis") << "\t" << (int)vars->getLength() << "\n";
  for( i = 0; i < vars->getLength(); i++ ){
    std::string sVar( XMLString::transcode( ((DOMText *)(vars->item(i)->getFirstChild()))->getData() ) );
    if( fFwrdLinks.end() == fFwrdLinks.find( sVar ) ){
      (fFwrdLinks[sVar])[sHypo] = 1;
    }
    else if( fFwrdLinks[sVar].end() == fFwrdLinks[sVar].find( sHypo ) ){
      (fFwrdLinks[sVar])[sHypo] ++;
    }
  }
}

/** Code generation
 */
void KBIRGenerator::generateIR( ){
  LLVMContext &Context = getGlobalContext();  
  DOMAttr *domRulesetName = 
    domDoc->getDocumentElement()->getAttributeNode( XMLString::transcode( "name" ));
  DOMNodeList *domRules = 
    domDoc->getElementsByTagName( XMLString::transcode("rule") );


  if( NULL == M ){
    // Create some module to put our rule set in
    M = new Module( (domRulesetName == NULL ? "UNNAMED" : XMLString::transcode( domRulesetName->getValue() )), Context);
    // Set up the external functions for the module
    /*
      int wm_known( char *)
      int wm_assign( char *, int ) and variants
      int ux_question( char * ) and variants
      double ux_questiond( char * )
      char * ux_questiond( char * )
      int ag_ppeval( char * )
      strcmp
    */
    std::vector<const Type*> Ptrs( 1, PointerType::get( charType(Context), 0) );
    FunctionType *FT = FunctionType::get( intType(Context), Ptrs, false); 
    Function *FExtern = 
      Function::Create(FT, Function::ExternalLinkage, "ux_question", M);
    FExtern = Function::Create(FT, Function::ExternalLinkage, "wm_known", M);
    FExtern = Function::Create(FT, Function::ExternalLinkage, "ag_ppeval", M);

    FT = FunctionType::get( Type::getDoubleTy( Context ), Ptrs, false );
    FExtern = Function::Create( FT, Function::ExternalLinkage, "ux_questiond", M );
    FT = FunctionType::get( PointerType::get( charType(Context), 0 ), Ptrs, false );
    FExtern = Function::Create( FT, Function::ExternalLinkage, "ux_questionc", M );
    // External linkage to the working memory
    std::vector<const Type*> APtrs;
    APtrs.push_back( PointerType::get(charType(Context), 0) );
    APtrs.push_back( intType(Context) );
    FT = FunctionType::get( intType(Context), APtrs, false ); 
    FExtern = Function::Create(FT, Function::ExternalLinkage, "wm_assign", M);
    FExtern = Function::Create(FT, Function::ExternalLinkage, "wm_assignh", M);
    APtrs.clear();
    APtrs.push_back( PointerType::get(charType(Context), 0) );
    APtrs.push_back( Type::getDoubleTy( Context ) );
    FT = FunctionType::get( intType(Context), APtrs, false ); 
    FExtern = Function::Create(FT, Function::ExternalLinkage, "wm_assignd", M);
    APtrs.clear();
    APtrs.push_back( PointerType::get(charType(Context), 0) );
    APtrs.push_back( PointerType::get(charType(Context), 0) );
    FT = FunctionType::get( intType(Context), APtrs, false ); 
    FExtern = Function::Create(FT, Function::ExternalLinkage, "wm_assignc", M);
    // Utility
    std::vector<const Type*> BPtrs( 2, PointerType::get( charType(Context), 0 ) );
    FT = FunctionType::get( intType(Context), BPtrs, false ); 
    FExtern = Function::Create(FT, Function::ExternalLinkage, "strcmp", M);
    //FExtern = Function::Create(FT, Function::ExternalLinkage, "mystrcmp", M);
    // Forward construction of agenda function
    std::string ag_forward("ag_forward");
    Ptrs.clear();
    Ptrs = std::vector<const Type*> ( 1, PointerType::get( charType(Context), 0) );
    FT = FunctionType::get( Type::getVoidTy(Context), Ptrs, false); 
    if( NULL == M->getFunction( ag_forward ) ){
      Function *F = cast<Function>( M->getOrInsertFunction( ag_forward, FT ) );
      Function::arg_iterator args = F->arg_begin();
      Value *x = args; 
      x->setName( "sign" );
    }
  
  }
  
  std::cout << "KBVM v1.0 (experimental)\nIntermediate Representation Generator\n";  
  std::cout << "Parsing from: "
	    << ((domRulesetName == NULL ? 
		 "UNNAMED" :  
		 XMLString::transcode( domRulesetName->getValue() ) ))
	    << ".\n" << domRules->getLength() 
	    << " rules found in set.\n";
  
  unsigned int i;
  char h[ STRINGSIZE ];
  KBIRRuleExpr *node;
  std::map < std::string, std::map< std::string, int > >::const_iterator it;
  std::map < std::string, int >::const_iterator ith;
  BasicBlock *BB;

  // Collect all distinct hypotheses and build template hypothesis functions
  for( i = 0; i < domRules->getLength(); i++ ){
    node = new KBIRRuleExpr( (DOMElement *)domRules->item(i) );
    if( fHypoEncyc.end() == fHypoEncyc.find( std::string( node->getHypo() ) ) ){
      sprintf( h, "%s", node->getHypo() );
      BB = BasicBlock::Create(
	  Context, 
	  "EntryBlock", 
	  fHypoEncyc[ std::string( node->getHypo() ) ] =
	  cast<Function>( M->getOrInsertFunction( std::string( node->getHypo() ),
						  Type::getInt8Ty(Context),
						  (Type *)0 ) ) );
      S_HypoGlobales[ std::string(node->getHypo()) ] = 
	KBIRGenerator::CreateEntryGlobalStringPtr( M, fHypoEncyc[ std::string( node->getHypo() ) ], h, Context);

    }
    delete node;
  }

  // Collect forward links from signs to hypotheses
  for( i = 0; i < domRules->getLength(); i++ ){
    collectFwrdLinks( (DOMElement *)domRules->item(i) );
  }  
  
  std::cout << (int)fHypoEncyc.size() << " hypotheses found in set(s).\n";
  
  /*
  for( it = fFwrdLinks.begin(); it != fFwrdLinks.end(); ++it ){
    std::cout << it->first << "\n";
    for( ith = (it->second).begin(); ith != (it->second).end(); ++ith ){
      std::cout << "\t" << ith->first << " (" << ith->second << ")\n";
    }
  }
  */

  // Each rule translates to one function
  for( i = 0; i < domRules->getLength(); i++ ){
    node = new KBIRRuleExpr( (DOMElement *)domRules->item(i) );
    node->gen( (Function *)NULL, M, Context );
    fEncyc[ std::string( node->getHypo() ) ].push_back( node->getF() );
    delete node;
  }


}

void KBIRGenerator::closeIR( std::string sourceFile ){
  std::map < std::string, std::vector< Function *> >::const_iterator iter;
  // Generate hypothesis functions, or-ing rules leading to said hypothesis
  for( iter = fEncyc.begin(); iter != fEncyc.end(); ++iter ){
    std::cout << iter->first << "\t has " << iter->second.size() << " rules ";
    genHypoFunction( iter );
  }

  genFwrdFunction();
  
  // M->dump();
  // Prints module to file; should use cmd line argument
  std::string ErrorInfo;
  raw_fd_ostream fd( sourceFile.c_str(), ErrorInfo ); 
  M->print( fd, 0 );
  fd.close();
}

/*------------------------------------------------------------------------------
 * Could bwd-chaining and fwd-chaining be implemented as LLVM passes?
 *------------------------------------------------------------------------------
 */


