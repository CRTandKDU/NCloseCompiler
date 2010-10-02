//===--- KBIRExpr.cpp - Abstract NClose KB Expr ---- ---------------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// Abstract representation of a NClose KB expression
//
//===----------------------------------------------------------------------===//
#if !defined(KBVM_INCLUDE_GUARD_KBIRExpr_HPP)
#define KBVM_INCLUDE_GUARD_KBIRExpr_HPP

#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/util/XMLString.hpp>

#include "../../llvm/include/llvm/LLVMContext.h"
#include "llvm/Module.h"

XERCES_CPP_NAMESPACE_USE
using namespace llvm;

/** The generic knowlegde base expression usually subclassed by implementation classes for various expression types such as rules, conditions, actions, tests, etc.
    @brief @b Compiler Generic code generation for NClose statements
*/
class KBIRExpr{
public:
  /** Constructors.
   */
  KBIRExpr( DOMElement *elt ){ dom = elt; };
  /** Destructor.
   */
  ~KBIRExpr(){};

  /** @brief Code generation
      @details This is a virtual method implemented by each subclass of expressions.
      @param F The LLVM Function under construction, NULL not yet ready
      @param M The LLVM Module container
      @param Context The global LLVM Context
  */
  virtual void gen( Function *F, Module *M, LLVMContext &Context ){}; 
  /** Collects metadata and returns global sign string pointer
       @param cst Name of sign
       @param F The LLVM Function in which the code is generated
       @param M The LLVM Module container
       @param Context The global LLVM Context
       @return A pointer to the globally allocated string value
   */
  Value *getOrInsertSign( char *cst, Function *F, Module *M, LLVMContext &Context );
  /** The parsed DOM element for the expression. */
  DOMElement *dom;
};

#endif
