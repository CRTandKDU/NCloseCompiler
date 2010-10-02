//===--- CouchHttpServer.cpp - The NClose Services ------------------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// This class implements an NClose interpreter as a service for HTTP
//
//===----------------------------------------------------------------------===//
#if !defined(KBVM_INCLUDE_GUARD_CouchRunHttpServer_HPP)
#define KBVM_INCLUDE_GUARD_CouchRunHttpServerr_HPP

#include <string>
#include <xercesc/dom/DOMDocument.hpp>
#include "CouchAgenda.hpp"

XERCES_CPP_NAMESPACE_USE
using namespace llvm;
/** @brief @b Interpreter Async, HTTP interactive interpreter.  (Tested with lighttpd.) */
class CouchRunHttpServer{
public:
  /** Constructors */
  CouchRunHttpServer( std::string URL, std::string DB, std::string DOC );
  /** Destructors */
  ~CouchRunHttpServer();

  /** @brief Reads in the prologue XML file
      @param S_PrologueXMLFilename The XML file for prologue
      @return A Xerces DOM document
  */
  DOMDocument *parsePrologue( std::string S_PrologueXMLFilename );

  /** @brief Current status of the inference process
      @return Either STOPPED, INITED, or RUNNING according to Agenda
  */
  const int status();
  /** @brief Initialises service by precompiling the prologue */
  void init();
  /** @brief Stops session, empties working memory, reload agenda and working memory from prologue file */
  void restart();
  /** @brief Resumes inference, returning either on interactive question or on session's end */
  void next();
  
  /** @brief Returns pending question (if any) */
  const std::string getPending();

  /** @brief Post goal to the agenda 
      @param hypo The hypothesis to post
  */
  void suggest( std::string hypo );
  /** @brief Various forms of volunteering typed data */
  void volunteer( std::string sign, int val_bool );
  void volunteer( std::string sign, double val_num );
  void volunteer( std::string sign, std::string val_str );

  /** @brief Get vocabularies of hypotheses and signs (with values) */
  void getEncyclopedia( std::map<std::string, std::string> *map_h, 
			std::map<std::string, std::string> *map_s );

  static const int SERVER_STOPPED = 0;
  static const int SERVER_INITED = 1;
  static const int SERVER_RUNNING = 2;

  static CouchHttpAgenda *S_Agenda;

private:
  int current_state;
  std::string couchURL;
  std::string couchDB;
  std::string couchDoc;
  DOMDocument *fDom;
};

#endif
