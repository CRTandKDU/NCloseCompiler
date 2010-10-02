/**
   @mainpage The KBVM Compiler Infrastructure
   @author jmc@neurondata.org
   @section IntroRef Introduction
   The KBVM projects explore the use of the LLVM Compiler infrastructure (See: http://www.llvm.org) for NClose knowledge base compilers, interpreters and virtual machines.  Warning: This is experimental code for exploratory investigations.

   NClose is an original rule-based system and implementation algorithm, developped by the founders of Neuron Data during extended research visits at the Robotics Institute, Carnegie-Mellon University, in 1982 and 1983.  The popular and succesful commercial expert system shell, Nexpert and later Nexpert Object, stemmed out of that elder NClose generation.

   The LLVM Project is a collection of modular and reusable compiler and toolchain technologies.  In the KBVM architecture, LLVM Core, one of the primary LLVM subproject is used to provide a virtual machine infrastructure for the execution of NClose knowledge bases ("knowcessing").  The LLVM Core libraries provide a modern source- and target-independent optimizer, along with code generation support for many popular CPUs (as well as some less common ones!) These libraries are built around a well specified code representation known as the LLVM intermediate representation ("LLVM IR"). The LLVM Core libraries are well documented, and it is particularly easy to invent your own language (or port an existing compiler) to use LLVM as a optimizer and code generator.

   In most of the documentation, we use the terms `sign' and 'variable', `goal' and `hypothesis', interchangeably in the context of NClose knowledge bases.  This experimental work emphasizes a @em functional view of rules and production systems, a departure from the later implementations that lead to the Nexpert series of commercial products.
   
   @section CompileRef nkb-compile: Knowledge Base Compilers
   In this project, NClose knowledge bases are considered as source code to be compiled to a compact format later to be run by an interpreter or natively executed.  This compact representation, the compilation and the interpretation/execution all rely on the LLVM infrastructure and tools.

   The nkb-compile tools takes one or several XML knowledge bases (See: kbml.dtd, for the definition of knowledge base XML schemas) and emits an assembly source file in the LLVM Intermediate Representation language.  This assembly can be inspected as it is human-readable and compataible with the LLVM tools. (In particular with the opt tool to run optimization passes.)

   This assembly compiled code follows a radically simple architecture:

  - Each goal in the ruleset translates to one function, which sequentially calls other functions representing the rules leading to said goal;

  - Each rule itself translates to one function that examines the LHS, test it, and, if matched, executes the RHS, and then returns the boolean result of the LHS evaluation.

  - A LHS is an and-ed sequence of tests.  Tests are defined by an operator, which also determines the type of the variables in its arguments, and a series of arguments.  In the current basic implementation there are three types of tests: boolean, numerical, and string.  (Note: this should be extensible with a form of plugin mechanism for user-defined types and tests.)

  - The goal-collecting procedure, used for forward chaining, is implemented by a single "posting" function.  It passes back these forwarded goals to the agenda process in the KBVM through an externally linked function in the bitcode.

  - Interactions with the working memory (is a sign known, what is the value of a sign, and assign a value to a sign) are also implemented as externally linked functions in the bitcode.

  The execuction of the NClose process is left to the KBVM engine in the interpreter (See: next section) which runs the compiled functions in proper sequence alternating backward and forward chainings.

Having the agenda externally linked allows experimental variations in the implementation of the main agenda process.  Similarly, having the access to the working memory mediated by a driver, outside of the assembly code,  allows several datastore models to be plugged into a knowledge base system.  For instance, the nkb-knowcess interpreter uses an in-memory standard vector structure to implement the working memory.  On the other hand, the nkb-couch interpreter and derivatives use the NoSQL database CouchDB as a persistent datastore.  Other data repositories are possible and linked to the KVBM via a simplified API.

The canonical invocation of the compiler tool is:

<tt>Debug/bin/nkb-compile <knowledge-base-file.xml></tt>

which, by default, produces a compiled `out.s' assembly file in LLVM IR format.

Several knowledge bases can be compiled and linked with the following invocation:

<tt>Debug/bin/nkb-compile -l <file-listing-XML-knowledge-base-files></tt>

where the list of knowledge bases to compile and link is in an external file, one base per line.

   @section KnowcessRef nkb-knowcess: Basic Knowledge Base Interpreter and VM
   The nkb-knowcess tool is a basic command line interpreter which executes a compiled NClose knowledge base.  It also takes as an input a prologue that sets up the initial agenda and working memory.

   Typical of the NClose design, the evaluation of goals, or hypotheses, is driven by a stack-like agenda.  Hypotheses are popped out of the agenda and evaluated in backward-chaining mode, i.e. from goals to subgoals recursively down to, eventually, signs.  During this process, alternate goals may be collected for later evaluation.  These "postponed" goals are thus posted to the tail of the agenda for further evaluation.  The overall evaluation process iterates until it empties the agenda, at which time it returns the final state of the working memory.  The KBVM is the engine that supports this NClose evaluation process for running knowledge bases.

   The planned KBVM implementation projects support the construction of various NClose interpreters by providing the basic required infrastructure: JIT verification and compilation of knowledge base bitcode, working memory high-level driver implementation, and agenda implementation.  

The nkb-knowcess interpreter uses this infrastructure to implement a simple command line interpreter which reads (i) an XML prologue file and (ii) the knowledge base bitcode file; and run it against a default working memory implementation (in-memory).  Mostly to be considered as a quick and easy tool to check and debug knowledge bases, it is also a convincing backbone for a richer family of interpreters.

The canonical invocation of this basic intepreter is:

<tt>Debug/bin/nkb-knowcess <prologue-file.xml></tt>

which, by default, loads the compiled `out.s' knowledge base(s), starts the prologue file setting up the initial agenda and working memory state, then runs the interactive process of evaluation -- sometimes popping up interactive questions to the user on the command line.  At the end of the evaluation, the final state of the working memory is simply printed out on screen.  Note that this interpreter implements "synchronous" interactive evaluation as the NClose process blocks on each question, waiting for user's answer.

@section MoreRef Further Interactive Interpreters

See also: @ref couch

See also: @ref server

See also: @ref gtk

See also: @ref httpserver

*/

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

