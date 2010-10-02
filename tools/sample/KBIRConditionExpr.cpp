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
#include "KBIRGenerator.hpp"

XERCES_CPP_NAMESPACE_USE
using namespace llvm;

/*------------------------------------------------------------------------------
 * Inlines to navigate DOM subtree rooted at the `dom' DOMElement 
 *------------------------------------------------------------------------------
*/
#define XML_CHILD_TEXT(ELTNAME) (XMLString::transcode(((DOMText *)(dom->getElementsByTagName(XMLString::transcode((ELTNAME)))->item(0)->getFirstChild()))->getData()))

// Operators. See: `kbml.dtd'
const std::string KBIRTestExpr::Sc_OpYes = "yes" ;
const std::string KBIRTestExpr::Sc_OpNo = "no" ;
const std::string KBIRTestExpr::Sc_OpGT = "gt" ;
const std::string KBIRTestExpr::Sc_OpLT = "lt" ;
const std::string KBIRTestExpr::Sc_OpGE = "ge" ;
const std::string KBIRTestExpr::Sc_OpLE = "le" ;
const std::string KBIRTestExpr::Sc_OpEQ = "eq" ;
const std::string KBIRTestExpr::Sc_OpNEQ = "neq" ;
const std::string KBIRTestExpr::Sc_OpIN = "in" ;
const std::string KBIRTestExpr::Sc_OpNIN = "notin" ;


const std::string KBIRTestExpr::S_IdVar ="var";
const std::string KBIRTestExpr::S_IdConst = "const";

void KBIRConditionExpr::gen( Function *F, Module *M, LLVMContext &Context ){
  KBIRTestExpr *node;
  DOMNodeList *domTests = 
    dom->getElementsByTagName( XMLString::transcode("test") );
  unsigned int i;
  BasicBlock *nextBB, *BB = &F->getEntryBlock();
  Value *isOne;

  // std::cout << "Generating Condition IR \n";

  BasicBlock *retBB_F = BasicBlock::Create(Context,"return_false", F);
  KBIRGenerator::S_Builder.SetInsertPoint( retBB_F );
  KBIRGenerator::S_Builder.CreateRet( KBIRGenerator::S_Zero );

  KBIRGenerator::S_Builder.SetInsertPoint(BB);
  // Pre-evaluates the rule, seeking for a known FALSE test in LHS
  for( i = 0; i < domTests->getLength(); i++ ){
    node = new KBIRTestExpr( (DOMElement *)domTests->item(i) );
    isOne = 
      KBIRGenerator::S_Builder.CreateICmpEQ( node->genPrologue(F, M, Context), 
					     KBIRGenerator::S_One, 
					     "prologue_isOne_gen");
    nextBB = BasicBlock::Create( Context, "next_prologue", F );
    KBIRGenerator::S_Builder.CreateCondBr( isOne, nextBB, retBB_F );
    KBIRGenerator::S_Builder.SetInsertPoint( nextBB );
    delete node;
  }
  // Ands the sequence of test evaluations in the source LHS order
  for( i = 0; i < domTests->getLength(); i++ ){
    node = new KBIRTestExpr( (DOMElement *)domTests->item(i) );
    isOne = KBIRGenerator::S_Builder.CreateICmpEQ( node->genBody(F, M, Context), KBIRGenerator::S_One, "if_cond");
    nextBB = BasicBlock::Create( Context, "next", F );
    KBIRGenerator::S_Builder.CreateCondBr( isOne, nextBB, retBB_F );
    KBIRGenerator::S_Builder.SetInsertPoint( nextBB );
    delete node;
  }
  // Ignore return value
  return;
}

