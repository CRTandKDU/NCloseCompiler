//===--- nkb-couchserver.cpp - The NClose IR runner --- -------------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// The NClose interpreter as a server
//
//===----------------------------------------------------------------------===//
/** @page server Async Execution
    @author jmc@neurondata.org

@section ServerIntro Introduction
The next step in exploring some variants for NClose interpretation is a first asynchronous interpreter.  In contrast to the previous synchronous interpreters, nkb-knowcess and nkb-couch, an asynchronous interactive interpreter persists its complete state when a question to the user is required.  When the user is ready to provide information or when it becomes available later, the interpretation process resumes from this persisted state until its next interaction (or termination).

This async execution is required, for instance, for GUI-based interpreters where the GUI event loop needs to be intertwined with the flow of inference process events.  It is also required in the Web Service context, where inference is triggerred by HTTP requests, while responses carry back questions to the browser.

As the knowledge base is compiled to function calls, however, the state of the computation which needs to be persisted is quite comprehensive: it should embed the call stack, registers and global variables values, etc.  Rather than handling such a computation state using LLVM libraries -- should this even be possible -- the implementation stores a more abstract state of the inference process. The inference state simply consists of the current state of the agenda coupled with the current state of the working memory.

When inference resumes, the top goal in the agenda is evaluated by calling its associated compiled function.  When a question is prompted, a C @c longjump instruction restores the context of this initial goal-function call and the inference process exits.  (Exceptions have been tried as an alternative to setjmp/longjmp but were found less reliable in the current LLVM version.)  If no question is prompted, the goal-function computes and exits normally, turning to the next goal in the agenda.

@section ServerDesign The nkb-server Interpreter
@subsection ServerAgenda The Agenda
In this implementation the agenda is persisted to the CouchDB datastore, reusing the same document in which the working memory is also stored.  The persistence is articulated around two API calls in the datastore manager:

@li @c getAgenda
@li @c setAgenda

which (hopefully) are self-explanatory.

@subsection ServerWM The Working Memory
    In this implementation, the working memory is stored as an individual CouchDB document in a dedicated database.  As a result of this design choice, an instance of a working memory is identified by three parameters:
@li The URL of the running CouchDB server
@li The database name
@li The document id in the database
In addition, both the assembly file and the prologue file names are stored in the CouchDB document.

The API to the working memory datasore is organized around:
@li @c knownp A boolean query to check whether a sign or hypothesis is already known
@li @c set calls to assign typed values to signs (boolean, numerical or string) and hypotheses (boolean)
@li @c get calls to retrieve typed values for signs and hypotheses
These are usually called from the assignment and question runtime functions in the KBVM.

The API to CouchDB is organized around three calls: @c init, @c clear, and @c update which respectively retrieve the full CouchDB document, store a empty version of the document, and store back a modified document respectively.

Note: internally, nkb-couch uses @c curl (See: http://curl.haxx.se/) and @c libcrul to access CouchDB.  It also uses @c jansson (See: http://www.digip.org/jansson/), a C library to parse and handle JSon data.

@subsection ServerUsage Usage
The use of an async interactive interpreter is different as it accepts commands that update the persistent state in the datastore and then exit.  The interpreter no longer blocks on question: it simply prints it out and exit.  Users are expected to answer using specific new commands to update the persistent working memory.

The main invocation command is:

<tt>Debug/bin/nkb-server <command> [<command-args>]</tt>

where the command is one of the following vocabulary:

@li @c restart Reloads the knowledge base and prologue file, ready to run.
@li @c next Resumes the inference process.  This either prompts the next question or terminates the session.
@li <tt>suggest <hypothesis></tt> Adds a goal to the agenda for later evaluation.
@li <tt>volunteer <sign> <val> </tt> Volunteers a value for sign.  Also used to answer pending questions.
@li @c status Returns the status of the service as an integer.

For instance, an interactive session might look like:

@htmlonly
<style>  
div.padded {  
background: #AAAAAA;  
color: black;
font-family: courier, monospace;  
font-size: 12pt;   
font-style: normal;
padding-top: 10px;  
padding-right: 0px;  
padding-bottom: 0.25in;  
padding-left: 5em;  
}  
</style>  
<div class="padded">
<i>The CouchDB service runs in the background and the path to document has been properly set up to the compiled `satfault' knowledge base and prologue.</i><br>
Debug/bin/nkb-couchserver  restart
<br>
Debug/bin/nkb-couchserver  next
<br>
<i>$ prints out question on pressure_P1</i>
<br>
Debug/bin/nkb-couchserver  volunteer pressure_P1 500
<br>
Debug/bin/nkb-couchserver  volunteer pressure_P2 500
<br>
Debug/bin/nkb-couchserver  next
<br>
<i>$ prints out question on pressure_out_P3</i>
<br>
Debug/bin/nkb-couchserver  volunteer pressure_out_P3 200
<br>
Debug/bin/nkb-couchserver  next
<br>
<i>$ prints out question on pressure_out_P4</i>
<br>
Debug/bin/nkb-couchserver  volunteer pressure_out_P4 200
<br>
Debug/bin/nkb-couchserver  volunteer pressure_P3 500
<br>
Debug/bin/nkb-couchserver  volunteer pressure_P4 500
<br>
Debug/bin/nkb-couchserver  next
<br>
<i>$ Session terminates</i>
</div>
@endhtmlonly

Note that several evidences may be volunteered while a question is pending, or goals suggested for that matter.  These are posted to the agenda for later evaluation and the working memory is appropriately updated.

 */
