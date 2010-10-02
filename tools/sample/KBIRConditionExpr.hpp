//===--- KBIRConditionExpr.cpp - Rule LHS Generator -----------------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// Generates IR code for the LHS of a NClose rule
//
//===----------------------------------------------------------------------===//
#if !defined(KBVM_INCLUDE_GUARD_KBIRConditionExpr_HPP)
#define KBVM_INCLUDE_GUARD_KBIRConditionExpr_HPP

#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/util/XMLString.hpp>

#include "llvm/DerivedTypes.h"
#include "../../llvm/include/llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Value.h"

#include "KBIRExpr.hpp"

XERCES_CPP_NAMESPACE_USE
using namespace llvm;

/** Expression implementation class for conditions in a rule.
    @brief @b Compiler Code generation for LHS conditions
*/
class KBIRConditionExpr : public KBIRExpr{
public:
  KBIRConditionExpr( DOMElement *elt ) : KBIRExpr( elt) {};
  ~KBIRConditionExpr(){};
  /** Generates code for a single condition in a rule LHS.
      @param F The LLVM Function in which the code is generated
      @param M The LLVM Module container
      @param Context The global LLVM Context
      @return The generated code as a LLVM Value*
  */
  void gen( Function *F, Module *M, LLVMContext &Context );
};



/**
   Expression implementation class for test in a condition.
   @brief @b Compiler Code generation for tests in LHS
*/
class KBIRTestExpr : public KBIRExpr {
public:
  KBIRTestExpr( DOMElement *elt ) : KBIRExpr( elt) {};
  ~KBIRTestExpr(){};



  /** The prologue, present in each rule function, looks for known signs in the LHS that do not match the test in which they appear.  In this situation, the LHS evaluation aborts early to `false', or else restarts sequential evaluation of each test in the LHS from the first one.
      @brief Prologue generation in LHS.
      @param F The LLVM Function in which the code is generated
      @param M The LLVM Module container
      @param Context The global LLVM Context
      @return The generated code as a LLVM Value*
  */
  Value *genPrologue( Function *F, Module *M, LLVMContext &Context );
  /** The body of the function rule, executed after the prologue, basically sifts through each test in sequence until one fails or all are passed.
      @brief Generated LLVM IR code for tests in LHS
      @param F The LLVM Function in which the code is generated
      @param M The LLVM Module container
      @param Context The global LLVM Context
      @return The generated code as a LLVM Value*
  */
  Value *genBody( Function *F, Module *M, LLVMContext &Context );

  /** @brief Generates code for a test in a condition.
      @param F The LLVM Function in which the code is generated
      @param M The LLVM Module container
      @param Context The global LLVM Context
      @return The generated code as a LLVM Value*
   */
  void gen( Function *F, Module *M, LLVMContext &Context ){ return; };

  /** @brief @b Operators. See: `kbml.dtd' */
  static const std::string Sc_OpYes;
  static const std::string Sc_OpNo;
  static const std::string Sc_OpGT;
  static const std::string Sc_OpLT;
  static const std::string Sc_OpGE;
  static const std::string Sc_OpLE;
  static const std::string Sc_OpEQ;
  static const std::string Sc_OpNEQ;
  static const std::string Sc_OpIN;
  static const std::string Sc_OpNIN;
  /** @brief @b Identifiers. See: `kbml.dtd' */
  static const std::string S_IdVar;
  static const std::string S_IdConst;


  static const int S_OpBoolp( char *s ){
    std::string op( s );
    return ( op == Sc_OpYes || op == Sc_OpNo ) ? 1 : 0;
  };


  static const int S_OpCompp( char *s ){
    std::string op( s );
    return ( op == Sc_OpGT || op == Sc_OpLT ||
	     op == Sc_OpGE || op == Sc_OpLE ||
	     op == Sc_OpEQ || op == Sc_OpNEQ ) ? 1 : 0;
  };

  static const int S_OpStrp( char *s ){
    std::string op( s );
    return ( op == Sc_OpIN || op == Sc_OpNIN ) ? 1 : 0;
  };


private:
  /** Generates code for testing a boolean sign, with special handling of hypotheses.
      @param ag Boolean set to false when generating prologue code
      @param F The LLVM Function in which the code is generated
      @param M The LLVM Module container
      @param Context The global LLVM Context
      @return The generated code as a LLVM Value*
   */
  Value *genBool( bool ag, Function *F, Module *M, LLVMContext &Context );
  /** Generates code for testing numerical data.
       @param ag Boolean set to false when generating prologue code
       @param F The LLVM Function in which the code is generated
       @param M The LLVM Module container
       @param Context The global LLVM Context
       @return The generated code as a LLVM Value*
   */
  Value *genComp( bool ag, Function *F, Module *M, LLVMContext &Context );
  /** Generates code for testing char data.
       @param ag Boolean set to false when generating prologue code
       @param F The LLVM Function in which the code is generated
       @param M The LLVM Module container
       @param Context The global LLVM Context
       @return The generated code as a LLVM Value*
   */
  Value *genSet( bool ag, Function *F, Module *M, LLVMContext &Context );


};


#endif
