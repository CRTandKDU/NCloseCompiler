//===--- nkb-couchgtk.cpp - The NClose IR runner --------------------------===//
//
//                     The KBVM Compiler Infrastructure
//
//
//===----------------------------------------------------------------------===//
//
// The NClose interpreter with a GTK+-based GUI
//
//===----------------------------------------------------------------------===//
// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <map>

#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/CommandLine.h"

#include <gtk/gtk.h>

#include "CouchRunServer.hpp"
#include "nkb-couchgtk.hpp"

using namespace std;

#define BUFLENGTH 128

enum nkb_EventType { NKB_IDLE, 
		     NKB_SIGNVOLUNTEERED, 
		     NKB_INFERENCESTEPPED };

static cl::opt<string> S_AssemblyFilename("nkbAssembly", cl::desc("Specify knowledge bases assembly filename (defaults to `assembly.s')."), cl::init("assembly.s"), cl::value_desc("filename"));
static cl::opt<string> S_PrologueXMLFilename("nkbPrologue", cl::desc("<XML prologue file>"), cl::init("prologue.xml"), cl::value_desc("filename"));
static cl::opt<string> S_CouchURL("nkbCouchURL", cl::desc("Couch full URL"), cl::init("http://127.0.0.1:5984"), cl::value_desc("URL"));
static cl::opt<string> S_CouchDB("nkbCouchDB", cl::desc("Couch database"), cl::init("knowcess"), cl::value_desc("database name"));
static cl::opt<string> S_CouchDoc("nkbCouchDoc", cl::desc("Couch document"), cl::init("cc619f59b60377032c97271afa44d566"), cl::value_desc("UUID"));

static CouchRunServer *S_server;
static GtkWidget *S_butRun;
static GtkWindow *S_Encyclopedia;
static GtkCList *S_hypos, *S_signs;
static int S_idle = NKB_IDLE;

/*------------------------------------------------------------------------------
 * Encyclopedia window
 *------------------------------------------------------------------------------
 */
gboolean sig_winEncDelete( GtkWidget *widget, GdkEvent  *event, 
			   gpointer data )
{ return TRUE; }

void sig_HypoSelected( GtkWidget      *clist,
		       gint            row,
		       gint            column,
		       GdkEventButton *event,
		       gpointer        data ){
  gchar *text;

  /* Get the text that is stored in the selected row and column
   * which was clicked in. We will receive it as a pointer in the
   * argument text.
   */
  gtk_clist_get_text(GTK_CLIST(clist), row, column, &text);

  /* Just prints some information about the selected row */
  g_print("You selected hypotheses row %d. More specifically you clicked in "
	  "column %d, and the text in this cell is %s\n\n",
	  row, column, text);

  return;
}

void sig_SignSelected( GtkWidget      *clist,
		       gint            row,
		       gint            column,
		       GdkEventButton *event,
		       gpointer        data ){
  gchar *text;

  /* Get the text that is stored in the selected row and column
   * which was clicked in. We will receive it as a pointer in the
   * argument text.
   */
  gtk_clist_get_text(GTK_CLIST(clist), row, column, &text);

  /* Just prints some information about the selected row */
  g_print("You selected sign row %d. More specifically you clicked in "
	  "column %d, and the text in this cell is %s\n\n",
	  row, column, text);

  return;
}

void updateEncyclopedia(){
  gchar *txt[2];
  map<std::string,std::string>::iterator it;
  std::map<std::string, std::string> map_h; 
  std::map<std::string, std::string> map_s;
  S_server->getEncyclopedia( &map_h, &map_s );
  gtk_clist_clear( S_hypos );   gtk_clist_clear( S_signs ); 
  txt[0] = (gchar *)malloc( (size_t)255 );
  txt[1] = (gchar *)malloc( (size_t)255 );
  for( it = map_h.begin(); it != map_h.end(); it++ ){
    strncpy( txt[0], (*it).first.c_str(), (size_t)255 );
    strncpy( txt[1], (*it).second.c_str(), (size_t)255 );
    gtk_clist_append( S_hypos, txt );
  }
  for( it = map_s.begin(); it != map_s.end(); it++ ){
    strncpy( txt[0], (*it).first.c_str(), (size_t)255 );
    strncpy( txt[1], (*it).second.c_str(), (size_t)255 );
    gtk_clist_append( S_signs, txt );
  }
  free( txt[0] ); free( txt[1] );
}

