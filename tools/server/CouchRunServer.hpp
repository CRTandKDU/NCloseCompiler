//===--- KBIRRunServer.cpp - The NClose Services --------------------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// This class implements an NClose interpreter as a service
//
//===----------------------------------------------------------------------===//
#if !defined(KBVM_INCLUDE_GUARD_CouchRunService_HPP)
#define KBVM_INCLUDE_GUARD_CouchRunServicer_HPP

#include <string>
#include <xercesc/dom/DOMDocument.hpp>
#include "CouchAgenda.hpp"

XERCES_CPP_NAMESPACE_USE
using namespace llvm;
/** @brief @b Interpreter Async interactive interpreter based on CouchDB datastore */
class CouchRunService{
public:
  /** Constructors */
  CouchRunService( std::string URL, std::string DB, std::string DOC );
  /** Destructors */
  ~CouchRunService();

  DOMDocument *parsePrologue( std::string S_PrologueXMLFilename );

  const int status();
  void restart();
  void next();
  
  void suggest( std::string hypo );
  void volunteer( std::string sign, int val_bool );
  void volunteer( std::string sign, double val_num );
  void volunteer( std::string sign, std::string val_str );

  static const int SERVER_STOPPED = 0;
  static const int SERVER_INITED = 1;
  static const int SERVER_RUNNING = 2;

  static CouchServerAgenda *S_Agenda;

private:
  int current_state;
  std::string couchURL;
  std::string couchDB;
  std::string couchDoc;
  
};

#endif