#include "nkb-compile.hpp"

#include <string.h>
#include <stdlib.h>

#if defined(XERCES_NEW_IOSTREAMS)
#include <fstream>
#else
#include <fstream.h>
#endif

#include "KBIRGenerator.hpp"


static std::string S_SourceFile("out.s");

// ---------------------------------------------------------------------------
//  This is a simple program which invokes the DOMParser to build a DOM
//  tree for the specified input file. It then walks the tree and counts
//  the number of elements. The element count is then printed.
// ---------------------------------------------------------------------------
static void usage()
{
    XERCES_STD_QUALIFIER cout << "\nUsage:\n"
            "    DOMCount [options] <XML file | List file>\n\n"
            "This program invokes the DOMLSParser, builds the DOM tree,\n"
            "and then prints the number of elements found in each XML file.\n\n"
            "Options:\n"
            "    -l          Indicate the input file is a List File that has a list of xml files.\n"
            "                Default to off (Input file is an XML file).\n"
            "    -o=<file>   Output file for generated assembly code.\n"
            "    -v=xxx      Validation scheme [always | never | auto*].\n"
            "    -n          Enable namespace processing. Defaults to off.\n"
            "    -s          Enable schema processing. Defaults to off.\n"
            "    -f          Enable full schema constraint checking. Defaults to off.\n"
            "    -locale=ll_CC specify the locale, default: en_US.\n"
            "    -p          Print out names of elements and attributes encountered.\n"
		    "    -?          Show this help.\n\n"
            "  * = Default if not provided explicitly.\n"
         << XERCES_STD_QUALIFIER endl;
}


// ---------------------------------------------------------------------------
//
//  Recursively Count up the total number of child Elements under the specified Node.
//  Process attributes of the node, if any.
//
// ---------------------------------------------------------------------------
static int countChildElements(DOMNode *n, bool printOutEncounteredEles)
{
    DOMNode *child;
    int count = 0;
    if (n) {
        if (n->getNodeType() == DOMNode::ELEMENT_NODE)
		{
            if(printOutEncounteredEles) {
                char *name = XMLString::transcode(n->getNodeName());
                XERCES_STD_QUALIFIER cout <<"----------------------------------------------------------"<<XERCES_STD_QUALIFIER endl;
                XERCES_STD_QUALIFIER cout <<"Encountered Element : "<< name << XERCES_STD_QUALIFIER endl;

                XMLString::release(&name);

                if(n->hasAttributes()) {
                    // get all the attributes of the node
                    DOMNamedNodeMap *pAttributes = n->getAttributes();
                    const XMLSize_t nSize = pAttributes->getLength();
                    XERCES_STD_QUALIFIER cout <<"\tAttributes" << XERCES_STD_QUALIFIER endl;
                    XERCES_STD_QUALIFIER cout <<"\t----------" << XERCES_STD_QUALIFIER endl;
                    for(XMLSize_t i=0;i<nSize;++i) {
                        DOMAttr *pAttributeNode = (DOMAttr*) pAttributes->item(i);
                        // get attribute name
                        char *name = XMLString::transcode(pAttributeNode->getName());

                        XERCES_STD_QUALIFIER cout << "\t" << name << "=";
                        XMLString::release(&name);

                        // get attribute type
                        name = XMLString::transcode(pAttributeNode->getValue());
                        XERCES_STD_QUALIFIER cout << name << XERCES_STD_QUALIFIER endl;
                        XMLString::release(&name);
                    }
                }
            }
			++count;
		}
        for (child = n->getFirstChild(); child != 0; child=child->getNextSibling())
            count += countChildElements(child, printOutEncounteredEles);
    }
    return count;
}