void winEncyclopedia(){
  GtkWidget *win;
  GtkWidget *vbox;
  GtkWidget *scrwinH, *scrwinS, *clistH, *clistS;
  gchar *titlesH[ 2 ] = { "Hypothesis", "Value" };
  gchar *titlesS[ 2 ] = { "Signs", "Value" };

  S_Encyclopedia = (GtkWindow *)(win = gtk_window_new( GTK_WINDOW_TOPLEVEL ));
  gtk_window_set_default_size( GTK_WINDOW(win), 300, 600 );
  gtk_window_set_title( GTK_WINDOW(win), "Encyclopedia" );
  g_signal_connect ( GTK_OBJECT(win), "delete-event",
		     G_CALLBACK (sig_winEncDelete), NULL);

  vbox = gtk_vbox_new( FALSE, 5 );
  gtk_container_set_border_width( GTK_CONTAINER(vbox), 5 );
  gtk_container_add( GTK_CONTAINER(win), vbox );
  gtk_widget_show( vbox );
  
  scrwinH = gtk_scrolled_window_new( NULL, NULL );
  gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(scrwinH),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_box_pack_start( GTK_BOX(vbox), scrwinH, TRUE, TRUE, 0 );
  gtk_widget_show( scrwinH );

  scrwinS = gtk_scrolled_window_new( NULL, NULL );
  gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(scrwinS),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_box_pack_start( GTK_BOX(vbox), scrwinS, TRUE, TRUE, 0 );
  gtk_widget_show( scrwinS );

  S_hypos = (GtkCList *)(clistH = gtk_clist_new_with_titles( 2, titlesH )) ;
  gtk_clist_set_shadow_type( GTK_CLIST(clistH), GTK_SHADOW_OUT );
  gtk_clist_set_column_width( GTK_CLIST(clistH), 0, 150);
  gtk_signal_connect(GTK_OBJECT(clistH), "select_row",
		     GTK_SIGNAL_FUNC(sig_HypoSelected), NULL);
  gtk_container_add( GTK_CONTAINER(scrwinH), clistH );
  gtk_widget_show( clistH );

  S_signs = (GtkCList *)(clistS = gtk_clist_new_with_titles( 2, titlesS )) ;
  gtk_clist_set_shadow_type( GTK_CLIST(clistS), GTK_SHADOW_OUT );
  gtk_clist_set_column_width( GTK_CLIST(clistS), 0, 150);
  gtk_signal_connect(GTK_OBJECT(clistS), "select_row",
		     GTK_SIGNAL_FUNC(sig_SignSelected), NULL);
  gtk_container_add( GTK_CONTAINER(scrwinS), clistS );
  gtk_widget_show( clistS );

  gtk_widget_show( win );
}

/*------------------------------------------------------------------------------
 * Question window
 *------------------------------------------------------------------------------
 */
gboolean sig_winQuestionDelete( GtkWidget *widget, 
				GdkEvent  *event, 
				gpointer data ){
  /* If you return FALSE in the "delete-event" signal handler,
   * GTK will emit the "destroy" signal. Returning TRUE means
   * you don't want the window to be destroyed.
   * This is useful for popping up 'are you sure you want to quit?'
   * type dialogs. */
  g_print ("delete event occurred\n");
  /* Change TRUE to FALSE and the main window will be destroyed with
   * a "delete-event". */
  return TRUE;
}

gboolean sig_winQuestionDestroy( GtkWidget *widget, GdkEvent  *event, gpointer data ){
  /* If you return FALSE in the "delete-event" signal handler,
   * GTK will emit the "destroy" signal. Returning TRUE means
   * you don't want the window to be destroyed.
   * This is useful for popping up 'are you sure you want to quit?'
   * type dialogs. */
  g_print ("destroy event occurred\n");
  /* Change TRUE to FALSE and the main window will be destroyed with
   * a "delete-event". */
  return FALSE;
}

