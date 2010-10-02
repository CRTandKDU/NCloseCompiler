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
#if !defined(KBVM_INCLUDE_GUARD_KBIRGenerator_HPP)
#define KBVM_INCLUDE_GUARD_KBIRGenerator_HPP

#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/LLVMContext.h"
#include "../../llvm/include/llvm/Support/IRBuilder.h"

#include <string>
#include <map>

XERCES_CPP_NAMESPACE_USE
using namespace llvm;

/** The LLVM IR generator class.  Generates LLVM assembly code for the XML knowledge bases while collecting metadata (list of hypotheses and forward links).
    @brief @b Compiler Code generation for a knowledge base
 */
class KBIRGenerator {
public:
  /** Constructors */
  KBIRGenerator( DOMDocument * );
  KBIRGenerator();
  /** Destructor */
  ~KBIRGenerator();
  
  /** @brief (Re)Associates a DOM document to generator.
      @param doc A DOM Document as parsed by Xerces 
  */
  void setDoc( DOMDocument *doc );
  
  /**    Intermediate Representation Generation and metadata accumulation.  Metadata consists of (i) a map of hypotheses to their global string value; (ii) a map of signs to their forwarded hypotheses, in the form of a `ag_forward' function which is unique to the module.  This function can be called several times on different XML knowledge base files to accumulate rules in the same LLVM module.
	 @brief Generates IR and accumulates metadata for LLVM
  */
  void generateIR();
  

  /** Closes code generation completing the module with (i) the hypotheses functions for all cumulated hypotheses; and (ii) the body of the module `ag_forward' function for forward chaining.  This function is called once, at the end of the compilation: it readies the code for execution.
      @brief Closes metadata accumulation, save and ready to execute
      @param sourceFile The output file for the source code assembly
  */
  void closeIR( std::string sourceFile );

  static IRBuilder<>  S_Builder;
  static Value *S_One;
  static Value *S_Zero;

  /** The metadata map from hypothesis names to their global string implement */
  static std::map< std::string, Value * > S_HypoGlobales;
  /** The metadat map from signs to their global string implementation */
  static std::map< std::string, Value * > S_SignGlobales;

  /** @brief Constructs integer type based on `int' size
      @param Context A LLVM Contex
      @return A LLVM Integer Type for `int' on this machine
  */
  static const IntegerType *intType(LLVMContext &Context){
    return IntegerType::get(Context, sizeof(int) * CHAR_BIT);
  }

  /** @brief Constructs char type based on `char'
      @param Context A LLVM Contex
      @return A LLVM Integer Type for `char' on this machine
  */
  static const IntegerType *charType(LLVMContext &Context){
    return IntegerType::get(Context, CHAR_BIT);
  }

  /** @brief Reports an error
      @param Str Error mesasge to print out
  */
  static void Error(const char *Str) { 
    fprintf(stderr, "Error: %s\n", Str);
  }

  /** @brief Creates a local variable store in a Function
      @param TheFunction Function scoping the variable
      @param VarName Name to be used in code
      @param Context LLVM Context
      @return The allocation instruction for futher `Load' and `Store' ops
  */
  static AllocaInst *CreateEntryBlockAlloca(Function *TheFunction,
					    const std::string &VarName,
					    LLVMContext &Context) {
    IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
		     TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(Type::getInt8Ty(Context), 0, VarName.c_str());
  }

  /** @brief Creates a (unique) global string variable
      @param TheModule Module scoping the generation
      @param TheFunction Function scoping the variable
      @param name Name to be used in code
      @param Context LLVM Context
      @return The LLVM pointer to the global string
  */
  static Value *CreateEntryGlobalStringPtr(Module *TheModule,
					   Function *TheFunction,
					   char *name,
					   LLVMContext &Context) {
    IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
		     TheFunction->getEntryBlock().begin());
    GlobalVariable *gv = TheModule->getGlobalVariable( StringRef(name) , true );
    if( NULL != gv  ){
      // printf( "Found %s\n", name );
      Value *zero = ConstantInt::get(Type::getInt32Ty(Context), 0);
      Value *Args[] = { zero, zero };
      return TmpB.CreateInBoundsGEP(gv, Args, Args+2, name);
    }
    else{
      return TmpB.CreateGlobalStringPtr( name, name );
    }
  }


private:
  /** A map of list of functions representing rules leading to same hypothesis.
   */
  std::map < std::string, std::vector< Function *> > fEncyc;
  /** A map of hypothesis functions in generated assembly code.
   */
  std::map < std::string, Function *> fHypoEncyc;
  /** A map of forward-chaining links leading from sign to hypotheses.
   */
  std::map < std::string, std::map< std::string, int > > fFwrdLinks;
  /** Current DOM Document  */
   DOMDocument *domDoc;
  /** Counter for IR function names  */
  int count;
  /** LLVM Module to contain generated code  */
  Module *M;
  

  /** Each hypothesis translates to a function calling sequentially all the subfunctions for rules leading to the hypothesis.  This is done once all the rules have been seen and compiled, i.e. at `closeIR' time.
      @brief Generates IR code representing a hypothesis
      @param iterator An iterator on the map `fEncyc' keeping track of rule functions for the same hypothesis
   */
  void genHypoFunction( std::map < std::string, std::vector< Function *> >::const_iterator iterator );

  /** Collects forward links from signs to hypotheses in `fFwrdLinks' by parsing the variables in the body of XML rules.
      @brief Collects forward links
      @param dom The Document element
  */
  void collectFwrdLinks( DOMElement *dom );

  /** Generates the module-scoped forward function pushing forward hypotheses on the agenda.  This is done when all rules have been seen and compiled, i.e. at `closeIR' time.
   */
  void genFwrdFunction();

};

#endif