Value *KBIRTestExpr::genSet( bool ag, Function *F, Module *M, LLVMContext &Context ){
  // Set tests are multi-argument relations with the (positional) first one LHS
  BasicBlock *BB = KBIRGenerator::S_Builder.GetInsertBlock();
  char *s, *cst;
  std::vector<Value *> args;
  std::vector<Value *> operands;

  DOMNode *cur;
  Value *operand;

  KBIRGenerator::S_Builder.SetInsertPoint( BB );

  // First positional arg
  for( cur = dom->getFirstChild(); cur; cur = cur->getNextSibling() ){
    s = XMLString::transcode(cur->getNodeName());

    if( 0 == S_IdVar.compare( s ) ){
      args.clear();
      cst = XMLString::transcode(((DOMText *)(cur->getFirstChild()))->getData());
      //std::cout << "\t" << s << ", " << cst << std::endl;
      // Collect global variable as sign
      args.push_back( getOrInsertSign( cst, F, M, Context ) );
      if( ag ) 	KBIRGenerator::S_Builder.CreateCall( M->getFunction("ag_forward"),
				      args.begin(), args.end() );
      operand = KBIRGenerator::S_Builder.CreateCall(M->getFunction("ux_questionc"), 
				     args.begin(),
				     args.end(),
				     "questionc_datastore");
      break;
    }
    else if( 0 == S_IdConst.compare( s ) ){
      cst = 
	XMLString::transcode(((DOMText *)(cur->getFirstChild()))->getData());
      operand = KBIRGenerator::CreateEntryGlobalStringPtr( M, F, cst,Context);
      break;
    }
    else{
      // A node text ?
    }
  }

  // Variable number of operands after the first one
  for( cur = cur->getNextSibling(); cur; cur = cur->getNextSibling() ){
    s = XMLString::transcode(cur->getNodeName());
    if( 0 == S_IdVar.compare( s ) ){
      args.clear();
      cst = 
	XMLString::transcode(((DOMText *)(cur->getFirstChild()))->getData());
      args.push_back( getOrInsertSign( cst, F, M, Context ) );
      if( ag ) 	KBIRGenerator::S_Builder.CreateCall( M->getFunction("ag_forward"),
				      args.begin(), args.end() );
      operands.push_back( KBIRGenerator::S_Builder.CreateCall(M->getFunction("ux_questionc"), 
					       args.begin(),
					       args.end(),
					       "questionc_datastore") );
    }
    else if( 0 == S_IdConst.compare( s ) ){
      cst = 
	XMLString::transcode(((DOMText *)(cur->getFirstChild()))->getData());
      operands.push_back( KBIRGenerator::CreateEntryGlobalStringPtr( M, F, cst, Context) );
    }
    else{
      // Future extension mechanism
    }
  }
  // Code generation
  BasicBlock *retBB_T, *nextBB, *exitBB = BasicBlock::Create( Context, "set_out", F );
  std::vector<Value *> callArgs;
  std::vector< Value * >::const_iterator it;
  Value *inst, *isZero;
  std::string op = 
    XMLString::transcode( dom->getAttribute( XMLString::transcode("op") ) );
  AllocaInst *Alloca = 
    KBIRGenerator::CreateEntryBlockAlloca(F, 
					  std::string("set_membership"), 
					  Context);

  // Lazy evaluation of membership

    for( it = operands.begin(); it != operands.end(); ++it ){
      callArgs.clear();
      // Compares strings (sign names)
      callArgs.push_back( operand );
      callArgs.push_back( *it );
      inst = 
	KBIRGenerator::S_Builder.CreateCall( M->getFunction( "strcmp" ), 
			      callArgs.begin(), 
			      callArgs.end(), "set_strcmp" );
      inst =
	KBIRGenerator::S_Builder.CreateIntCast( inst, 
				 Type::getInt8Ty(Context), 
				 false, "cast_strcmp" );
      isZero = KBIRGenerator::S_Builder.CreateICmpEQ( inst, KBIRGenerator::S_Zero, "eq");
      // Next blocks to chain to
      nextBB = BasicBlock::Create( Context, "set_chain", F );
      retBB_T = BasicBlock::Create( Context, "set_found", F );
      KBIRGenerator::S_Builder.CreateCondBr( isZero, retBB_T, nextBB );
      // Block: Found match, returns 1
      KBIRGenerator::S_Builder.SetInsertPoint( retBB_T );
      if( 0 == op.compare( Sc_OpIN ) ){
	KBIRGenerator::S_Builder.CreateStore( KBIRGenerator::S_One, Alloca );
	KBIRGenerator::S_Builder.CreateBr( exitBB );
      }
      else if( 0 == op.compare( Sc_OpNIN ) ){
	KBIRGenerator::S_Builder.CreateStore( KBIRGenerator::S_Zero, Alloca );
	KBIRGenerator::S_Builder.CreateBr( exitBB );
      }
      else{
	// Future stuff
      }
      // Move insertion point to next test
      KBIRGenerator::S_Builder.SetInsertPoint( nextBB );
    }
    // Went through the whole list of operands
    if( 0 == op.compare( Sc_OpIN ) ){
      KBIRGenerator::S_Builder.CreateStore( KBIRGenerator::S_Zero, Alloca );
      KBIRGenerator::S_Builder.CreateBr( exitBB );
    }
    else if( 0 == op.compare( Sc_OpNIN ) ){
      KBIRGenerator::S_Builder.CreateStore( KBIRGenerator::S_One, Alloca );
      KBIRGenerator::S_Builder.CreateBr( exitBB );
    }
    else{
      // Future stuff
    }
    //    KBIRGenerator::S_Builder.CreateBr( exitBB );
    
    // Return to caller
    KBIRGenerator::S_Builder.SetInsertPoint( exitBB );
    inst = KBIRGenerator::S_Builder.CreateLoad( Alloca );
    return inst;
}