void sig_butContinue( GtkWidget *widget,  gpointer data ){
  GtkWidget *entry = (GtkWidget *)data;
  printf( gtk_entry_get_text (GTK_ENTRY (entry) ) );
  S_server->volunteer( S_server->getPending(), 
		       std::string(gtk_entry_get_text (GTK_ENTRY (entry) )) );
  S_idle = NKB_SIGNVOLUNTEERED;
  /*
  S_server->next();

  updateEncyclopedia();

  if( CouchRunServer::SERVER_STOPPED == S_server->status() )
    gtk_widget_set_sensitive( GTK_WIDGET(S_butRun), TRUE );
  */
}

void winQuestion( std::string question ){
  GtkWidget *window;
  GtkWidget *butContinue;
  GtkWidget *label;
  GtkWidget *entry;


  char buf[ BUFLENGTH ];
  snprintf( buf, BUFLENGTH, "What is the value of %s?", question.c_str() );

  /* create a new top level window */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Question");
  gtk_window_set_default_size( GTK_WINDOW(window), 300, 100 );
  gtk_window_set_position( GTK_WINDOW(window), GTK_WIN_POS_CENTER );

  g_signal_connect (window, "delete-event",
		    G_CALLBACK (sig_winQuestionDelete), NULL);
    
  g_signal_connect (window, "destroy",
		    G_CALLBACK (sig_winQuestionDestroy), NULL);
    
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    
  butContinue = gtk_button_new_with_label ("OK");
  label = gtk_label_new( buf );
  entry = gtk_entry_new();

  g_signal_connect (entry, "activate",
                    G_CALLBACK(sig_butContinue), entry);
  g_signal_connect (butContinue, "clicked",
		    G_CALLBACK (sig_butContinue), entry);
  g_signal_connect_swapped (butContinue, "clicked",
			    G_CALLBACK (gtk_widget_destroy), window);

    
  /* This packs the button into the window (a gtk container). */
  GtkWidget *vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start( GTK_BOX(vbox), label, FALSE, FALSE, 0 );
  gtk_box_pack_start( GTK_BOX(vbox), entry, FALSE, FALSE, 0 );
  gtk_box_pack_start( GTK_BOX(vbox), butContinue, FALSE, FALSE, 0 );
  gtk_container_add (GTK_CONTAINER (window), vbox);

  gtk_widget_show_all (window);  

  gtk_widget_grab_focus( entry );

}

/** @brief @b Interpreter GTK+ ancillary struct for GtkCList iteration */
typedef struct{
  char title[255];
  GtkWidget *wgt;
} util_queryTitleTy;

void gfunc_titlep( gpointer data, gpointer user_data ){
  printf( "EnumTitle: %s\n", gtk_window_get_title( GTK_WINDOW(data) ) );
  if( NULL != data &&
      NULL != gtk_window_get_title( GTK_WINDOW(data) ) &&
      0 == strcmp( gtk_window_get_title( GTK_WINDOW(data) ), 
		   ((util_queryTitleTy *) user_data)->title ) ){
    ((util_queryTitleTy *) user_data)->wgt = GTK_WIDGET(data);
  }
}

void util_closeQuestion(){
  util_queryTitleTy query;
  strncpy( query.title, "Question", 255 );
  query.wgt = NULL;
  g_list_foreach( gtk_window_list_toplevels(),
		  (GFunc)gfunc_titlep, (gpointer) &query );
  if( NULL != query.wgt ) gtk_widget_destroy( query.wgt );
}

/*------------------------------------------------------------------------------
 * Command window
 *------------------------------------------------------------------------------
 */

/* Another callback */
void destroy( GtkWidget *widget, gpointer data ){
  gtk_main_quit ();
}

/* This is a callback function. The data arguments are ignored
 * in this example. More on callbacks below. */
void sig_butRestart( GtkWidget *widget, gpointer data ){
  util_closeQuestion();
  S_server->restart();
  S_idle = NKB_INFERENCESTEPPED;
}

void sig_butNext( GtkWidget *widget, gpointer data ){
  gtk_widget_set_sensitive( GTK_WIDGET(widget), FALSE );
  S_server->next();
}

void sig_butClose( GtkWidget *widget, gpointer data ){
  printf( "Closing..." );
}