// ---------------------------------------------------------------------------
//
//   main
//
// ---------------------------------------------------------------------------
int main(int argC, char* argV[])
{
  // Check command line and extract arguments.
  if (argC < 2){
    usage();
    return 1;
  }

  const char*                xmlFile = 0;
  AbstractDOMParser::ValSchemes valScheme = AbstractDOMParser::Val_Auto;
  bool                       doNamespaces       = false;
  bool                       doSchema           = false;
  bool                       schemaFullChecking = false;
  bool                       doList = false;
  bool                       errorOccurred = false;
  bool                       recognizeNEL = false;
  bool                       printOutEncounteredEles = false;
  char                       localeStr[64];
  memset(localeStr, 0, sizeof localeStr);

  int argInd;
  for (argInd = 1; argInd < argC; argInd++){
    // Break out on first parm not starting with a dash
    if (argV[argInd][0] != '-')
      break;

    // Watch for special case help request
    if (!strcmp(argV[argInd], "-?")){
      usage();
      return 2;
    }
    else if (!strncmp(argV[argInd], "-o=", 3)
	     ||  !strncmp(argV[argInd], "-O=", 3)){
      const char* const parm = &argV[argInd][3];
      S_SourceFile = std::string( parm );
    }
    else if (!strncmp(argV[argInd], "-v=", 3)
	     ||  !strncmp(argV[argInd], "-V=", 3)){
      const char* const parm = &argV[argInd][3];
      
      if (!strcmp(parm, "never"))
	valScheme = AbstractDOMParser::Val_Never;
      else if (!strcmp(parm, "auto"))
	valScheme = AbstractDOMParser::Val_Auto;
      else if (!strcmp(parm, "always"))
	valScheme = AbstractDOMParser::Val_Always;
      else{
	XERCES_STD_QUALIFIER cerr << "Unknown -v= value: " << parm << XERCES_STD_QUALIFIER endl;
	return 2;
      }
    }
    else if (!strcmp(argV[argInd], "-n")
	     ||  !strcmp(argV[argInd], "-N")){
	doNamespaces = true;
    }
    else if (!strcmp(argV[argInd], "-s")
	     ||  !strcmp(argV[argInd], "-S")){
      doSchema = true;
    }
    else if (!strcmp(argV[argInd], "-f")
	     ||  !strcmp(argV[argInd], "-F")){
      schemaFullChecking = true;
    }
    else if (!strcmp(argV[argInd], "-l")
	     ||  !strcmp(argV[argInd], "-L")){
      doList = true;
    }
    else if (!strcmp(argV[argInd], "-special:nel")){
      // turning this on will lead to non-standard compliance behaviour
      // it will recognize the unicode character 0x85 as new line character
      // instead of regular character as specified in XML 1.0
      // do not turn this on unless really necessary
      
      recognizeNEL = true;
    }
    else if (!strcmp(argV[argInd], "-p")
	     ||  !strcmp(argV[argInd], "-P")){
      printOutEncounteredEles = true;
    }
    else if (!strncmp(argV[argInd], "-locale=", 8)){
      // Get out the end of line
      strcpy(localeStr, &(argV[argInd][8]));
    }
    else {
      XERCES_STD_QUALIFIER cerr << "Unknown option '" << argV[argInd]
				<< "', ignoring it\n" << XERCES_STD_QUALIFIER endl;
    }
  }

  //
  //  There should be only one and only one parameter left, and that
  //  should be the file name.
  //
  if (argInd != argC - 1){
    usage();
    return 1;
  }

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

  config->setParameter(XMLUni::fgDOMNamespaces, doNamespaces);
  config->setParameter(XMLUni::fgXercesSchema, doSchema);
  config->setParameter(XMLUni::fgXercesHandleMultipleImports, true);
  config->setParameter(XMLUni::fgXercesSchemaFullChecking, schemaFullChecking);

  if (valScheme == AbstractDOMParser::Val_Auto) {
    config->setParameter(XMLUni::fgDOMValidateIfSchema, true);
  }
  else if (valScheme == AbstractDOMParser::Val_Never){
    config->setParameter(XMLUni::fgDOMValidate, false);
  }
  else if (valScheme == AbstractDOMParser::Val_Always){
    config->setParameter(XMLUni::fgDOMValidate, true);
  }

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

  bool more = true;
  XERCES_STD_QUALIFIER ifstream fin;

  // the input is a list file
  if (doList)
    fin.open(argV[argInd]);

  if (fin.fail()) {
    XERCES_STD_QUALIFIER cerr <<"Cannot open the list file: " << argV[argInd] << XERCES_STD_QUALIFIER endl;
    return 2;
  }

  KBIRGenerator *builder = new KBIRGenerator();

  while (more){
    char fURI[1000];
    //initialize the array to zeros
    memset(fURI,0,sizeof(fURI));
    
    if (doList) {
      if (! fin.eof() ) {
	fin.getline (fURI, sizeof(fURI));
	if (!*fURI)
	  continue;
	else {
	  xmlFile = fURI;
	  XERCES_STD_QUALIFIER cerr << "==Parsing== " << xmlFile << XERCES_STD_QUALIFIER endl;
	}
      }
      else
	break;
    }
    else {
      xmlFile = argV[argInd];
      more = false;
    }

    //reset error count first
    errorHandler.resetErrors();

    XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *doc = 0;

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
				<< StrX(toCatch.getMessage()) << "\n" << XERCES_STD_QUALIFIER endl;
      errorOccurred = true;
      continue;
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
      continue;
    }
    catch (...){
      XERCES_STD_QUALIFIER cerr << "\nUnexpected exception during parsing: '" << xmlFile << "'\n";
      errorOccurred = true;
      continue;
    }

    //
    //  Extract the DOM tree, get the list of all the elements and report the
    //  length as the count of elements.
    //
    if (errorHandler.getSawErrors()){
      XERCES_STD_QUALIFIER cout << "\nErrors occurred, no output available\n" << XERCES_STD_QUALIFIER endl;
      errorOccurred = true;
    }
    else{
      
      builder->setDoc( doc );
      builder->generateIR( );

      unsigned int elementCount = 0;
      if (doc) {
	elementCount = countChildElements((DOMNode*)doc->getDocumentElement(), printOutEncounteredEles);
	// test getElementsByTagName and getLength
	XMLCh xa[] = {chAsterisk, chNull};
	if (elementCount != doc->getElementsByTagName(xa)->getLength()) {
	  XERCES_STD_QUALIFIER cout << "\nErrors occurred, element count is wrong\n" << XERCES_STD_QUALIFIER endl;
	  errorOccurred = true;
	}
      }

      // Print out the stats that we collected and time taken.
      XERCES_STD_QUALIFIER cout << xmlFile << ": " << duration << " ms ("
				<< elementCount << " elems)." << XERCES_STD_QUALIFIER endl;
    }
  }

  if( !errorHandler.getSawErrors() ){
    builder->closeIR( S_SourceFile );
    delete builder;
    /*
    KBIRRunner *interpreter = new KBIRRunner( S_SourceFile,
					      (DOMDocument*)NULL );
    WorkingMemory *wm = new WorkingMemory();
    interpreter->runIR( wm );
    delete interpreter;
    delete wm;
    */
  }

  //
  //  Delete the parser itself.  Must be done prior to calling Terminate, below.
  //
  parser->release();

  // And call the termination method
  XMLPlatformUtils::Terminate();

  if (doList)
    fin.close();

 
  if (errorOccurred)
    return 4;
  else
    return 0;
}


DOMCountErrorHandler::DOMCountErrorHandler() :

    fSawErrors(false)
{
}

DOMCountErrorHandler::~DOMCountErrorHandler()
{
}


// ---------------------------------------------------------------------------
//  DOMCountHandlers: Overrides of the DOM ErrorHandler interface
// ---------------------------------------------------------------------------
bool DOMCountErrorHandler::handleError(const DOMError& domError)
{
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

void DOMCountErrorHandler::resetErrors()
{
    fSawErrors = false;
}