Value *KBIRTestExpr::genComp( bool ag, Function *F, Module *M, LLVMContext &Context ){
  // Comparison tests: apply to numerical signs
  short i;
  char *s, *cst; 
  DOMNode *cur;
  BasicBlock *BB = KBIRGenerator::S_Builder.GetInsertBlock();
  Value *res[2];
  std::vector<Value *> args;

  //std::cout << "In CompNum test\n";
  KBIRGenerator::S_Builder.SetInsertPoint(BB);
  for( i = 0, cur = dom->getFirstChild(); cur; cur = cur->getNextSibling() ){
    s = XMLString::transcode(cur->getNodeName());
    // A numerical variable
    if( 0 == S_IdVar.compare( s ) ){
      args.clear();
      cst = 
	XMLString::transcode(((DOMText *)(cur->getFirstChild()))->getData());
      //std::cout << "\t" << s << ", " << cst << std::endl;
      // Collect global variable as sign
      args.push_back( getOrInsertSign( cst, F, M, Context ) );
      if( ag ) 	KBIRGenerator::S_Builder.CreateCall( M->getFunction("ag_forward"),
				      args.begin(), args.end() );
      res[i++] = KBIRGenerator::S_Builder.CreateCall(M->getFunction("ux_questiond"),
			 args.begin(),args.end(),"questiond_datastore");
    }
    // A numerical constant
    else if( 0 == S_IdConst.compare( s ) ){
      double d = 0.0;
      cst = 
	XMLString::transcode(((DOMText *)(cur->getFirstChild()))->getData());
      //std::cout << "\t" << s << ", " << cst << std::endl;
      d = strtod( cst, NULL );
      res[i++] = ConstantFP::get( Type::getDoubleTy(Context), d );
    }
    else{
      // Future extension mechanism
    }
  }

  // Comparison test
  AllocaInst *Alloca = 
    KBIRGenerator::CreateEntryBlockAlloca(F, std::string("comp_result") + XML_CHILD_TEXT("var"), 
					  Context);
  BasicBlock *retBB_T = BasicBlock::Create(Context,"return_true", F);
  BasicBlock *retBB_F = BasicBlock::Create(Context,"return_false", F);
  BasicBlock *mergeBB = BasicBlock::Create(Context,"if_continuation", F);
  Value *resTest, *resTestZero, *resTestOne;
  std::string op = 
    XMLString::transcode( dom->getAttribute( XMLString::transcode("op") ) );

  if( 0 == op.compare( Sc_OpGT ) ){
    resTest = KBIRGenerator::S_Builder.CreateFCmpOGT( res[0], res[1], "num_comp" );
  }
  else if( 0 == op.compare( Sc_OpLT ) ){
    resTest = KBIRGenerator::S_Builder.CreateFCmpOLT( res[0], res[1], "num_comp" );
  }
  else if( 0 == op.compare( Sc_OpGE ) ){
    resTest = KBIRGenerator::S_Builder.CreateFCmpOGE( res[0], res[1], "num_comp" );
  }
  else if( 0 == op.compare( Sc_OpLE ) ){
    resTest = KBIRGenerator::S_Builder.CreateFCmpOLE( res[0], res[1], "num_comp" );
  }
  else if( 0 == op.compare( Sc_OpEQ ) ){
    resTest = KBIRGenerator::S_Builder.CreateFCmpOEQ( res[0], res[1], "num_comp" );
  }
  else if( 0 == op.compare( Sc_OpNEQ ) ){
    resTest = KBIRGenerator::S_Builder.CreateFCmpONE( res[0], res[1], "num_comp" );
  }
  KBIRGenerator::S_Builder.CreateCondBr( resTest, retBB_T, retBB_F );
 
  KBIRGenerator::S_Builder.SetInsertPoint( retBB_F );
  KBIRGenerator::S_Builder.CreateStore( KBIRGenerator::S_Zero, Alloca );
  resTestZero = KBIRGenerator::S_Builder.CreateLoad( Alloca );
  KBIRGenerator::S_Builder.CreateBr( mergeBB );

  KBIRGenerator::S_Builder.SetInsertPoint( retBB_T );
  KBIRGenerator::S_Builder.CreateStore( KBIRGenerator::S_One, Alloca );
  resTestOne = KBIRGenerator::S_Builder.CreateLoad( Alloca );
  KBIRGenerator::S_Builder.CreateBr( mergeBB );
  
  KBIRGenerator::S_Builder.SetInsertPoint( mergeBB );
  PHINode *PN = KBIRGenerator::S_Builder.CreatePHI(Type::getInt8Ty(getGlobalContext()), "merge");
  PN->addIncoming(resTestOne, retBB_T);
  PN->addIncoming(resTestZero, retBB_F);
  return PN;
}

