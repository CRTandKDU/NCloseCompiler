//===--- nkb-knowcess.cpp - The NClose IR runner --- ----------------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// The NClose interpreter
//
//===----------------------------------------------------------------------===//
// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/parsers/AbstractDOMParser.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMImplementationRegistry.hpp>
#include <xercesc/dom/DOMLSParser.hpp>
#include <xercesc/dom/DOMException.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMError.hpp>
#include <xercesc/dom/DOMLocator.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMAttr.hpp>



#include "llvm/Support/CommandLine.h"

#include <string.h>
#include <stdlib.h>
#if defined(XERCES_NEW_IOSTREAMS)
#include <fstream>
#else
#include <fstream.h>
#endif

#include "../sample/nkb-compile.hpp"
#include "KBWorkingMemory.hpp"
#include "KBIRRunner.hpp"

using namespace std;
using namespace llvm;

static cl::opt<string> S_AssemblyFilename("s", cl::desc("Specify knowledge bases assembly filename (defaults to `out.s')."), cl::init("out.s"), cl::value_desc("filename"));
static cl::opt<string> S_PrologueXMLFilename(cl::Positional, cl::desc("<XML prologue file>"), cl::Required);


// ---------------------------------------------------------------------------
//  DOMCountHandlers: Overrides of the DOM ErrorHandler interface
// ---------------------------------------------------------------------------
DOMCountErrorHandler::DOMCountErrorHandler() : fSawErrors(false){}

DOMCountErrorHandler::~DOMCountErrorHandler(){}

bool DOMCountErrorHandler::handleError(const DOMError& domError){
  fSawErrors = true;
  if (domError.getSeverity() == DOMError::DOM_SEVERITY_WARNING)
    XERCES_STD_QUALIFIER cerr << "\nWarning at file ";
  else if (domError.getSeverity() == DOMError::DOM_SEVERITY_ERROR)
    XERCES_STD_QUALIFIER cerr << "\nError at file ";
  else
    XERCES_STD_QUALIFIER cerr << "\nFatal Error at file ";
  
  XERCES_STD_QUALIFIER cerr << StrX(domError.getLocation()->getURI())
			    << ", line " << domError.getLocation()->getLineNumber()
			    << ", char " << domError.getLocation()->getColumnNumber()
			    << "\n  Message: " << StrX(domError.getMessage()) << XERCES_STD_QUALIFIER endl;
  
  return true;
}

void DOMCountErrorHandler::resetErrors(){ fSawErrors = false; }

// ---------------------------------------------------------------------------
//  Interpreter
// ---------------------------------------------------------------------------
int main( int argc, char* argv[] ){
  cl::ParseCommandLineOptions(argc, argv);

  bool recognizeNEL = false;
  char localeStr[64];
  memset(localeStr, 0, sizeof localeStr);

  // Initialize the XML4C system
  try {
    if (strlen(localeStr)) {
	  XMLPlatformUtils::Initialize(localeStr);
    }
    else{
	  XMLPlatformUtils::Initialize();
    }
    
    if (recognizeNEL){
      XMLPlatformUtils::recognizeNEL(recognizeNEL);
    }
  }
  catch (const XMLException& toCatch){
    XERCES_STD_QUALIFIER cerr << "Error during initialization! :\n"
			      << StrX(toCatch.getMessage()) << XERCES_STD_QUALIFIER endl;
    return 1;
  }

  // Instantiate the DOM parser.
  static const XMLCh gLS[] = { chLatin_L, chLatin_S, chNull };
  DOMImplementation *impl = 
    DOMImplementationRegistry::getDOMImplementation(gLS);
  DOMLSParser *parser = 
    ((DOMImplementationLS*)impl)->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
  DOMConfiguration  *config = parser->getDomConfig();
  // enable datatype normalization - default is off
  config->setParameter(XMLUni::fgDOMDatatypeNormalization, true);

  // And create our error handler and install it
  DOMCountErrorHandler errorHandler;
  config->setParameter(XMLUni::fgDOMErrorHandler, &errorHandler);

  //
  //  Get the starting time and kick off the parse of the indicated
  //  file. Catch any exceptions that might propogate out of it.
  //
  unsigned long duration;
  const char *xmlFile = 0;
  bool more = true;
  XERCES_STD_QUALIFIER ifstream fin;

  // Later possibly a list of prologue files rather than a single one
  while (more){
    xmlFile = S_PrologueXMLFilename.c_str();
    more = false;
  }
      
  // Parse XML prologue file
  // Reset error count first
  errorHandler.resetErrors();

  XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *doc = 0;
  bool errorOccurred = false;

  try{
    // reset document pool
    parser->resetDocumentPool();
      
    const unsigned long startMillis = XMLPlatformUtils::getCurrentMillis();
    doc = parser->parseURI(xmlFile);
    const unsigned long endMillis = XMLPlatformUtils::getCurrentMillis();
    duration = endMillis - startMillis;
  }
  catch (const XMLException& toCatch){
    XERCES_STD_QUALIFIER cerr << "\nError during parsing: '" << xmlFile << "'\n"
			      << "Exception message is:  \n"
			      << StrX(toCatch.getMessage()) << "\n" 
                              << XERCES_STD_QUALIFIER endl;
    errorOccurred = true;
    //    continue;
  }
  catch (const DOMException& toCatch){
    const unsigned int maxChars = 2047;
    XMLCh errText[maxChars + 1];
      
    XERCES_STD_QUALIFIER cerr << "\nDOM Error during parsing: '" 
			      << xmlFile << "'\n"
			      << "DOMException code is:  " 
			      << toCatch.code 
			      << XERCES_STD_QUALIFIER endl;
      
    if (DOMImplementation::loadDOMExceptionMsg(toCatch.code, errText, maxChars))
      XERCES_STD_QUALIFIER cerr << "Message is: " 
				<< StrX(errText) 
				<< XERCES_STD_QUALIFIER endl;
      
    errorOccurred = true;
    //    continue;
  }
  catch (...){
    XERCES_STD_QUALIFIER cerr << "\nUnexpected exception during parsing: '" << xmlFile << "'\n";
    errorOccurred = true;
    //    continue;
  }

  // Print out the stats that we collected and time taken.
  XERCES_STD_QUALIFIER cout << "Parsed " 
			    << xmlFile << ": " << duration << " ms."
			    << XERCES_STD_QUALIFIER endl;

  
  if( !errorOccurred ){
    KBIRRunner *interpreter = 
      new KBIRRunner( S_AssemblyFilename, doc );
    WorkingMemory *wm = new WorkingMemory();
    interpreter->runIR( wm );
    delete interpreter;
    delete wm;
  }
  
  parser->release();

  // And call the termination method
  XMLPlatformUtils::Terminate();

  return 0;
}

