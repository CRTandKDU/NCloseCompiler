//===--- nkb-couchgtk.cpp - The GTK NClose Interpreter --------------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// A GTK front-end to the NClose Services
//
//===----------------------------------------------------------------------===//
/** @page gtk GTK+ GUI Interactive Interpreter
    @author jmc@neurondata.org

@section GnomeIntro Introduction
This implementation hides the previous async implementation behind a GTK+ based GUI.  It also shows how to program async calls against a single instance of the NCloser server.

The design is articulated around three windows: a command window with "Close", "Restart" and "Run" buttons, an Encyclopedia window, listing goals and signs, and an interactive dialog window prompting for a value for unknown signs, as required by the inference process.

An async NClose server class (CouchRunServer) is initialized with sensible default values at the main entry point.  In contrast to the NClose Web Server, this instance is unique along the duration of the (possibly multiple) sessions.  Communication with this instance occurs mostly in the GTK callback functions installed for the various button widgets.

The implementation also uses its own @idle event to resume inference, once data has been volunteered in the window question, or to update the Encyclopedia window with the list of goals and signs already explored by the inference engine.  It shows how to intersperse GUI- and NClose-events.

@section GnomeUsage Usage
Like before, CouchDB is the persistent datastore; it is started with <tt>sudo -u couchdb couchdb -i</tt> in a separate terminal, if not run as a service at boot time.

The standalone GUI client application is run with:

<tt>Debug/bin/nkb-couchgtk</tt>

which opens the Encyclopedia and Command windows.

A session is started by clicking "Restart" and then "Run" in the Command window. The (non blocking) question window opens when a piece of data is required from the user.

Note that the GUI lacks so many features that it's almost painful to compare it to the glory of Nexpert.

*/
