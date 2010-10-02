//===--- CouchRunner.cpp - The NClose interpreter -------------------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// This class implements a simple LLVM-based interpreter for IR generated
// by the nkb-compile tool (or by the `KBIRGenerator' class).
//
//===----------------------------------------------------------------------===//
#if !defined(KBVM_INCLUDE_GUARD_CouchRunner_HPP)
#define KBVM_INCLUDE_GUARD_CouchRunner_HPP

#include <string>

#include "../runner/KBIRRunner.hpp"
#include "CouchWorkingMemory.hpp"
#include "CouchAgenda.hpp"

XERCES_CPP_NAMESPACE_USE
using namespace llvm;

/** Another basic implementation of an interpreter based on a CouchDB working memeory.  It requires a DOM document representing the prologue (See: `kbprologueml.dtd'), and the assembly source file name.  It is called by passing it the working memory implementation against which it runs the session initiated by the prologue file.
    @brief @b Interpreter Sync, command-line, interactive interpreter with CouchDB datastore
 */
class CouchRunner{
public:
  /**  Runner Generic Constructors
       @brief Uses LLVM JIT to run an NClose KBs assembly source 
       @param s The assembly source file name
       @param domDoc The DOM document for the prologue
  */
  CouchRunner( std::string s, DOMDocument *domDoc );
  /** @name Destructor */
  ~CouchRunner();


  /**  Interpreter's main entry point
       @brief Parses and run assembly source against an XML prologue file
       @param wm The working memory driver for this run
  */
  void runIR( CouchWorkingMemory *wm );

  static CouchAgenda *S_Agenda;

private:
  std::string fSourceFile;
  DOMDocument *fDom;
};

#endif