gboolean evt_Idle( gpointer data ){
  int curState;
  switch( S_idle ){
  case NKB_SIGNVOLUNTEERED:
    S_server->next();
    S_idle = NKB_INFERENCESTEPPED;
    break;

  case NKB_INFERENCESTEPPED:
    updateEncyclopedia();
    curState = S_server->status();
    if( CouchRunServer::SERVER_STOPPED == curState || 
	CouchRunServer::SERVER_INITED == curState  )
      gtk_widget_set_sensitive( GTK_WIDGET(S_butRun), TRUE );
    S_idle = NKB_IDLE;
    break;

  default:
    break;
  }

  return TRUE;
}

int main( int argc, char *argv[] ){
  cl::ParseCommandLineOptions(argc, argv);
  // We need exception handling in LLVM compiled code.
  // Pending question returns control to interpreter via throwing exceptions or setjmp/longjmp
  llvm::JITExceptionHandling = true;
  
  GtkWidget *window;
  GtkWidget *butRestart, *butNext, *butClose;
    
  /* This is called in all GTK applications. Arguments are parsed
   * from the command line and are returned to the application. */
  gtk_init (&argc, &argv);

  S_server =  new CouchRunServer( S_CouchURL, S_CouchDB, S_CouchDoc );
  S_server->init();
    
  /* create a new window */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "KBVM Experimental v1.0");
  gtk_window_set_default_size( GTK_WINDOW(window), 300, 200 );
    
  /* When the window is given the "delete-event" signal (this is given
   * by the window manager, usually by the "close" option, or on the
   * titlebar), we ask it to call the delete_event () function
   * as defined above. The data passed to the callback
   * function is NULL and is ignored in the callback function. */
  g_signal_connect (window, "delete-event",
		    G_CALLBACK (sig_winQuestionDelete), NULL);
    
  /* Here we connect the "destroy" event to a signal handler.  
   * This event occurs when we call gtk_widget_destroy() on the window,
   * or if we return FALSE in the "delete-event" callback. */
  g_signal_connect (window, "destroy",
		    G_CALLBACK (destroy), NULL);
    
  /* Sets the border width of the window. */
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    
  /* Creates a new button with the label "Hello World". */
  butRestart = gtk_button_new_with_label ("Restart");
  S_butRun = butNext = gtk_button_new_with_label ("Run...");
  butClose = gtk_button_new_with_label ("Close");
  //  label = gtk_label_new( "What is the value of?" );
  //  entry = gtk_entry_new();

  g_signal_connect (butRestart, "clicked", G_CALLBACK (sig_butRestart), NULL);
  g_signal_connect (butNext,    "clicked", G_CALLBACK (sig_butNext),    NULL);
  g_signal_connect (butClose,   "clicked", G_CALLBACK (sig_butClose),   NULL);
    
  /* This will cause the window to be destroyed by calling
   * gtk_widget_destroy(window) when "clicked".  Again, the destroy
   * signal could come from here, or the window manager. */
  g_signal_connect_swapped (butClose, "clicked",
			    G_CALLBACK (gtk_widget_destroy), window);    

  GtkWidget *vbox = gtk_vbox_new (FALSE, 2);
  //  gtk_box_pack_start( GTK_BOX(vbox), label, FALSE, FALSE, 0 );
  //  gtk_box_pack_start( GTK_BOX(vbox), entry, FALSE, FALSE, 0 );
  gtk_box_pack_start( GTK_BOX(vbox), butRestart, FALSE, FALSE, 0 );
  gtk_box_pack_start( GTK_BOX(vbox), butNext, FALSE, FALSE, 0 );
  gtk_box_pack_start( GTK_BOX(vbox), butClose, FALSE, FALSE, 0 );
  gtk_container_add (GTK_CONTAINER (window), vbox);
    
  /* The final step is to display this newly created widget. */
  //gtk_widget_show (button);
    
  /* and the window */
  gtk_widget_show_all (window);
  winEncyclopedia();

  (void) g_idle_add( evt_Idle, NULL );
    
  /* All GTK applications must have a gtk_main(). Control ends here
   * and waits for an event to occur (like a key press or
   * mouse event). */
  gtk_main ();
  
  if( S_server ) delete S_server;
  return 0;
}

