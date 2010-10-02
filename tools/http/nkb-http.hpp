//===--- nkb-http.cpp - The NClose Interpreter as a Service----------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// The NClose interpreter as a, HTTP server
//
//===----------------------------------------------------------------------===//
/**
@page httpserver Web Service
@author jmc@neurondata.org

@section httpintro Introduction
This implementation embeds the NClose interpreter in a HTTP server.  It uses the GNU libmicrohttpd (See: http://www.gnu.org/software/libmicrohttpd/microhttpd.html#Top) to create a NClose Web Service.  The design is loosely based on the CouchDB type of interactions: requests are POST with data carrying commands and data; responses are JSON strings.

With each connection to the server, the service stores away its identification and data in the POST request into a service-specific @c struct which is passed back and forth during processing of the request.

The processing itself consists in passing back the commands to the NClose Interreter as in the various async implementations.  The vocabulary of commands is similar : @c status, @c suggest, @c volunteer, @c restart and @c next (to resume inference).

This processing results in the inference engine ending in different states:

@li Session initialized, inference ready to run, as a result of loading the prologue file;
@li Session running, a question pending on the value of a sign;
@li Session terminated, on an empty agenda;
@li Server failure: for some reason the inference process failed.

These results are passed back in HTTP responses as JSON strings (in CouchDB nispired formats).  These are visible in the @c nkb-http.cpp set of @c S_FmtXXX global pattern constants.

An interesting direction, not explored in this implementation, would be to implement NClose RHS commands that send commands or information back to the service caller.

@section httprunning Running NClose as a Web Service
@subsection run Running the NClose Server
Keeping in mind that the file names for the compiled knowledge base and the prologue XML -- which are stored in the associated CouchDB document -- are relative to the directory where the server is started (and running), the command line invocation is simply:

<tt>Debug/bin/nkb-couchhttp</tt>

The server is simply stopped by typing @c <Return> in the same terminal.

@subsection frontend A Basic Web User Interface
The directory @c www contains a basic example of a browser-based client to the NClose Web Service.  It is built using Ajax as provided by the jQuery Javascript library (See: http://jquery.com/).

The @c index.html file contains a HTML page with four sections displaying goals, signs, agenda and question (with prompt for an answer when pending).  Communication with the NClose Web Service is mediated by jQuery scripts loaded from the @c nkb-tools.js example file.

The scripts call both the CouchDB server -- did you forget to start it?  It's still time to relax: <tt>sudo -u couchdb couchdb -i</tt> -- and the NClose Web service.  Since these calls are possibly on a different domain, even when the port number is different on the same machine does the same-origin policy blocks the Ajax calls, there are two simplistic CGI shell scripts to pass back calls to the CouchDB and the NClose servers respectively: @c couchdb.sh and @c nkbcouchhttp.sh.  These quick-and-dirty artifacts redirect the query string they get passed by the CGI into GET or POST requests to the appropriate server URLs with @c curl.  (Simpler architectures do, of course, exist!)

Making that @c www directory the root document directory for the Web server -- this implementation has been tested and run with lighttpd 1.4.28 (See: http://www.lighttpd.net/), for instance (invocation: <tt>lighttpd -D -f lighttpd.conf</tt>) -- point your browser to @c index.html.  The upper three buttons control the inference engine, the body of the page is displaying the four sections already mentioned.  Clicking "Restart" empties the agenda and loads the prologue file and readies the session to be run.  This is done by clicking the "Next" button.  As the inference progresses questions pop up on the right hand side and, as answers are provided, the agenda, signs and goals are updated.  The "Update" button can be used any time to refresh the information display. 

*/
