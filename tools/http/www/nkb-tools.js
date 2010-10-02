// The KBVM Compiler Infrastructure
// A basic javascript front-end to NClose Services
// @author jmc@neurondata.org

// Skip provisioning of the document id for each new identified user/session
var S_UUID = 'cc619f59b60377032c97271afa44d566';

/** @brief Redraws hypotheses and signs as tables, and agenda as list */
redrawEncys = function(){
  // Directly access the CouchDB document  
  $.getJSON(
      'http://127.0.0.1:8000/couchdb.sh?http://127.0.0.1:5984/knowcess/' + S_UUID,
      function( data ){
	  $('#agenda').empty();
	  $('#hypos').empty();
	  $('#signs').empty();
	  $.each( data.agenda, 
		  function(index,item){
		      $('#agenda').append( item + '<br/>');
		  });
	  // Fill goals table
	  sHTML = "<table width='100%' border='1'>";
	  sHTML += "<tr><th>Goal</th><th>Status</th></tr>";
	  $.each( data.hypotheses, 
		  function(index,item){
		      val = data.wm[item] == 1 ? "true" : "false";
		      deco = data.wm[item] == 1 ? "booltrue" : "boolfalse";
		      sHTML += "<tr>";
		      sHTML += "<td><span class='" + deco + "'>" + 
			  item +
			  "</span></td>" +
			  "<td><span class='" + deco + "'>" +
			  val + 
			  "</span></td>";
		  });
	  $('#hypos').append( sHTML );
	  // Fill signs table
	  sHTML = "<table width='100%' border='1'>";
	  sHTML += "<tr><th>Sign</th><th>Value</th></tr>";
	  $.each( data.signs, 
		  function(index,item){
		      deco = "valuedsign";
		      sHTML += "<tr>";
		      sHTML += "<td><span class='" + deco + "'>" + 
			  item +
			  "</span></td>" +
			  "<td><span class='" + deco + "'>" +
			  data.wm[item] + 
			  "</span></td>";
		  });
	  $('#signs').append( sHTML );
      }
  );
};

// The following functions access the NClose Web Service

/** @brief Resumes remote inference */
nkbNext = function(){
    var sArgs = "couchdoc=" + S_UUID + "&command=next";
    $("#sign").empty();
    $.getJSON(
	"http://127.0.0.1:8000/nkbcouchhttp.sh?" + sArgs,
	function(data){
	    if( data.session == 'running' ){
		$("#sign").text( data.question );
		$("#question").empty().append('What is the value of '+ data.question + '?' );
		$("#qBlock").show();
	    }
	    if( data.session == 'done' ){
		$("#agenda").empty().html("<i>Session Terminated</i>");
	    }
	    redrawEncys();
	}
    );
};

/** Provides response by volunteering data and resuming remote inference */
nkbAnswer = function(){
    var sArgs = 
	"couchdoc=" + S_UUID +
	"&sign=" + $("#sign").text() + 
	"&value=" + $("#answer").val() + 
	"&command=volunteer";
    $("#qBlock").hide();
    // console.log( args );
    $("#answer").val("");
    $.getJSON(
	"http://127.0.0.1:8000/nkbcouchhttp.sh?" + sArgs,
	function(data){
	    nkbNext();
	    redrawEncys();
	}
    );    
};

/** @brief Reinitailizes session */
nkbRestart = function(){
    var sArgs = "couchdoc=" + S_UUID + "&command=restart";
    $("#sign").empty();
    $("#qBlock").hide();
    $.getJSON(
	"http://127.0.0.1:8000/nkbcouchhttp.sh?" + sArgs,
	function(data){
	    // console.log(data);
	    redrawEncys();
	}
    );
};

$(document).ready(
    function(){
	// Hide question block and init globales
	$("#qBlock").hide();
	$("#sign").hide();
	$("#answer").val("");
	// Attach click functions
	$("#butUpdate").click( redrawEncys );
	$("#butNext").click(
	    function(){
		nkbNext();
	    }
	);
	$("#butRestart").click( 
	    function(){
		nkbRestart();
	    }
	);
	$("#butAnswer").click(
	    nkbAnswer
	);
    }
);
