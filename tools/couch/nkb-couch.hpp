//===--- nkb-couch.cpp - The NClose IR runner -----------------------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// The NClose interpreter
//
//===----------------------------------------------------------------------===//
/** @page couch Using CouchDB as the working memory datastore
    @author jmc@neurondata.org

    @section intro Introduction
    As a first step in building increasingly functional NClose interpreters, the nkb-couch intepreter uses the NoSQL CouchDB database to store the working memory state at runtime.

Apache CouchDB (See: http://wiki.apache.org/couchdb/) is a scalable, fault-tolerant, and schema-free document-oriented database written in Erlang. It's used in large and small organizations for a variety of applications where a traditional SQL database isn't the best solution for the problem at hand. Among other features, it provides:
    @li A RESTful HTTP/JSON API accessible from many programming libraries and tools.
    @li Futon, a browser based GUI.
    @li Robust, incremental replication with bi-directional conflict detection/resolution, and more.
    @li Incremental Map/Reduce queries written in JavaScript.
    @li Excellent data integrity/reliability utilizing MVCC.
    @li Stores BLOBs (Binary Large Objects) natively.
    @li Easy installation on many platforms.
    @li A strong and active community.
    @li Good documentation in the form of Books, Presentations, Blog Posts, Wikis, and more. 

    This is only but one example of external datastore for an interpreter.

    @section program The nkb-couch interpreter
    @subsection agenda The Agenda
    In this basic interpreter, the agenda is implemented as in the previous itertation, through a @c deque standard C++ library type.
    @subsection wm The Working Memory
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

@section usage Usage
Provided the CouchDB server is running (it is usually invoked with the following command: <tt>sudo -u couchdb couchdb -i</tt>, if not started at boot time as a service.)

As all session information is stored away in the CouchDB document this interpreter is simply launched by:

<tt>Debug/bin/nkb-couch</tt>

which gets its compiled knowledge base assembly file and prologue XML file from the document. (Path to the CouchDB document may be passed as arguments to the command line to change the default.)

nkb-couch is another synchronous intereactive interpreter as questions are asked to the user via the text terminal and the inference process blocks, waiting for users' responses.  At any time though, the current state of the working memory is available from the CouchDB database, for instance through the Futon GUI.

*/