Value *KBIRTestExpr::genBool( bool ag, Function *F, Module *M, LLVMContext &Context ){
  std::vector<Value *> args, qargs;
  Value *res, *isUnKnown, *res_T, *res_F;
  BasicBlock *BB = KBIRGenerator::S_Builder.GetInsertBlock();

  // Boolean tests: apply to both boolean signs and to hypotheses
  std::string sign( XML_CHILD_TEXT("var") );

  //std::cout << "In Boolean Test: " << XML_CHILD_TEXT("var") << "\n";
  // Test for hypothesis name (based on the existence of named Function in M)
  if( M->getFunction( sign ) ){
    // Collect global hypothesis as sign
      if( KBIRGenerator::S_SignGlobales.end() == KBIRGenerator::S_SignGlobales.find( sign ) ){
      KBIRGenerator::S_SignGlobales[sign] = KBIRGenerator::S_HypoGlobales[sign];
    }
    // Generates code to evaluate hypothesis
      /*
    AllocaInst *Alloca = 
      KBIRGenerator::CreateEntryBlockAlloca(F, std::string("known"), Context);
      */
    BasicBlock *retBB_T = BasicBlock::Create(Context,
					     ag ? "return_true" : "prol_T", 
					     F);
    BasicBlock *retBB_F = BasicBlock::Create(Context, 
					     ag ? "return_false" : "prol_F", 
					     F);
    BasicBlock *mergeBB = BasicBlock::Create(Context, 
					     ag ? "if_continuation" : "prol_C",
					     F);
    // Block: Hypothesis is known, ask question to the datastore
    KBIRGenerator::S_Builder.SetInsertPoint( retBB_F );
    args.push_back(KBIRGenerator::S_HypoGlobales[sign]);
    res = 
      KBIRGenerator::S_Builder.CreateCall(M->getFunction("ux_question"),
			   args.begin(),args.end(),
			   ag ? "question_datastore" : "prol_Q" );
    res_F =
      KBIRGenerator::S_Builder.CreateIntCast(res, Type::getInt8Ty(Context), false, 
			      ag ? "cast_val" : "prol_cast" );
    //    KBIRGenerator::S_Builder.CreateStore( res, Alloca );
    // res = KBIRGenerator::S_Builder.CreateLoad( Alloca );
    KBIRGenerator::S_Builder.CreateBr( mergeBB );

    // Block: Hypothesis still unknown, use immediate backward chaining
    KBIRGenerator::S_Builder.SetInsertPoint( retBB_T );
    res_T = KBIRGenerator::S_Builder.CreateCall( M->getFunction( sign ), "backward_chaining" );
    /*
    KBIRGenerator::S_Builder.CreateStore( res, Alloca );
    res_T = KBIRGenerator::S_Builder.CreateLoad( Alloca );
    */
    KBIRGenerator::S_Builder.CreateBr( mergeBB );

    // Back to first block:  Test if Hypo is known
    KBIRGenerator::S_Builder.SetInsertPoint(BB);
    qargs.push_back( KBIRGenerator::S_HypoGlobales[sign] );
    // Push forward hypotheses to the agenda
    if( ag ) 	KBIRGenerator::S_Builder.CreateCall( M->getFunction("ag_forward"),
				      qargs.begin(), qargs.end() );
    // Test 
    res = KBIRGenerator::S_Builder.CreateCall( M->getFunction( "wm_known" ),
			      qargs.begin(), qargs.end(), "query_datastore" );
    res =
      KBIRGenerator::S_Builder.CreateIntCast( res, Type::getInt8Ty(Context), false, "cast_val" );
    isUnKnown = KBIRGenerator::S_Builder.CreateICmpEQ( res, KBIRGenerator::S_Zero, "if_cond");
    // If unknown, backward chain, if known, use value from store
    KBIRGenerator::S_Builder.CreateCondBr( isUnKnown, retBB_T, retBB_F );

    // Joining the two branches of previous IF
    KBIRGenerator::S_Builder.SetInsertPoint( mergeBB );
    PHINode *PN = KBIRGenerator::S_Builder.CreatePHI(Type::getInt8Ty(getGlobalContext()),
				      "merge");
    PN->addIncoming(res_T, retBB_T);
    PN->addIncoming(res_F, retBB_F);
    res = PN;
  }
  // Otherwise it is a variable/sign
  else{
    Value *arg;
    // Collect global variable as sign
    KBIRGenerator::S_Builder.SetInsertPoint(BB);
    if( KBIRGenerator::S_SignGlobales.end() == KBIRGenerator::S_SignGlobales.find( sign ) ){
      arg = KBIRGenerator::S_SignGlobales[sign] = KBIRGenerator::CreateEntryGlobalStringPtr( M, F,XML_CHILD_TEXT("var"),Context);
    }
    else{
      arg = KBIRGenerator::S_SignGlobales[sign];
    }

    args.clear();
    args.push_back( arg );
    // Push forward hypotheses onto agenda
    if( ag ) 	KBIRGenerator::S_Builder.CreateCall( M->getFunction("ag_forward"),
				      args.begin(), args.end() );
    res = 
      KBIRGenerator::S_Builder.CreateCall(M->getFunction("ux_question"),
			 args.begin(),args.end(),"question_datastore");
    res =
      KBIRGenerator::S_Builder.CreateIntCast( res, Type::getInt8Ty(Context), false, "cast_val" );
  }

  // Returns 0 or 1 according to operator 
  if( 0 == Sc_OpYes.compare( XMLString::transcode( dom->getAttribute( XMLString::transcode("op") ) ) ) ){
    return res;
  }
  else{
    return KBIRGenerator::S_Builder.CreateSub( ConstantInt::get(Type::getInt8Ty(Context), 1) ,res, "op_no" );
  }
}

