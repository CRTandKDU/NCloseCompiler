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
#if !defined(KBVM_INCLUDE_GUARD_KBIRActionExpr_HPP)
#define KBVM_INCLUDE_GUARD_KBIRActionExpr_HPP

#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/util/XMLString.hpp>

#include "../../llvm/include/llvm/LLVMContext.h"
#include "llvm/Module.h"

#include "KBIRExpr.hpp"

XERCES_CPP_NAMESPACE_USE
using namespace llvm;

/** @brief @b Compiler Code generation for RHS commands*/
class KBIRActionExpr : public KBIRExpr{
public:
  /** Constructor */
  KBIRActionExpr( DOMElement *elt ) : KBIRExpr( elt ) { };
  /** Destructor */
  ~KBIRActionExpr(){};

  /** Generates IR code for RHS
      @brief Code generation for commands in the RHS
      @param F The LLVM Function in which the code is generated
      @param M The LLVM Module container
      @param Context The global LLVM Context
      @return The generated code as a LLVM Value*
  */
  void gen( Function *F, Module *M, LLVMContext &Context );

  /** @brief Constant boolean `true' value for set family of commands.  Anyhting else is `false'. */
  static const std::string Sc_IdTrue;
  /** @brief Op codes for the typed `set' commands */
  static const std::string Sc_OpSetB;
  static const std::string Sc_OpSetD;
  static const std::string Sc_OpSetS;
  
  /** @brief Sign identifiers.  See: `kbml.dtd' */
  static const std::string S_IdVar;
  /** @brief Constant identifiers.  See: `kbml.dtd' */
  static const std::string S_IdConst;

  /** @brief Is command a `set' command? */
  static const int S_OpCmdSet( const char *s ){
    std::string op( s );
    return ( op == Sc_OpSetB || op == Sc_OpSetD || op == Sc_OpSetS  ) ? 1 : 0;
  }

};

#endif
