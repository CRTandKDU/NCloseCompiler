//===--- KBIRRunner.cpp - The NClose interpreter --------------------------===//
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
#if !defined(KBVM_INCLUDE_GUARD_KBIRRunner_HPP)
#define KBVM_INCLUDE_GUARD_KBIRRunner_HPP

#include <string>
#include "KBWorkingMemory.hpp"

XERCES_CPP_NAMESPACE_USE
using namespace llvm;

/** A basic implementation of an NClose interpreter.  It requires a DOM document representing the prologue (See: `kbprologueml.dtd'), and the assembly source file name.  It is called by passing it a working memory implementation against which it runs the session initiated by the prologue file.
    @brief @b Interpreter Sync, command-line based, interactive interpreter
 */
class KBIRRunner{
public:
  /**  Runner Generic Constructors
       @brief Constructor
       @param s The assembly source file name
       @param domDoc The DOM document for the prologue
  */
  KBIRRunner( std::string s, DOMDocument *domDoc );
  KBIRRunner(){};
  /**  Destructor */
  ~KBIRRunner();


  /**  Interpreter's main entry point
       @brief Parses and run assembly source against an XML prologue file
       @param wm The working memory driver
  */
  void runIR( WorkingMemory *wm );

private:
  std::string fSourceFile;
  DOMDocument *fDom;
};

#endif