Value *KBIRTestExpr::genBody( Function *F, Module *M, LLVMContext &Context ){
  char *s = 
    XMLString::transcode( dom->getAttribute( XMLString::transcode("op") ) );
  if( S_OpBoolp( s ) ) return genBool( true, F, M, Context );
  if( S_OpCompp( s ) ) return genComp( true, F, M, Context );
  if( S_OpStrp( s ) ) return genSet( true, F, M, Context );
  return KBIRGenerator::S_Zero;
}

Value *KBIRTestExpr::genPrologue( Function *F, Module *M, LLVMContext &Context ){
  std::vector<Value *> args, qargs;
  Value *res, *res_F, *res_T, *isOne, *isUnKnown;
  BasicBlock *BB = KBIRGenerator::S_Builder.GetInsertBlock();
  AllocaInst *Alloca, *AllocaRes;
  BasicBlock *nextBB, *retBB_T, *retBB_F, *Phi_retBB_F, *mergeBB, *outBB;
  short i;
  DOMNode *cur;
  char *cst, *s = 
    XMLString::transcode( dom->getAttribute( XMLString::transcode("op") ) );

  if( S_OpBoolp( s ) ){
    Alloca = 
      KBIRGenerator::CreateEntryBlockAlloca(F, 
					    std::string("known") + XML_CHILD_TEXT("var"),
					    Context);
    retBB_T = BasicBlock::Create(Context,"return_true", F);
    retBB_F = BasicBlock::Create(Context,"return_false", F);
    mergeBB = BasicBlock::Create(Context,"if_continuation", F);

    // Block: If variable is known then generate the test code
    KBIRGenerator::S_Builder.SetInsertPoint( retBB_F );
    res_F = genBool( false, F, M, Context );
    // KBIRGenerator::S_Builder.CreateStore( res, Alloca );
    KBIRGenerator::S_Builder.CreateBr( mergeBB );
    Phi_retBB_F = KBIRGenerator::S_Builder.GetInsertBlock(); // `genBool' changes the insert point

    // Block: If not, return 1 and pass
    KBIRGenerator::S_Builder.SetInsertPoint( retBB_T );
    KBIRGenerator::S_Builder.CreateStore( KBIRGenerator::S_One, Alloca );
    res_T = KBIRGenerator::S_Builder.CreateLoad( Alloca );
    KBIRGenerator::S_Builder.CreateBr( mergeBB );

    // Back to first Block:  Test if hypothesis is known
    KBIRGenerator::S_Builder.SetInsertPoint(BB);
    qargs.push_back( KBIRGenerator::CreateEntryGlobalStringPtr( M, F,XML_CHILD_TEXT("var"),Context) );
    res = KBIRGenerator::S_Builder.CreateCall( M->getFunction( "wm_known" ),
			      qargs.begin(), qargs.end(), "pro_query_datastore" );
    res =
      KBIRGenerator::S_Builder.CreateIntCast( res, Type::getInt8Ty(Context), false, "cast_val" );
    isUnKnown = KBIRGenerator::S_Builder.CreateICmpEQ( res, KBIRGenerator::S_Zero, "if_unknown");
    KBIRGenerator::S_Builder.CreateCondBr( isUnKnown, retBB_T, retBB_F );

    // Joining the two branches of previous IF
    KBIRGenerator::S_Builder.SetInsertPoint( mergeBB );
    PHINode *PN = 
      KBIRGenerator::S_Builder.CreatePHI(Type::getInt8Ty(getGlobalContext()),
					 "if_unknown_bool");
    PN->addIncoming(res_T, retBB_T);
    PN->addIncoming(res_F, Phi_retBB_F);
    return PN;
  }
  
  if( S_OpCompp( s ) ){
    Alloca = 
      KBIRGenerator::CreateEntryBlockAlloca(F, std::string("known_")+ XML_CHILD_TEXT("var"), Context);
    AllocaRes = 
      KBIRGenerator::CreateEntryBlockAlloca(F, std::string("prol_res_")+ XML_CHILD_TEXT("var"), Context);
    retBB_F = BasicBlock::Create(Context,"return_false", F);
    outBB = BasicBlock::Create(Context,"out", F);

    KBIRGenerator::S_Builder.SetInsertPoint( retBB_F );
    res = 
      KBIRGenerator::S_Builder.CreateStore( KBIRGenerator::S_One, AllocaRes );
    KBIRGenerator::S_Builder.CreateBr( outBB );

    KBIRGenerator::S_Builder.SetInsertPoint( BB );
    for( i = 0, cur = dom->getFirstChild(); cur; cur = cur->getNextSibling() ){
      s = XMLString::transcode(cur->getNodeName());
      // A numerical variable
      if( 0 == S_IdVar.compare( s ) ){
	nextBB = BasicBlock::Create( Context, "next", F );
	cst = 
	  XMLString::transcode(((DOMText *)(cur->getFirstChild()))->getData());
	qargs.clear();
	qargs.push_back( KBIRGenerator::CreateEntryGlobalStringPtr( M, F, cst, Context) );
	res = KBIRGenerator::S_Builder.CreateCall( M->getFunction( "wm_known" ),
				    qargs.begin(), qargs.end(), 
				    "query_datastore" );
	res =
	  KBIRGenerator::S_Builder.CreateIntCast( res, Type::getInt8Ty(Context), false, 
				   "cast_val" );
	isOne = KBIRGenerator::S_Builder.CreateICmpEQ( res, KBIRGenerator::S_One, "if_cond");
	KBIRGenerator::S_Builder.CreateCondBr( isOne, nextBB, retBB_F );
	KBIRGenerator::S_Builder.SetInsertPoint( nextBB );
      }
    }
    
    res = genComp( false, F, M, Context );
    KBIRGenerator::S_Builder.CreateStore( res, AllocaRes );
    KBIRGenerator::S_Builder.CreateBr( outBB );

    KBIRGenerator::S_Builder.SetInsertPoint( outBB );
    res = KBIRGenerator::S_Builder.CreateLoad( AllocaRes );
    return res;
  }

  if( S_OpStrp( s ) ){
    Alloca = 
      KBIRGenerator::CreateEntryBlockAlloca(F, std::string("known_s_")+ XML_CHILD_TEXT("var"), Context);
    AllocaRes = 
      KBIRGenerator::CreateEntryBlockAlloca(F, std::string("prol_s_res_")+ XML_CHILD_TEXT("var"), Context);
    retBB_F = BasicBlock::Create(Context,"return_false_s", F);
    outBB = BasicBlock::Create(Context,"out_s", F);
    
    // Prepares block to call when at least one var in test is still unknown
    KBIRGenerator::S_Builder.SetInsertPoint( retBB_F );
    res = 
      KBIRGenerator::S_Builder.CreateStore( KBIRGenerator::S_One, AllocaRes );
    KBIRGenerator::S_Builder.CreateBr( outBB );

    // Check if all vars in test are known, otherwise exit through retBB_F
    KBIRGenerator::S_Builder.SetInsertPoint( BB );
    for( i = 0, cur = dom->getFirstChild(); cur; cur = cur->getNextSibling() ){
      s = XMLString::transcode(cur->getNodeName());
      // A string variable
      if( 0 == S_IdVar.compare( s ) ){
	nextBB = BasicBlock::Create( Context, "next_s", F );
	cst = 
	  XMLString::transcode(((DOMText *)(cur->getFirstChild()))->getData());
	qargs.clear();
	qargs.push_back( KBIRGenerator::CreateEntryGlobalStringPtr( M, F, cst, Context) );
	res = KBIRGenerator::S_Builder.CreateCall( M->getFunction( "wm_known" ),
				    qargs.begin(), qargs.end(), 
				    "query_datastore" );
	res =
	  KBIRGenerator::S_Builder.CreateIntCast( res, Type::getInt8Ty(Context), false, "cast_val" );
	isOne = KBIRGenerator::S_Builder.CreateICmpEQ( res, KBIRGenerator::S_One, "if_cond");
	KBIRGenerator::S_Builder.CreateCondBr( isOne, nextBB, retBB_F );
	KBIRGenerator::S_Builder.SetInsertPoint( nextBB );
      }
    }

    // All vars in test are known, perform test and returns its result
    res = genSet( false, F, M, Context );
    KBIRGenerator::S_Builder.CreateStore( res, AllocaRes );
    KBIRGenerator::S_Builder.CreateBr( outBB );

    KBIRGenerator::S_Builder.SetInsertPoint( outBB );
    res = KBIRGenerator::S_Builder.CreateLoad( AllocaRes );
    return res;
  }

  return KBIRGenerator::S_Zero;
}
