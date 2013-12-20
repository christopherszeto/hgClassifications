//global variables
var activePane = 0;
var activeRow = 0;
var activeRowDefaultClass = "";

function setUp()
{	
	// bind for segfault type errors
	$("#error").bind("ajaxError", function(){
		$(this).hide();
	});

	$('#summaryPanel').dialog({ autoOpen: false }); 
    $('#bookmarkPanel').dialog({ autoOpen: false });

	var dataVars       	= new Object;
	dataVars.cl_mode  	= "loadTasks";
	$.ajax({
		type: "POST",
		url: "/cgi-bin/hgClassifications",
		data: dataVars,
		dataType: "json",
		success: loadTasks,
		cache: true
	});	
}

function highlightRow(row){
	activeRow = row;
	activeRowDefaultClass = $("#jobsTable"+activePane+"_row"+activeRow).attr('class');
	$("#jobsTable"+activePane+"_row"+activeRow).addClass("highlighted");
}

function unhighlightRow(){
	$("#jobsTable"+activePane+"_row"+activeRow).removeClass("highlighted");
    if(activeRowDefaultClass == "even"){
        $("#jobsTable"+activePane+"_row"+activeRow).addClass("even");
    }else{
        $("#jobsTable"+activePane+"_row"+activeRow).addClass("odd");
    }
	activeRow = 0;
	activeRowDefaultClass = "";
}

function lookupTaskIdFromTaskName(taskName){
	var result = 1;
	var found = 0;
	console.log(taskName);
	$("#accordion").children().each(function () {
		if($(this).hasClass('taskTitle')){
			if($(this).text() == taskName){
				found = 1;
				result = this.id.replace("taskTitle_","");
				return false;//acts like break in an each statement
			}else{
				;
			}
		}
	});
	if(found){
		console.log("Found "+result);
		return result;
	}
	return -1;
}

function lookupTaskNameFromTaskId(taskId){
    var result = "";
	var thisId = 1;
    var found = 0;
    $("#accordion").children().each(function () {
        if($(this).hasClass('taskTitle')){
            if(thisId == taskId){
                found = 1;
				result = $(this).text();
                return false;//acts like break in an each statement
            }else{
                thisId++;
            }
        }
    });
    if(found){
        return result;
    }
    return "";
}

function loadLinkDialog(linkVars){
	//hide the original X button, then hack the titlebar so it contains the buttons you want
    $("#bookmarkPanel").dialog({ open: function(event, ui) { $(".ui-dialog-titlebar-close").hide();}});
	$("#bookmarkPanel").prev().append("<span id='bookmarkDialogCloseButton' class='dialogControlButton'></span>");
    $("#bookmarkDialogCloseButton").button({text:false, icons: { primary: "ui-icon-close"} }).click(function() {
        $("#bookmarkPanel").dialog('close');
    });
    $("#bookmarkPanel").dialog('open');
    $("#bookmarkPanel").dialog( "option", "width", 500);
    $("#bookmarkPanel").dialog( "option", "height", 200);
    $("#bookmarkPanel").dialog( "option", "title", "Bookmark me!");
    $("#bookmarkPanel").dialog( "option", "modal", true);
    $("#bookmarkPanel").empty();
	var thisURL = location.protocol+"//"+location.hostname+location.pathname;
	var encodedMode = encodeURIComponent(linkVars.mode);
	var encodedParameters = encodeURIComponent(linkVars.parameters);
	var encodedURL = '';
	if(encodedParameters.length){
		encodedURL=thisURL+"#mode="+encodedMode+"&amp;parameters="+encodedParameters;
	}else{
		encodedURL=thisURL+"#mode="+encodedMode;
	}
	//add the stuff for the link in here
	$("#bookmarkPanel").append("<p>Bookmark the link below to return to your currect view:</p>");
	$("#bookmarkPanel").append("<textarea rows=\'4\' class='text ui-widget-content ui-corner-all' style='width:100%; font-size:0.5em'>"+encodedURL+"</textarea>")
    $(document).ready(function() {
        $("#bookmarkPanel").show();
    });
}

function showLinkError(URLencodedBookmark){
	//hide the original X button, then hack the titlebar so it contains the buttons you want
    $("#summaryPanel").dialog({ open: function(event, ui) { $(".ui-dialog-titlebar-close").hide();}});
    $("#dialogCloseButton").remove();
	$("#summaryPanel").prev().append("<span id='dialogCloseButton' class='dialogControlButton' title='Close'></span>");
    $("#dialogCloseButton").button({text:false, icons: { primary: "ui-icon-close"} }).click(function() {
        $("#summaryPanel").dialog('close');
    })
    .tooltip({
        predelay: 700,
        position: "top-15 center",
        opacity: 0.6
    });
    $("#summaryPanel").dialog('open');
    $("#summaryPanel").dialog( "option", "width", 700);
    $("#summaryPanel").dialog( "option", "height", 200);
    $("#summaryPanel").dialog( "option", "title", "Link Error!");
    $("#summaryPanel").dialog( "option", "modal", true);
    $("#summaryPanel").empty();
    $("#summaryPanel").append("<p>Your bookmarklet <br>"+URLencodedBookmark+"<br>isn't valid.<br><br> Sorry!</p>").hide();
    $(document).ready(function() {
        $("#summaryPanel").show();
    });
}

function openSesame(data) {
	var URLencodedBookmark = data;
	if (URLencodedBookmark.length > 1) {
		URLencodedBookmark = URLencodedBookmark.substr(1); // slice off the #
		//get args		
    	var vars = URLencodedBookmark.split('&');
		var mode = "";
		var parameters = "";
    	for (var i = 0; i < vars.length; i++) {
        	var pair = vars[i].split('=');
			var argName = decodeURIComponent(pair[0]);
			var argVal = decodeURIComponent(pair[1]);
			if(argName == "mode"){
				mode = argVal;
        	}else if(argName == "parameters"){
				parameters = argVal;
			}
    	}

		//do what args suggest
		if(mode == "viewTutorial"){
			viewTutorial();
		}else if(mode == "viewTasksOverview"){
			viewTasksOverview();
		}else if(mode == "viewClassifiersOverview"){
			viewClassifiersOverview();
		}else if(mode == "viewTaskDetails"){
            var taskId = lookupTaskIdFromTaskName(parameters);
            if(taskId < 0){
                showLinkError(data);
            }else{
				viewTaskDetails(taskId);
			}
		}else if(mode == "datasetsSummary"){
			var taskId = lookupTaskIdFromTaskName(parameters);
            if(taskId < 0){
                showLinkError(data);
            }else{
				viewTaskDetails(taskId);
				datasetsSummary(taskId);
			}
		}else if(mode == "featureSelectionsSummary"){
            var taskId = lookupTaskIdFromTaskName(parameters);
            if(taskId < 0){
                showLinkError(data);
            }else{
            	viewTaskDetails(taskId);
            	featureSelectionsSummary(taskId);
			}
		}else if(mode == "classifiersSummary"){
            var taskId = lookupTaskIdFromTaskName(parameters);
            if(taskId < 0){
                showLinkError(data);
            }else{
            	viewTaskDetails(taskId);
            	classifiersSummary(taskId);
			}
		}else if(mode == "samplesSummary"){
            var taskId = lookupTaskIdFromTaskName(parameters);
            if(taskId < 0){
                showLinkError(data);
            }else{
            	viewTaskDetails(taskId);
            	samplesSummary(taskId);
			}
		}else if(mode == "featuresSummary"){
            var taskId = lookupTaskIdFromTaskName(parameters);
			if(taskId < 0){
				showLinkError(data);	
			}else{
                viewTaskDetails(taskId); 
                featuresSummary(taskId);
			}
        }else if(mode == "subgroupsSummary"){
            var taskId = lookupTaskIdFromTaskName(parameters);
            if(taskId < 0){
                showLinkError(data);
            }else{
                viewTaskDetails(taskId);
                subgroupsSummary(taskId);
            }
		}else if(mode == "viewSamples"){
			pair = parameters.split(',');
			var taskName = pair[0];
			var jobId = pair[1];
			var taskId = lookupTaskIdFromTaskName(taskName);
            if(taskId < 0){
                showLinkError(data);
            }else{
                viewTaskDetails(taskId);
				viewSamples(taskId,jobId);
			}
        }else if(mode == "viewFeatures"){
            pair = parameters.split(',');
            var taskName = pair[0];
            var jobId = pair[1];
            var taskId = lookupTaskIdFromTaskName(taskName);
            if(taskId < 0){
                showLinkError(data);
            }else{
                viewTaskDetails(taskId);
                viewFeatures(taskId,jobId);
            }
        }else if(mode == "applyModel"){
            pair = parameters.split(',');
            var taskName = pair[0];
            var jobId = pair[1];
            var taskId = lookupTaskIdFromTaskName(taskName);
            if(taskId < 0){
                showLinkError(data);
            }else{
                viewTaskDetails(taskId);
                applyModel(taskId,jobId);
            }
		}else{
			showLinkError(data);
		}
	}   
}

function loadTasks(data)
//Parse the data JSON object (a list of tasks and their stats) into a sortable table.
{
	// bind for segfault type errors
    $("#error").bind("ajaxError", function(){
        $(this).show();
    });
	//insert buttons for overviews
	$("#mainPanel").append("<table><tr id=\'overviewButtons\'></tr></table>");
	$("#overviewButtons").append("<td><div style='padding: 10px'><img height=100px src='css/images/logo.png'><br><b>Prediction Viewer</b></div></td>");
	$("#overviewButtons").append("<td><div class=\'tableButton\' style=\'width:150px; text-align:center;\' onclick=\'viewTutorial()\'><b>New here?</b><br>View a tutorial</div></td>");
    $("#overviewButtons").append("<td><div class=\'tableButton\' style=\'width:150px; text-align:center;\' onclick=\'hgClassificationsRequest()\'>Request<br>a Prediction</div></td>");
	$("#overviewButtons").append("<td><div class=\'tableButton\' style=\'width:150px; text-align:center;\' onclick=\'viewTasksOverview()\'>View Tasks Overview</div></td>");
	$("#overviewButtons").append("<td><div class=\'tableButton\' style=\'width:150px; text-align:center;\' onclick=\'viewClassifiersOverview()\'>View Classifiers Overview</div></td>");
	//insert accordions for each task
	$("#mainPanel").append("<div id=\"accordion\" class=\'accordion\' width=100%></div>");
	//iterate over all tasks in the JSON
    var tasksArray = $(data).attr('tasks');
	$.each(tasksArray, function(i)
	{
		var task = $(this).attr('name');
		var taskId = $(this).attr('id');
		//add title bar
        $("#accordion").append("<div id=\'taskTitle_"+taskId+"\' class=\'taskTitle\'>"+task+"</div>"); 
		//add content pane (empty right now)
        $("#accordion").append("<div id=\'pane_"+taskId+"\' class=\'accordionPane\'></div>");
		//add control buttons to title bar
		$("#taskTitle_"+taskId).append("<div id=\'taskControls_"+taskId+"\' style=\'float:right\'></div>");
		$("#taskControls_"+taskId).append("<button id=\'openButton_"+taskId+"\' class=\'accordionControlButton\' title=\'Expand task\'></button>");
        $("#taskControls_"+taskId).append("<button id=\'shutButton_"+taskId+"\' class=\'accordionControlButton\' title=\'Collapse task\'></button>");
        $("#taskControls_"+taskId).append("<button id=\'linkButton_"+taskId+"\' class=\'accordionControlButton\' title=\'Generate link\'></button>");
		$("#linkButton_"+taskId)
		.button({
			text: false, 
			icons: { primary: "ui-icon-link"}
		})
		.click(function() {
        	var linkVars       = new Object;
        	linkVars.mode   = "viewTaskDetails";
        	linkVars.parameters = lookupTaskNameFromTaskId(taskId);;
			viewTaskDetails(taskId);
			loadLinkDialog(linkVars);
        })
		.tooltip({
            predelay: 700,
            position: "top-15 center",
            opacity: 0.6
        });
		$("#shutButton_"+taskId)
		.button({
			text: false, icons: { primary: "ui-icon-minus"}
		})
		.click(function() {
			activePane = 0;
            $("#pane_"+taskId).slideUp('normal');
        })
		.tooltip({
            predelay: 700,
            position: "top-15 center",
            opacity: 0.6
        });
		$("#openButton_"+taskId)
		.button({
			text: false, 
			icons: { primary: "ui-icon-plus"}
		})
		.click(function() {
			viewTaskDetails(taskId);
		})
		.tooltip({
			predelay: 700, 
			position: "top-15 center", 
			opacity: 0.6
		});
	});
    $(document).ready(function() {
		//HIDE THE DIVS ON PAGE LOAD	
		$("div.accordionPane").hide();
	});
	//If there are arguments passed by URL deal with them now
    var URLencodedBookmark = location.hash;
	if(URLencodedBookmark.length > 1){
		openSesame(URLencodedBookmark);
	}
}

function viewTutorial()
{
//hide the original X button, then hack the titlebar so it contains the buttons you want
    $("#summaryPanel").dialog({ open: function(event, ui) { $(".ui-dialog-titlebar-close").hide();}});
    $("#dialogCloseButton").remove();
    $("#summaryPanel").prev().append("<span id='dialogCloseButton' class='dialogControlButton' title='Close'></span>");
    $("#dialogCloseButton").button({text:false, icons: { primary: "ui-icon-close"} }).click(function() {
        $("#summaryPanel").dialog('close');
    })
    .tooltip({
        predelay: 700,
        position: "top-15 center",
        opacity: 0.6
    });
    $("#dialogLinkButton").remove();
	$("#summaryPanel").prev().append("<span id='dialogLinkButton' class='dialogControlButton' title='Generate link'></span>");
    $("#dialogLinkButton").button({text:false, icons: { primary: "ui-icon-link"} }).click(function() {
            var linkVars       = new Object;
            linkVars.mode   = "viewTutorial";
            linkVars.parameters = "";
            loadLinkDialog(linkVars);
    })
    .tooltip({
        predelay: 700,
       	position: "top-15 center",
    	opacity: 0.6
    });
	$("#summaryPanel").dialog('open');
    $("#summaryPanel").dialog( "option", "width", 900);
    $("#summaryPanel").dialog( "option", "height", 600);
    $("#summaryPanel").dialog( "option", "title", "Welcome!");
    $("#summaryPanel").dialog( "option", "modal", true);
    $("#summaryPanel").empty();
	$("#summaryPanel").load("tutorial.htm").hide();
    $(document).ready(function() {
		$("#summaryPanel").show();
    });
}

function hgClassificationsRequest()
{	
	window.open("../hgClassificationsRequester-cszeto");
}

function viewTaskDetails(taskId)
//Call the hgClassifications cgi to make a JSON object that displays the jobs in this task
{
	if(taskId != activePane){ //activePane is a global
	    $("div.accordionPane").slideUp('normal');
		$("#pane_"+taskId).slideDown('normal');
        $('html, body').animate({ scrollTop: $("#taskTitle_"+taskId).offset().top, duration:'slow'});
	
		activePane = taskId;
	    $(document).ready(function() {
	    	$("#pane_"+taskId).empty().html("<div id='loadingGif'><img src='css/images/ajax-loader.gif'></div>");
//       	$('html, body').animate({ scrollTop: $("#taskTitle_"+taskId).offset().top, duration:'slow'});
	    });
	
	    var dataVars       = new Object;
	    dataVars.cl_mode   = "loadTaskDetails";
	    dataVars.cl_taskId = taskId;
		
		//check that you haven't already had an ajax request for this table. If not, request it.
	    if($("#jobsTable"+taskId).length == 0){
		    $.ajax({
		        type: "POST",
		        url: "/cgi-bin/hgClassifications",
		        data: dataVars,
		        dataType: "json",
		        success: loadTaskDetails,
		        cache: true
		    });
		}
	}
}

function loadTaskDetails(data)
//Parse the JSON object returned from cgi 'hgClassifications?cl_mode=summarizeFeatures' to a graph and show the summary div
{
	$("#loadingGif").remove();

    var taskId = $(data).attr('id');
	var tableId = "jobsTable"+taskId;

	//Set up the table
	$("#pane_"+taskId).append("<table id=\""+tableId+"\"></table>");

	//set up the table header
    var tableHead = tableId+"_head";
	$("#"+tableId).append("<thead><tr id=\""+tableHead+"\"></tr></thead>");
	$("#"+tableHead).append("<th>Dataset</th>");
    $("#"+tableHead).append("<th>Feature Selection</th>");
    $("#"+tableHead).append("<th>M</th>");
	$("#"+tableHead).append("<th>Data Transformation</th>");
	$("#"+tableHead).append("<th>Classifier</th>");
	$("#"+tableHead).append("<th>Classifier parameters</th>");
	$("#"+tableHead).append("<th>Subgrouping</th>");
	$("#"+tableHead).append("<th>N</th>");
	$("#"+tableHead).append("<th>Accuracy</th>");
	$("#"+tableHead).append("<th>Accuracy gain over majority</th>");
	$("#"+tableHead).append("<th>Accuracy Type</th>");
	$("#"+tableHead).append("<th>&nbsp</th>");
	$("#"+tableHead).append("<th>&nbsp</th>");
    $("#"+tableHead).append("<th>&nbsp</th>");

	//iterate over jobs, addign them to the table
    var jobsArray = $(data).attr('jobs');
	$.each(jobsArray, function(j)
	{
		//Save the info out of the JSON object for this row
		var jobId = $(this).attr('id');
		var dataset = $(this).attr('dataset');
        var featureSelection = $(this).attr('featureSelection');
        var featureSelectionParameters = $(this).attr('featureSelectionParameters');
		if(featureSelection == "filterByIncludeList"){
        	featureSelectionParameters = featureSelectionParameters.replace(/.+\//g, ""); 
		}
        var M = $(this).attr('M');
        var transformation = $(this).attr('transformation');
        var transformationParameters = $(this).attr('transformationParameters');
		var classifier = $(this).attr('classifier');
		var classifierParameters = $(this).attr('classifierParameters');
 		classifierParameters = classifierParameters.replace(/,/g, "\n"); //allows wordwrapping to work. Consider revising.
		var subgrouping = $(this).attr('subgrouping');
		var subgroupingParameters = $(this).attr('subgroupingParameters');
		var N = $(this).attr('N');
        var accuracy = roundVal($(this).attr('accuracy')); 
		var accuracyGain = roundVal($(this).attr('accuracyGain'));
        var accuracyType = $(this).attr('accuracyType'); 
		var featuresAvailable = $(this).attr('featuresAvailable');

		//create the row
		var rowId= tableId+"_row"+jobId;
		$("#"+tableId).append("<tr id=\""+rowId+"\"></tr>");
		$("#"+rowId).append("<td>"+dataset+"</td>");
        $("#"+rowId).append("<td id=\""+rowId+"_featureSelection\" title=\"Parameters:"+featureSelectionParameters+"\">"+featureSelection+"</td>");
        $("#"+rowId).append("<td>"+M+"</td>");
        $("#"+rowId).append("<td id=\""+rowId+"_transformation\" title=\"Parameters:"+transformationParameters+"\">"+transformation+"</td>");
		$("#"+rowId).append("<td>"+classifier+"</td>");
        $("#"+rowId).append("<td>"+classifierParameters+"</td>");
        $("#"+rowId).append("<td id=\""+rowId+"_subgrouping\" title=\"Parameters:"+subgroupingParameters+"\">"+subgrouping+"</td>");
		$("#"+rowId).append("<td>"+N+"</td>");
		$("#"+rowId).append("<td>"+accuracy+"</td>");
		$("#"+rowId).append("<td>"+accuracyGain+"</td>");
		$("#"+rowId).append("<td>"+accuracyType+"</td>");
		$("#"+rowId).append("<td><button class='tableButton' onclick='viewSamples("+taskId+","+jobId+")'>View<br>Samples</button></td>");
		if(featuresAvailable){
        	$("#"+rowId).append("<td><button class='tableButton' onclick='viewFeatures("+taskId+","+jobId+")'>View<br>Features</button></td>");
		}else{
            $("#"+rowId).append("<td>&nbsp</td>");
		}
		if(classifier != "WEKA"){
			$("#"+rowId).append("<td><button class='tableButton' onclick='applyModel("+taskId+","+jobId+")'>Apply<br>model</button></td>");
		}else{
			$("#"+rowId).append("<td>&nbsp</td>");
		}

		//bind tooltips
		$('#'+rowId+"_subgrouping").tooltip({ predelay: 1000, position: "center right", opacity: 0.6});
        $('#'+rowId+"_featureSelection").tooltip({ predelay: 1000, position: "center right", opacity: 0.6});
        $('#'+rowId+"_transformation").tooltip({ predelay: 1000, position: "center right", opacity: 0.6});
	});

	//add the footer buttons
	var tableFooter = tableId+"_footer";
	$("#"+tableId).append("<tfoot><tr id=\'"+tableFooter+"\'></tr></tfoot>");
	$("#"+tableFooter).append("<td><button class='tableButton' onclick='datasetsSummary(\""+taskId+"\")'>View Datasets Summary</button></td>");
    $("#"+tableFooter).append("<td colspan=1><button class='tableButton' onclick='featureSelectionsSummary(\""+taskId+"\")'>View Feature Selections Summary</button></td>");
	$("#"+tableFooter).append("<td colspan=2>&nbsp</td>");
	$("#"+tableFooter).append("<td colspan=2><button class='tableButton' onclick='classifiersSummary(\""+taskId+"\")'>View Classifiers Summary</button></td>");
	$("#"+tableFooter).append("<td colspan=2><button class='tableButton' onclick='subgroupsSummary(\""+taskId+"\")'>View Subgrouping Summary</button></td>");
	$("#"+tableFooter).append("<td colspan=3>&nbsp</td><td><button class='tableButton' onclick='samplesSummary(\""+taskId+"\")'>View Samples Summary</button></td>");
	$("#"+tableFooter).append("<td><button class='tableButton' onclick='featuresSummary(\""+taskId+"\")'>View Feature Summary</button></td>");
	$("#"+tableFooter).append("<td>&nbsp</td>");
	
	$(document).ready(function() {
		$('#'+tableId).dataTable( {
			"bJQueryUI": true,
			"aaSorting": [[ 9, "desc" ]],
			"bSort" : true,
			"aoColumns": [
				null,
				null,
				null,
				null,
				null,
				null,
				null,
				null,
				null,
				null,
				null,
				{ "bSortable": false },
				{ "bSortable": false },
				{ "bSortable": false }
			]
		});
		//update the width of the overall div
   		var mainPanelWidth = $("#"+tableId).width()+20;
    	$("#mainPanel").css("width", mainPanelWidth);
		var overallDivWidth = mainPanelWidth+30;
		$("#overallDiv").css("width", overallDivWidth);
		if(activeRow){
			console.log("Active row is "+activeRow);
			highlightRow(activeRow);
		}
	});
} 

function samplesSummary(taskId)
{
//hide the original X button, then hack the titlebar so it contains the buttons you want
    $("#summaryPanel").dialog({ open: function(event, ui) { $(".ui-dialog-titlebar-close").hide();}});
    $("#dialogCloseButton").remove();
	$("#summaryPanel").prev().append("<span id='dialogCloseButton' class='dialogControlButton'></span>");
    $("#dialogCloseButton").button({text:false, icons: { primary: "ui-icon-close"} }).click(function() {
        $("#summaryPanel").dialog('close');
    });
    $("#dialogLinkButton").remove();
	$("#summaryPanel").prev().append("<span id='dialogLinkButton' class='dialogControlButton'></span>");
    $("#dialogLinkButton").button({text:false, icons: { primary: "ui-icon-link"} }).click(function() {
            var linkVars       = new Object;
            linkVars.mode   = "samplesSummary";
            linkVars.parameters = lookupTaskNameFromTaskId(taskId);
            loadLinkDialog(linkVars);
    });
    $("#summaryPanel").dialog('open');
    $("#summaryPanel").dialog( "option", "width", 900);
    $("#summaryPanel").dialog( "option", "height", 600);
    $("#summaryPanel").dialog( "option", "title", "Samples Summary");
    $("#summaryPanel").dialog( "option", "modal", true);
    $("#summaryPanel" ).bind( "dialogbeforeclose", function(event, ui) {
        $("#summaryGraph").remove();
    });
    $(document).ready(function() {
        $("#summaryPanel").empty().html("<div id='loadingGif'><img src='css/images/ajax-loader.gif'></div>");
        $('html, body').animate({ scrollTop: $("#summaryPanel").offset().top - 40 , duration:'slow'});
    });

	var dataVars       = new Object;
    dataVars.cl_mode   = "summarizeSamples";
    dataVars.cl_taskId = taskId;
    $.ajax({
        type: "POST",
        url: "/cgi-bin/hgClassifications",
        data: dataVars,
        dataType: "json",
        success: loadSamplesSummary,
        cache: true
    });
}

function loadSamplesSummary(data)
//Parse the JSON object returned from cgi 'hgClassifications?cl_mode=summarizeSamples' to a graph and show the summary div
//TODO: hardcoded for 2 subgroups. Extend this.
{
	$("#loadingGif").remove();
	var taskId = $(data).attr('taskId');
    var taskName = lookupTaskNameFromTaskId(taskId);
    var subgroupColorsArray = selectDistinctColors(2);
	var samplesArray = $(data).attr('samples');
	//sort the data by descending total accuracy
	samplesArray.sort(function(a,b) { 
		return ((parseFloat(a.testingValue)+parseFloat(a.trainingValue)) - (parseFloat(b.testingValue)+parseFloat(b.trainingValue))); 
	});
	$("#summaryPanel").append("<div id=\"summaryGraph\"></div>");

	var sampleNames = [];
	var trainingData = [];
    var testingData = [];
	var subgroup1Data = [];
	var subgroup2Data = [];
	var longestName = 0;
	$.each(samplesArray, function(i){
		var name = $(this).attr('name');
       	name = name.replace(/ |\-/g, "_");
		sampleNames.push(name);
		if(longestName < name.length){
			longestName = name.length;
		}
		trainingData.push([$(this).attr('trainingValue'),(i+1)]);
        testingData.push([$(this).attr('testingValue'),(i+1)]);
		var subgroupCountsArray = $(this).attr('subgroupCounts');

        $.each(subgroupCountsArray, function(j){
            var subgroup = $(this).attr('subgroup');
            var count = $(this).attr('count');
            if(subgroup == 1){
                subgroup1Data.push([count, (i+1)]);
            }else if(subgroup == 2){
                subgroup2Data.push([count, (i+1)]);
            }
        });
	});
	var graphDivHeight = 15*samplesArray.length+60;
	var graphDivWidth = 12*longestName + 200;
	//plot the left-panel graph
	$("#summaryGraph").append("<div id=\"summaryGraphLeft\" style='height:"+graphDivHeight+"px; width:"+graphDivWidth+"px; float:left'></div>");
	plot1 = $.jqplot('summaryGraphLeft', [trainingData, testingData], {
  	title: 'Summary of samples',
	seriesDefaults:{
        renderer:$.jqplot.BarRenderer, 
        rendererOptions:{barDirection:'horizontal', barPadding:0, barMargin:2, shadowAlpha:0}
        },
    legend:{show:true, location:'ne'},
	series: [
		{label: 'Training'},
		{label: 'Validation'}
	],
  	axes: {
    	xaxis: {
			label: 'Percent Accuracy over Classifications in Task',
			max:100, min:0
    	},
    	yaxis: {
			label: 'Sample',
			renderer: $.jqplot.CategoryAxisRenderer,
			ticks: sampleNames
    	}
  	}	
	});

	$("#summaryGraph").append("<div id=\"summaryGraphRight\" style='height:"+graphDivHeight+"px; width:"+graphDivWidth+"px; float:left'></div>");
    plot2 = $.jqplot('summaryGraphRight', [subgroup1Data, subgroup2Data], {
		legend:{show:true, location:'ne'},
		stackSeries: true, 
        seriesColors: subgroupColorsArray,
    	title: 'Subgroupings',
    	seriesDefaults:{
        	renderer:$.jqplot.BarRenderer,
        	rendererOptions:{barDirection:'horizontal', barPadding:0, barMargin:5, shadowAlpha:0}
   		},
		series: [
			{label: 'Low'},
			{label: 'High'}
		],
		axes: {
			xaxis: {
				label: 'Count',
				min:0
			},
   			yaxis: {
            	label: 'Sample',
            	renderer: $.jqplot.CategoryAxisRenderer,
            	ticks: sampleNames
       		}
		}
    });
}

function featuresSummary(taskId)
{
	//hide the original X button, then hack the titlebar so it contains the buttons you want
    $("#summaryPanel").dialog({ open: function(event, ui) { $(".ui-dialog-titlebar-close").hide();}});
	$("#dialogCloseButton").remove();
	$("#summaryPanel").prev().append("<span id='dialogCloseButton' class='dialogControlButton' title='Close'></span>");
    $("#dialogCloseButton").button({text:false, icons: { primary: "ui-icon-close"} }).click(function() {
        $("#summaryPanel").dialog('close');
    })
    .tooltip({
        predelay: 700,
        position: "top-15 center",
        opacity: 0.6
    });
	$("#dialogLinkButton").remove();
	$("#summaryPanel").prev().append("<span id='dialogLinkButton' class='dialogControlButton' title='Generate link'></span>");
    $("#dialogLinkButton").button({text:false, icons: { primary: "ui-icon-link"} }).click(function() {
            var linkVars       = new Object;
            linkVars.mode   = "featuresSummary";
            linkVars.parameters = lookupTaskNameFromTaskId(taskId);;
            loadLinkDialog(linkVars);
    })
    .tooltip({
        predelay: 700,
        position: "top-15 center",
        opacity: 0.6
    });

    $("#summaryPanel").dialog('open');
    $("#summaryPanel").dialog( "option", "width", 900);
    $("#summaryPanel").dialog( "option", "height", 600);
    $("#summaryPanel").dialog( "option", "title", "Features Summary");
    $("#summaryPanel").dialog( "option", "modal", true);
    $("#summaryPanel" ).bind( "dialogbeforeclose", function(event, ui) {
        $("#summaryGraph").remove();
    });
    $(document).ready(function() {
        $("#summaryPanel").empty().html("<div id='loadingGif'><img src='css/images/ajax-loader.gif'></div>");
        $('html, body').animate({ scrollTop: $("#summaryPanel").offset().top  - 40, duration:'slow'});
    });

    var dataVars       = new Object;
    dataVars.cl_mode   = "summarizeFeatures";
    dataVars.cl_taskId = taskId;
    $.ajax({
        type: "POST",
        url: "/cgi-bin/hgClassifications",
        data: dataVars,
        dataType: "json",
        success: loadFeaturesSummary,
        cache: true
    });
}

function loadFeaturesSummary(data)
{
    $("#loadingGif").remove();
	var taskId = $(data).attr('taskId');
    var taskName = lookupTaskNameFromTaskId(taskId);
	var featuresArray=$(data).attr('features');
	featuresArray = featuresArray.reverse();
    var graphDivHeight = 15*featuresArray.length+60;
    $("#summaryPanel").append("<div id=\"summaryGraph\"></div>");
    $("#summaryGraph").append("<div id=\"summaryGraphLeft\" style='height:"+graphDivHeight+"px; width:500px; float:left'></div>");

	var featureNames = [];
	var weightData = [];
	var maxWeight;
    $.each(featuresArray, function(i){
        featureNames.push($(this).attr('feature'));
		var weight = $(this).attr('weight');
        weightData.push([weight,(i+1)]);
        if(i == 0){
            maxWeight = Math.abs(weight);
        }else{
            if(Math.abs(weight) > maxWeight){
                maxWeight =Math.abs(weight);
            }
        }
    });

    //plot the left-panel graph
    plot1 = $.jqplot('summaryGraphLeft', [weightData], {
    title: 'Summary of features predicting '+taskName,
    seriesDefaults:{
        renderer:$.jqplot.BarRenderer,
        pointLabels: { show: true, location: 'e', edgeTolerance: -15, formatString: '%.4f' },
        rendererOptions:{
			barDirection:'horizontal', 
			barPadding:0, 
			barMargin:2, 
			shadowAlpha:0,
        	fillAndStroke: true,
        	fillToZero: true
			},
        },
    axes: {
        xaxis: {
            label: 'Accumulative Weight in Task',
            min: (-1.5*maxWeight),
            max: (1.5*maxWeight)
        },
        yaxis: {
            label: 'Feature',
            renderer: $.jqplot.CategoryAxisRenderer,
            ticks: featureNames
        }
    	}
    });
}

function viewSamples(taskId, jobId)
//Call the hgClassifications cgi to make a JSON object that summarizes the features in this job
{
	var taskName = lookupTaskNameFromTaskId(taskId);
	$("#summaryPanel").dialog({ open: function(event, ui) { $(".ui-dialog-titlebar-close").hide();}});
    $("#dialogCloseButton").remove();
	$("#summaryPanel").prev().append("<span id='dialogCloseButton' class='dialogControlButton' title='Close'></span>");
    $("#dialogCloseButton").button({text:false, icons: { primary: "ui-icon-close"} }).click(function() {
        $("#summaryPanel").dialog('close');
		unhighlightRow();
    })
    .tooltip({
        predelay: 700,
        position: "top-15 center",
        opacity: 0.6
    });
    $("#dialogLinkButton").remove();
	$("#summaryPanel").prev().append("<span id='dialogLinkButton' class='dialogControlButton' title='Generate link'></span>");
    $("#dialogLinkButton").button({text:false, icons: { primary: "ui-icon-link"} }).click(function() {
        var linkVars       = new Object;
        linkVars.mode   = "viewSamples";
        linkVars.parameters = taskName+","+jobId;
        loadLinkDialog(linkVars);
    })
    .tooltip({
        predelay: 700,
        position: "top-15 center",
        opacity: 0.6
    });

    $("#summaryPanel").dialog('open');
    $("#summaryPanel").dialog( "option", "width", 900);
    $("#summaryPanel").dialog( "option", "height", 600);
    $("#summaryPanel").dialog( "option", "title", "Sample Details");
    $("#summaryPanel").dialog( "option", "modal", true);
    $("#summaryPanel" ).bind( "dialogbeforeclose", function(event, ui) {
        $("#summaryGraph").remove();
        unhighlightRow();
    });
 	$(document).ready(function() {
        $("#summaryPanel").empty().html("<div id='loadingGif'><img src='css/images/ajax-loader.gif'></div>");
		highlightRow(jobId);
        $('html, body').animate({ scrollTop: $("#summaryPanel").offset().top  - 40 , duration:'slow'});
    });
	
    var dataVars       = new Object;
    dataVars.cl_mode   = "viewSamples";
    dataVars.cl_jobId = jobId;
	
    $.ajax({
        type: "POST",
        url: "/cgi-bin/hgClassifications",
        data: dataVars,
        dataType: "json",
        success: loadSamples,
        cache: true
    });
}

function loadSamples(data)
{
    $("#loadingGif").remove();
    $("#summaryPanel").append("<div id=\"summaryGraph\"></div>");
	var subgroupsArray = $(data).attr('subgroups');
    var subgroupCountsArray = [];
    var subgroupNumber = subgroupsArray.length;
	var subgroupColorsArray = selectDistinctColors(subgroupNumber);
	
	//check if any samples have been ommited. If so, put up a warning.
	var ommittedSamplesFlag = 0;
	$.each(subgroupsArray, function(i){
		if($(this).attr('ommittedSamples')){
			ommittedSamplesFlag++;
		}
	});
	if(ommittedSamplesFlag){
		$("#summaryGraph").append("<p>Warning: This is a large cohort. Only samples with inaccurate predictions  will be shown</p>");
	}

	$.each(subgroupsArray, function(i){
    	var sortedTrAccArray = [];
		var sortedTeAccArray = [];
    	var sortedSamplesNamesArray = [];

		//grab the data out of subgroup JSON object
	    var subgroup = $(this).attr('subgroup');
	    var count = $(this).attr('count');
		var ommittedSamples = $(this).attr('ommittedSamples');
	
        //count up the samples in this subgroup for the pie chart
        if(subgroup == 1){
            subgroupCountsArray.push(["Low ("+count+")", count]);
        }else if(subgroup == 2){
            subgroupCountsArray.push(["High ("+count+")", count]);
        }

		//if there are samples to display left over after omitting..
		if(count != ommittedSamples){
	        var samplesArray = $(this).attr('samples');
	
			//sort samples in this subgroup
		   	samplesArray.sort(function(a,b) {
		        return (parseFloat(a.testingAccuracy + a.trainingAccuracy) - parseFloat(b.testingAccuracy + b.trainingAccuracy));
		   	});

			//push the accuracy for each sample
			var longestName = 0;
		    $.each(samplesArray, function(j){
				var name = $(this).attr('name');
				name = name.replace(/ |\-/g, "_");
		        sortedSamplesNamesArray.push(name);
				if(name.length > longestName){
					longestName = name.length;
				}
	            sortedTrAccArray.push([$(this).attr('trainingAccuracy'), j+1]);
		        sortedTeAccArray.push([$(this).attr('testingAccuracy'), j+1]);
		    });
			
			//set up plotting area for this subgroup
			var subgroupGraphDiv = "summaryGraph_subgroup"+subgroup;
	        var graphDivHeight = 15*sortedSamplesNamesArray.length+100;//space for samples plus some for labels
			var graphDivWidth = 12*longestName +200;
	        $("#summaryGraph").append("<div id=\""+subgroupGraphDiv+"\" style='height:"+graphDivHeight+"px; width:"+graphDivWidth+"px; float:left'></div>");
	
			//plot, using color from this subgroup
			var name;
			if(subgroup == 1){
				name = "Low";
			}else if(subgroup == 2){
				name = "High";
			}
		    var plot1 = $.jqplot(subgroupGraphDiv, [sortedTeAccArray,sortedTrAccArray], {
		        title: name,
			    legend:{show:true, location:'se'},
		        seriesDefaults:{
		            renderer:$.jqplot.BarRenderer,
		            rendererOptions:{barDirection:'horizontal', barPadding:0, barMargin:2, shadowAlpha:0}
		        },
	            series:[
	                {label:"Validation"},
	                {label:"Training"}
	            ],
				seriesColors: [subgroupColorsArray[i], lightenHexColor(subgroupColorsArray[i])],
		        axes: {
		            xaxis: {
		                label: 'Accuracy (%)',
		                min:0, max:100
		            },
		            yaxis: {
		                label: 'Sample',
		                renderer: $.jqplot.CategoryAxisRenderer,
		 	        	ticks: sortedSamplesNamesArray
		            }
		        }
		    });
		}//end if there's samples left over after omission
	});

    $("#summaryGraph").append("<div id=\"summaryGraphRight\" style='height:280px; width:170px; float:left'></div>");
    plot2 = $.jqplot('summaryGraphRight', [subgroupCountsArray], {
        title: 'Subgroup proportions',
        seriesDefaults:{renderer:$.jqplot.PieRenderer, rendererOptions:{sliceMargin:2, diameter: 90, shadowAlpha:0}},
        seriesColors: subgroupColorsArray,
        legend:{show:true, location: 's'}
    });

	//hack to show sens and spec
//    var sg1sensitivity = $(data).attr('sg1sensitivity');
//    var sg2sensitivity = $(data).attr('sg2sensitivity');
//    var sg1specificity = $(data).attr('sg1specificity');
//    var sg2specificity = $(data).attr('sg2specificity');
//	$("#summaryGraph").append("<table id=\"summaryGraphTable\" class=\'statTable\'></div>");
//  $("#summaryGraphTable").append("<thead><tr><td>&nbsp</td><td>Sensitivity</td><td>Specificity</td></tr></thead>");
//	$("#summaryGraphTable").append("<tr><td>Subgroup1</td><td>"+roundVal(sg1sensitivity)+"</td><td>"+roundVal(sg1specificity)+"</td></tr>");
//    $("#summaryGraphTable").append("<tr><td>Subgroup2</td><td>"+roundVal(sg2sensitivity)+"</td><td>"+roundVal(sg2specificity)+"</td></tr>");
	//hack over

//	$("#summaryPanel").show();
}

function viewFeatures(taskId, jobId)
//Call the hgClassifications cgi to make a JSON object that summarizes the features in this job
{
	var taskName = lookupTaskNameFromTaskId(taskId);
	$("#summaryPanel").dialog({ open: function(event, ui) { $(".ui-dialog-titlebar-close").hide();}});
    $("#dialogCloseButton").remove();
	$("#summaryPanel").prev().append("<span id='dialogCloseButton' class='dialogControlButton' title='Close'></span>");
    $("#dialogCloseButton").button({text:false, icons: { primary: "ui-icon-close"} }).click(function() {
        $("#summaryGraph").remove();
        $("#summaryPanel").dialog('close');
		unhighlightRow();
    })
    .tooltip({
        predelay: 700,
        position: "top-15 center",
        opacity: 0.6
    });

    $("#dialogLinkButton").remove();
	$("#summaryPanel").prev().append("<span id='dialogLinkButton' class='dialogControlButton' title='Generate link'></span>");
    $("#dialogLinkButton").button({text:false, icons: { primary: "ui-icon-link"} }).click(function() {
            var linkVars       = new Object;
            linkVars.mode   = "viewFeatures";
            linkVars.parameters = taskName+","+jobId;
            loadLinkDialog(linkVars);
    })
    .tooltip({
        predelay: 700,
        position: "top-15 center",
        opacity: 0.6
    });

    $("#summaryPanel").dialog('open');
    $("#summaryPanel").dialog( "option", "width", 900);
    $("#summaryPanel").dialog( "option", "height", 600);
    $("#summaryPanel").dialog( "option", "title", "View Features");
    $("#summaryPanel").dialog( "option", "modal", true);
    $("#summaryPanel" ).bind( "dialogbeforeclose", function(event, ui) {
        $("#summaryGraph").remove();
        unhighlightRow();
    });
    $(document).ready(function() {
        $("#summaryPanel").empty().html("<div id='loadingGif'><img src='css/images/ajax-loader.gif'></div>");
		highlightRow(jobId);
        $('html, body').animate({ scrollTop: $("#summaryPanel").offset().top - 40 , duration:'slow'});
    });

    var dataVars       = new Object;
    dataVars.cl_mode   = "viewFeatures";
    dataVars.cl_jobId = jobId;

    $.ajax({
        type: "POST",
        url: "/cgi-bin/hgClassifications",
        data: dataVars,
        dataType: "json",
        success: loadFeatures,
        cache: true
    });
}

function loadFeatures(data)
{
    $("#loadingGif").remove();
    $("#summaryPanel").append("<div id=\"summaryGraph\"></div>");
    var featuresArray = $(data).attr('features');
	featuresArray = featuresArray.reverse();
	var graphDivHeight = 15*featuresArray.length+60;
    $("#summaryPanel").append("<div id=\"summaryGraph\"></div>");
    $("#summaryGraph").append("<div id=\"summaryGraphLeft\" style='height:"+graphDivHeight+"px; width:500px; float:left'></div>");

    var featureNames = [];
    var weights = [];
	var maxWeight;
    $.each(featuresArray, function(i){
		var feature = $(this).attr('feature');
		var weight = $(this).attr('weight');
        featureNames.push(feature);
        weights.push([weight,(i+1)]);
		if(i == 0){
			maxWeight = Math.abs(weight);
		}else{
			if(Math.abs(weight) > maxWeight){
				maxWeight =Math.abs(weight);
			}
		}
    });

    //plot the left graph
    plot1 = $.jqplot('summaryGraphLeft', [weights], {
    title: "Feature Weights",
    seriesDefaults:{
        renderer:$.jqplot.BarRenderer,
		pointLabels: { show: true, location: 'e', edgeTolerance: -15, formatString: '%.4f' },
        rendererOptions:{barDirection:'horizontal', barPadding:0, barMargin:2, shadowAlpha:0},
        fillAndStroke: true,
		fillToZero: true
        },
    axesDefaults:{
        fontSize: '6pt'
    },
    axes:{
        xaxis:{ 
			label:'Weight',
			min: (-1.5*maxWeight),
            max: (1.5*maxWeight),
			tickInterval: ((maxWeight*3)/5),
   			tickOptions: {
            	formatString: '%.2f'
        		}
			},
        yaxis: {
            label: 'Feature',
            renderer: $.jqplot.CategoryAxisRenderer,
            ticks: featureNames
        	}
    	}
    });
	
	var thisFeatureCount = weights.length;
	var missingFeatureCount = $(data).attr('featureCount') - thisFeatureCount;
	var pieData = [];
	if(missingFeatureCount == 0){ //kludge to fix pies that don't render properly with zero data in them
    	pieData =  [['Features Displayed Here ('+thisFeatureCount+')', thisFeatureCount]];
	}else{
		pieData =  [['Features Displayed Here ('+thisFeatureCount+')', thisFeatureCount],
					['Features Not Displayed Here ('+ missingFeatureCount +')', missingFeatureCount]];
	}
    $("#summaryGraph").append("<div id=\"summaryGraphRight\" style='height:300px; width:200px; float:left'></div>");
    plot2 = $.jqplot('summaryGraphRight', [pieData], {
        title: 'Feature Coverage',
        seriesDefaults:{
			renderer:$.jqplot.PieRenderer, 
			rendererOptions:{
				showDataLabels: true,
				sliceMargin:2, 
				diameter: 90, 
				shadowAlpha:0}
		},
        legend:{show:true, location: 's'}
    });
	
	//re-sort by raw weight
    featuresArray.sort(function(a,b) {
        return (parseFloat(a.weight) - parseFloat(b.weight));
    });
	//create a string of signatures and genes for linking to CGB
	var genes = "";
	var signature = "";
	$.each(featuresArray, function(i){
		var feature = $(this).attr('feature');
		var weight = $(this).attr('weight');
		//scrub out genes with illegal chars in them
		if((feature.indexOf(" ") == -1) && (feature.indexOf(",") == -1) && (feature.indexOf("_") == -1) && (feature.indexOf("-") == -1) && (feature.indexOf("+") == -1) &&(feature.indexOf(".") == -1)){
			if(weight != 0){
	        	genes = genes+feature+",";
	        	if(signature.length != 0){
	           		signature = signature+"+"+feature+"*"+weight;
	        	}else{
	            	signature = feature+"*"+weight;
	        	}
			}
		}
	});
	//var encodedGenes = encodeURIComponent(genes);
	var encodedSignature = encodeURIComponent(signature);
	encodedSignature = encodedSignature.replace(/\-/g, "%2D");
	var encodedTask = encodeURIComponent($(data).attr('task'));
//	var encodedDataset = encodeURIComponent($(data).attr('dataset'));
//	encodedURL="https://genome-cancer.ucsc.edu/proj/site/hgHeatmap/#?dataset="+dataset+"&displayas=geneset&genes="+genes+"&signatureName=TopModel%20"+task+"%20signature&signature="+signature;
	encodedURL="https://genome-cancer.ucsc.edu/proj/site/hgHeatmap/#?dataset=NCI60exp&displayas=geneset&genes="+genes+"&signatureName=TopModel%20"+encodedTask+"%20signature&signature="+encodedSignature;
	$("#summaryPanel").append("<div class=\'tableButton\' style=\'width:200px; text-align:center; float:left;\' onclick=\'window.open(\""+encodedURL+"\")\'>View in UCSC Cancer<br>Genomics Browser</div>");
}

function classifiersSummary(taskId)
{
	$("#summaryPanel").dialog({ open: function(event, ui) { $(".ui-dialog-titlebar-close").hide();}});
    $("#dialogCloseButton").remove();
	$("#summaryPanel").prev().append("<span id='dialogCloseButton' class='dialogControlButton' title='Close'></span>");
    $("#dialogCloseButton").button({text:false, icons: { primary: "ui-icon-close"} }).click(function() {
        $("#summaryPanel").dialog('close');
    })
    .tooltip({
        predelay: 700,
        position: "top-15 center",
        opacity: 0.6
    });
    $("#dialogLinkButton").remove();
	$("#summaryPanel").prev().append("<span id='dialogLinkButton' class='dialogControlButton' title='Generate link'></span>");
    $("#dialogLinkButton").button({text:false, icons: { primary: "ui-icon-link"} }).click(function() {
            var linkVars       = new Object;
            linkVars.mode   = "classifiersSummary";
            linkVars.parameters = lookupTaskNameFromTaskId(taskId);;
            loadLinkDialog(linkVars);
    })
    .tooltip({
        predelay: 700,
        position: "top-15 center",
        opacity: 0.6
    });

    $("#summaryPanel").dialog('open');
    $("#summaryPanel").dialog( "option", "width", 900);
    $("#summaryPanel").dialog( "option", "height", 600);
    $("#summaryPanel").dialog( "option", "title", "Classifiers Summary");
    $("#summaryPanel").dialog( "option", "modal", true);
    $("#summaryPanel" ).bind( "dialogbeforeclose", function(event, ui) {
        $("#summaryGraph").remove();
    });
    $(document).ready(function() {
        $("#summaryPanel").empty().html("<div id='loadingGif'><img src='css/images/ajax-loader.gif'></div>");
        $('html, body').animate({ scrollTop: $("#summaryPanel").offset().top - 40, duration:'slow'});
    });

    var dataVars       = new Object;
    dataVars.cl_mode   = "summarizeClassifiers";
    dataVars.cl_taskId = taskId;
    $.ajax({
        type: "POST",
        url: "/cgi-bin/hgClassifications",
        data: dataVars,
        dataType: "json",
        success: loadClassifiersSummary,
        cache: true
    });
}

function loadClassifiersSummary(data)
{
    $("#loadingGif").remove();

	var taskId = $(data).attr('taskId');
    var taskName = lookupTaskNameFromTaskId(taskId);
    var classifiersArray=$(data).attr('classifiers');
    classifiersArray.sort(compareMedians);//sort by medians
    $("#summaryPanel").append("<div id=\"summaryGraph\"></div>");
	var classifierNames = [];
    var classifierData = [];
	var longestName = 0; 
    $.each(classifiersArray, function(i){
		var name = $(this).attr('name');
		var parameters = $(this).attr('parameters');
		parameters = parameters.replace(/,dataDiscretizer=.+$/g, "");
       	classifierNames.push(name+' '+parameters);
		if(classifierNames[i].length >longestName){
			longestName = classifierNames[i].length;
		}
        classifierData.push([(i+1), $(this).attr('max'), $(this).attr('min'), $(this).attr('median') ]);
    });
    //plot the left-panel graph
	var graphDivHeight = 6*longestName + 250;
    $("#summaryGraph").append("<div id=\"summaryGraphTop\" style='height:"+graphDivHeight+"px; width:700px; float:left'></div>");
    plot1 = $.jqplot('summaryGraphTop', [classifierData], {
        title: 'Summary of classifiers predicting '+taskName,
		series:[{
       		renderer:$.jqplot.OHLCRenderer, 
        	rendererOptions:{ hlc:true }
      	}],
    	axesDefaults: {
        	tickRenderer: $.jqplot.CanvasAxisTickRenderer,
    	},
		axes: {
            xaxis: {
                renderer: $.jqplot.CategoryAxisRenderer,
                ticks: classifierNames,
                tickOptions: {
                    angle: -70,
                    labelPosition: 'end'
                },
            },
      		yaxis: {
				label: 'Accuracy (%)',
				max:100, 
				min:0
      		}
    	}
    });

    classifiersArray.sort(compareMedianGains);//sort by median gain over majority

   	classifierNames = [];
    classifierData = [];
    $.each(classifiersArray, function(i){
        var name = $(this).attr('name');
        var parameters = $(this).attr('parameters');
        parameters = parameters.replace(/,dataDiscretizer=.+$/g, "");
        classifierNames.push(name+' '+parameters);
        classifierData.push([(i+1), $(this).attr('maxGain'), $(this).attr('minGain'), $(this).attr('medianGain') ]);
    });

	//plot the bottom graph
    $("#summaryGraph").append("<div id=\"summaryGraphBottom\" style='height:"+graphDivHeight+"px; width:700px; float:left'></div>");
    plot2 = $.jqplot('summaryGraphBottom', [classifierData], {
        title: 'Summary of classifiers sorted by accuracy gain over majority classifier',
        series:[{
            renderer:$.jqplot.OHLCRenderer,
            rendererOptions:{ hlc:true }
        }],
        axesDefaults: {
            tickRenderer: $.jqplot.CanvasAxisTickRenderer,
        },
        axes: {
            xaxis: {
                renderer: $.jqplot.CategoryAxisRenderer,
                ticks: classifierNames,
                tickOptions: {
                    angle: -70,
                    labelPosition: 'end'
                },
            },
            yaxis: {
                label: 'Accuracy gain (%)',
                max:50,
                min:-50
            }
        }
    });
}

function featureSelectionsSummary(taskId)
{
	$("#summaryPanel").dialog({ open: function(event, ui) { $(".ui-dialog-titlebar-close").hide();}});
    $("#dialogCloseButton").remove();
	$("#summaryPanel").prev().append("<span id='dialogCloseButton' class='dialogControlButton' title='Close'></span>");
    $("#dialogCloseButton").button({text:false, icons: { primary: "ui-icon-close"} }).click(function() {
        $("#summaryPanel").dialog('close');
    })
    .tooltip({
        predelay: 700,
        position: "top-15 center",
        opacity: 0.6
    });
    $("#dialogLinkButton").remove();
	$("#summaryPanel").prev().append("<span id='dialogLinkButton' class='dialogControlButton' title='Generate link'></span>");
    $("#dialogLinkButton").button({text:false, icons: { primary: "ui-icon-link"} }).click(function() {
            var linkVars       = new Object;
            linkVars.mode   = "featureSelectionsSummary";
            linkVars.parameters = lookupTaskNameFromTaskId(taskId);;
            loadLinkDialog(linkVars);
    })
    .tooltip({
        predelay: 700,
        position: "top-15 center",
        opacity: 0.6
    });

    $("#summaryPanel").dialog('open');
    $("#summaryPanel").dialog( "option", "width", 900);
    $("#summaryPanel").dialog( "option", "height", 600);
    $("#summaryPanel").dialog( "option", "title", "Feature Selections Summary");
    $("#summaryPanel").dialog( "option", "modal", true);
    $("#summaryPanel" ).bind( "dialogbeforeclose", function(event, ui) {
        $("#summaryGraph").remove();
    });
    $(document).ready(function() {
        $("#summaryPanel").empty().html("<div id='loadingGif'><img src='css/images/ajax-loader.gif'></div>");
        $('html, body').animate({ scrollTop: $("#summaryPanel").offset().top - 40, duration:'slow'});
    });

    var dataVars       = new Object;
    dataVars.cl_mode   = "summarizeFeatureSelections";
    dataVars.cl_taskId = taskId;
    $.ajax({
        type: "POST",
        url: "/cgi-bin/hgClassifications",
        data: dataVars,
        dataType: "json",
        success: loadFeatureSelectionsSummary,
        cache: true
    });
}

function loadFeatureSelectionsSummary(data)
{
    $("#loadingGif").remove();

	var taskId = $(data).attr('taskId');
    var taskName = lookupTaskNameFromTaskId(taskId);
    var featureSelectionsArray=$(data).attr('featureSelections');
    featureSelectionsArray.sort(compareMedians);//sort by medians
    $("#summaryPanel").append("<div id=\"summaryGraph\"></div>");
    var featureSelectionNames = [];
    var featureSelectionData = [];
    var longestName = 0;
    $.each(featureSelectionsArray, function(i){
        var name = $(this).attr('name');
        var parameters = $(this).attr('parameters');
        if(name == "filterByIncludeList"){
            parameters = parameters.replace(/.+\//g, "");
        }
		var dataset = $(this).attr('dataset');
        //parameters = parameters.replace(/,dataDiscretizer=.+$/g, "");
        featureSelectionNames.push(name+' '+parameters+' '+dataset);
        if(featureSelectionNames[i].length >longestName){
            longestName = featureSelectionNames[i].length;
        }
        featureSelectionData.push([(i+1), $(this).attr('max'), $(this).attr('min'), $(this).attr('median') ]);
    });

    //plot the left-panel graph
    var graphDivHeight = 6*longestName + 250;
    $("#summaryGraph").append("<div id=\"summaryGraphTop\" style='height:"+graphDivHeight+"px; width:700px; float:left'></div>");
    plot1 = $.jqplot('summaryGraphTop', [featureSelectionData], {
        title: 'Summary of Feature Selections predicting '+taskName,
        series:[{
            renderer:$.jqplot.OHLCRenderer,
            rendererOptions:{ hlc:true }
        }],
        axesDefaults: {
            tickRenderer: $.jqplot.CanvasAxisTickRenderer,
        },
        axes: {
            xaxis: {
                renderer: $.jqplot.CategoryAxisRenderer,
                ticks: featureSelectionNames,
                tickOptions: {
                    angle: -70,
                    labelPosition: 'end'
                },
            },
            yaxis: {
                label: 'Accuracy (%)',
                max:100,
                min:0
            }
        }
    });

    featureSelectionsArray.sort(compareMedianGains);//sort by median gain over majority

    featureSelectionNames = [];
    featureSelectionData = [];
    $.each(featureSelectionsArray, function(i){
        var name = $(this).attr('name');
        var parameters = $(this).attr('parameters');
        if(name == "filterByIncludeList"){
            parameters = parameters.replace(/.+\//g, "");
        }
		var dataset = $(this).attr('dataset');
        //parameters = parameters.replace(/,dataDiscretizer=.+$/g, "");
        featureSelectionNames.push(name+' '+parameters+' '+ dataset);
        featureSelectionData.push([(i+1), $(this).attr('maxGain'), $(this).attr('minGain'), $(this).attr('medianGain') ]);
    });

    //plot the bottom graph
    $("#summaryGraph").append("<div id=\"summaryGraphBottom\" style='height:"+graphDivHeight+"px; width:700px; float:left'></div>");
    plot2 = $.jqplot('summaryGraphBottom', [featureSelectionData], {
        title: 'Summary of feature selections sorted by accuracy gain over majority classifier',
        series:[{
            renderer:$.jqplot.OHLCRenderer,
            rendererOptions:{ hlc:true }
        }],
        axesDefaults: {
            tickRenderer: $.jqplot.CanvasAxisTickRenderer,
        },
        axes: {
            xaxis: {
                renderer: $.jqplot.CategoryAxisRenderer,
                ticks: featureSelectionNames,
                tickOptions: {
                    angle: -70,
                    labelPosition: 'end'
                },
            },
            yaxis: {
                label: 'Accuracy gain (%)',
                max:50,
                min:-50
            }
        }
    });
}


function datasetsSummary(taskId)
{
	$("#summaryPanel").dialog({ open: function(event, ui) { $(".ui-dialog-titlebar-close").hide();}});
    $("#dialogCloseButton").remove();
	$("#summaryPanel").prev().append("<span id='dialogCloseButton' class='dialogControlButton' title='Close'></span>");
    $("#dialogCloseButton").button({text:false, icons: { primary: "ui-icon-close"} }).click(function() {
        $("#summaryPanel").dialog('close');
    })
    .tooltip({
        predelay: 700,
        position: "top-15 center",
        opacity: 0.6
    });

    $("#dialogLinkButton").remove();
	$("#summaryPanel").prev().append("<span id='dialogLinkButton' class='dialogControlButton' title='Generate link'></span>");
    $("#dialogLinkButton").button({text:false, icons: { primary: "ui-icon-link"} }).click(function() {
            var linkVars       = new Object;
            linkVars.mode   = "datasetsSummary";
            linkVars.parameters = lookupTaskNameFromTaskId(taskId);;
            loadLinkDialog(linkVars);
    })
    .tooltip({
        predelay: 700,
        position: "top-15 center",
        opacity: 0.6
    });

    $("#summaryPanel").dialog('open');
    $("#summaryPanel").dialog( "option", "width", 900);
    $("#summaryPanel").dialog( "option", "height", 600);
    $("#summaryPanel").dialog( "option", "title", "Datasets Summary");
    $("#summaryPanel").dialog( "option", "modal", true);
    $("#summaryPanel" ).bind( "dialogbeforeclose", function(event, ui) {
        $("#summaryGraph").remove();
    });
    $(document).ready(function() {
        $("#summaryPanel").empty().html("<div id='loadingGif'><img src='css/images/ajax-loader.gif'></div>");
        $('html, body').animate({ scrollTop: $("#summaryPanel").offset().top - 40 , duration:'slow'});
    });

    var dataVars       = new Object;
    dataVars.cl_mode   = "summarizeDatasets";
    dataVars.cl_taskId = taskId;
    $.ajax({
        type: "POST",
        url: "/cgi-bin/hgClassifications",
        data: dataVars,
        dataType: "json",
        success: loadDatasetsSummary,
        cache: true
    });
}

function loadDatasetsSummary(data)
{
    $("#loadingGif").remove();

	var taskId = $(data).attr('taskId');
	var taskName = lookupTaskNameFromTaskId(taskId);
    var datasetsArray=$(data).attr('datasets');
    datasetsArray.sort(compareMedians);//sort by medians

    $("#summaryPanel").append("<div id=\"summaryGraph\"></div>");

    var datasetNames = [];
    var datasetData = [];
	var longestName = 0;
    $.each(datasetsArray, function(i){
        datasetNames.push($(this).attr('name'));
        datasetData.push([(i+1), $(this).attr('max'), $(this).attr('min'), $(this).attr('median') ]);
		if(datasetNames[i].length > longestName){
			longestName = datasetNames[i].length;
		}
    });

    //plot the top panel graph
	var graphDivHeight = 6*longestName + 250;
    $("#summaryGraph").append("<div id=\"summaryGraphTop\" style='height:"+graphDivHeight+"px; width:700px; float:left'></div>");
    plot1 = $.jqplot('summaryGraphTop', [datasetData], {
        title: 'Summary of dataset accuracy predicting '+taskName,
        series:[{
            renderer:$.jqplot.OHLCRenderer,
            rendererOptions:{ hlc:true }
        }],
        axesDefaults: {
            tickRenderer: $.jqplot.CanvasAxisTickRenderer,
        },
        axes: {
            xaxis: {
                renderer: $.jqplot.CategoryAxisRenderer,
                ticks: datasetNames,
                tickOptions: {
                    angle: -70,
                    labelPosition: 'end'
                },
            },
            yaxis: {
                label: 'Accuracy (%)',
				max:100, 
				min:0
            }
        }
    });

	//resort by % gain and plot again
    $("#summaryGraph").append("<div id=\"summaryGraphBottom\" style='height:"+graphDivHeight+"px; width:700px; float:left'></div>");
	datasetsArray.sort(compareMedianGains);//sort by medians
    datasetNames = [];
    datasetData = [];
    $.each(datasetsArray, function(i){
        datasetNames.push($(this).attr('name'));
        datasetData.push([(i+1), $(this).attr('maxGain'), $(this).attr('minGain'), $(this).attr('medianGain') ]);
    });

    //plot the left-panel graph
    plot2 = $.jqplot('summaryGraphBottom', [datasetData], {
        title: 'Summary of datasets sorted by accuracy gain over majority classifier',
        series:[{
            renderer:$.jqplot.OHLCRenderer,
            rendererOptions:{ hlc:true }
        }],
        axesDefaults: {
            tickRenderer: $.jqplot.CanvasAxisTickRenderer,
        },
        axes: {
            xaxis: {
                renderer: $.jqplot.CategoryAxisRenderer,
                ticks: datasetNames,
                tickOptions: {
                    angle: -70,
                    labelPosition: 'end'
                },
            },
            yaxis: {
                label: 'Accuracy gain (%)',
                max:50,
                min:-50
            }
        }
    });

}

function subgroupsSummary(taskId)
{
    $("#summaryPanel").dialog({ open: function(event, ui) { $(".ui-dialog-titlebar-close").hide();}});
    $("#dialogCloseButton").remove();
    $("#summaryPanel").prev().append("<span id='dialogCloseButton' class='dialogControlButton' title='Close'></span>");
    $("#dialogCloseButton").button({text:false, icons: { primary: "ui-icon-close"} }).click(function() {
        $("#summaryPanel").dialog('close');
    })
    .tooltip({
        predelay: 700,
        position: "top-15 center",
        opacity: 0.6
    });

    $("#dialogLinkButton").remove();
    $("#summaryPanel").prev().append("<span id='dialogLinkButton' class='dialogControlButton' title='Generate link'></span>");
    $("#dialogLinkButton").button({text:false, icons: { primary: "ui-icon-link"} }).click(function() {
            var linkVars       = new Object;
            linkVars.mode   = "subgroupsSummary";
            linkVars.parameters = lookupTaskNameFromTaskId(taskId);;
            loadLinkDialog(linkVars);
    })
    .tooltip({
        predelay: 700,
        position: "top-15 center",
        opacity: 0.6
    });

    $("#summaryPanel").dialog('open');
    $("#summaryPanel").dialog( "option", "width", 900);
    $("#summaryPanel").dialog( "option", "height", 600);
    $("#summaryPanel").dialog( "option", "title", "Subgroups Summary");
    $("#summaryPanel").dialog( "option", "modal", true);
    $("#summaryPanel" ).bind( "dialogbeforeclose", function(event, ui) {
        $("#summaryGraph").remove();
    });
    $(document).ready(function() {
        $("#summaryPanel").empty().html("<div id='loadingGif'><img src='css/images/ajax-loader.gif'></div>");
        $('html, body').animate({ scrollTop: $("#summaryPanel").offset().top - 40 , duration:'slow'});
    });

    var dataVars       = new Object;
    dataVars.cl_mode   = "summarizeSubgroups";
    dataVars.cl_taskId = taskId;
    $.ajax({
        type: "POST",
        url: "/cgi-bin/hgClassifications",
        data: dataVars,
        dataType: "json",
        success: loadSubgroupsSummary,
        cache: true
    });
}

function loadSubgroupsSummary(data)
{
    $("#loadingGif").remove();

    var taskId = $(data).attr('taskId');
    var taskName = lookupTaskNameFromTaskId(taskId);
    var subgroupsArray=$(data).attr('subgroups');
    subgroupsArray.sort(compareMedians);//sort by medians

    var subgroupNames = [];
    var subgroupData = [];
	var longestName = 0;
    $.each(subgroupsArray, function(i){
        subgroupNames.push($(this).attr('name'));
        subgroupData.push([(i+1), $(this).attr('max'), $(this).attr('min'), $(this).attr('median') ]);
        if(subgroupNames[i].length >longestName){
            longestName = subgroupNames[i].length;
        }
    });
    //plot the left-panel graph
    $("#summaryPanel").append("<div id=\"summaryGraph\"></div>");
    var graphDivHeight = 6*longestName + 250;
    $("#summaryGraph").append("<div id=\"summaryGraphTop\" style='height:"+graphDivHeight+"px; width:700px; float:left'></div>");

    //plot the top panel graph
    plot1 = $.jqplot('summaryGraphTop', [subgroupData], {
        title: 'Summary of subgrouping accuracies predicting '+taskName,
        series:[{
            renderer:$.jqplot.OHLCRenderer,
            rendererOptions:{ hlc:true }
        }],
        axesDefaults: {
            tickRenderer: $.jqplot.CanvasAxisTickRenderer,
        },
        axes: {
            xaxis: {
                renderer: $.jqplot.CategoryAxisRenderer,
                ticks: subgroupNames,
                tickOptions: {
                    angle: -70,
                    labelPosition: 'end'
                },
            },
            yaxis: {
                label: 'Accuracy (%)',
                max:100,
                min:0
            }
        }
    });

    //resort by % gain and plot again
    $("#summaryGraph").append("<div id=\"summaryGraphBottom\" style='height:"+graphDivHeight+"px; width:700px; float:left'></div>");
    subgroupsArray.sort(compareMedianGains);//sort by medians
    subgroupNames = [];
    subgroupData = [];
    $.each(subgroupsArray, function(i){
        subgroupNames.push($(this).attr('name'));
        subgroupData.push([(i+1), $(this).attr('maxGain'), $(this).attr('minGain'), $(this).attr('medianGain') ]);
    });

    //plot the left-panel graph
    plot2 = $.jqplot('summaryGraphBottom', [subgroupData], {
        title: 'Summary of subgroups sorted by accuracy gain over majority classifier',
        series:[{
            renderer:$.jqplot.OHLCRenderer,
            rendererOptions:{ hlc:true }
        }],
        axesDefaults: {
            tickRenderer: $.jqplot.CanvasAxisTickRenderer,
        },
        axes: {
            xaxis: {
                renderer: $.jqplot.CategoryAxisRenderer,
                ticks: subgroupNames,
                tickOptions: {
                    angle: -70,
                    labelPosition: 'end'
                },
            },
            yaxis: {
                label: 'Accuracy gain (%)',
                max:50,
                min:-50
            }
        }
    });

}

function viewTasksOverview()
{
    //hide the original X button, then hack the titlebar so it contains the buttons you want
    $("#summaryPanel").dialog({ open: function(event, ui) { $(".ui-dialog-titlebar-close").hide();}});
	$("#summaryPanel").dialog( "option", "title", "Tasks Overview");
    $("#dialogCloseButton").remove();
    $("#summaryPanel").prev().append("<span id='dialogCloseButton' class='dialogControlButton' title='Close'></span>");
    $("#dialogCloseButton").button({text:false, icons: { primary: "ui-icon-close"} }).click(function() {
		$("#summaryPanel").dialog('close');
    })
    .tooltip({
        predelay: 700,
        position: "top-15 center",
        opacity: 0.6
    });
    $("#dialogLinkButton").remove();
	$('.ui-dialog-titlebar').append("<span id='dialogLinkButton' class='dialogControlButton' title='Generate link'></span>");
    $("#dialogLinkButton").button({text:false, icons: { primary: "ui-icon-link"} }).click(function() {
            var linkVars       = new Object;
            linkVars.mode   = "viewTasksOverview";
            linkVars.parameters = "";
            loadLinkDialog(linkVars);
    })
    .tooltip({
        predelay: 700,
        position: "top-15 center",
        opacity: 0.6
    });

	//open the dialog
	$("#summaryPanel").dialog( 'open' );
    $("#summaryPanel").dialog( "option", "width", 900);
    $("#summaryPanel").dialog( "option", "height", 600);
    $("#summaryPanel").dialog( "option", "modal", true);
    $("#summaryPanel" ).bind( "dialogbeforeclose", function(event, ui) {
        $("#summaryGraph").remove();
    });
    $(document).ready(function() {
        $("#summaryPanel").empty().html("<div id='loadingGif'><img src='css/images/ajax-loader.gif'></div>");
        $('html, body').animate({ scrollTop: $("#summaryPanel").offset().top  - 40, duration:'slow'});
    });

    var dataVars       = new Object;
    dataVars.cl_mode   = "overviewTasks";
    $.ajax({
        type: "POST",        
		url: "/cgi-bin/hgClassifications",
        data: dataVars,
        dataType: "json",
        success: loadTasksOverview,
        cache: true
    });
}

function loadTasksOverview(data)
{
    $("#loadingGif").remove();

    var tasksArray=$(data).attr('tasks');
	tasksArray.sort(compareMedians);//sort by medians

    var taskNames = [];
    var taskData = [];
	var longestName = 0;
    $.each(tasksArray, function(i){
		name = $(this).attr('name');
        taskNames.push(name);
		if(longestName < name.length){
			longestName = name.length;
		}
        taskData.push([(i+1), $(this).attr('max'), $(this).attr('min'), $(this).attr('median') ]);
    });

    var graphDivWidth = tasksArray.length*20 + 150;
	var graphDivHeight = longestName*6 + 300;
    $("#summaryPanel").append("<div id=\"summaryGraph\"></div>");
    $("#summaryGraph").append("<div id=\"summaryGraphTop\" style='height:"+graphDivHeight+"px; width:"+graphDivWidth+"px'; float:left'></div>");

    plot1 = $.jqplot('summaryGraphTop', [taskData], {
        title: 'Tasks sorted by accuracy',
        series:[{
            renderer:$.jqplot.OHLCRenderer,
            rendererOptions:{ hlc:true }
        }],
        axesDefaults: {
            tickRenderer: $.jqplot.CanvasAxisTickRenderer,
        },
        axes: {
            xaxis: {
                renderer: $.jqplot.CategoryAxisRenderer,
                ticks: taskNames,
            	tickOptions: {
                	angle: -70,
                    labelPosition: 'end'
            	},
            },
            yaxis: {
                label: 'Accuracy (%)',
				max:100,
				min:0
            }
        }
    });
	
	//re-sort by median gain and plot again
    tasksArray.sort(compareMedianGains);//sort by medians

    taskNames = [];
    taskData = [];
    $.each(tasksArray, function(i){
        taskNames.push($(this).attr('name'));
        taskData.push([(i+1), $(this).attr('maxGain'), $(this).attr('minGain'), $(this).attr('medianGain') ]);
    });

    $("#summaryGraph").append("<div id=\"summaryGraphBottom\" style='height:"+graphDivHeight+"px; width:"+graphDivWidth+"px'; float:left'></div>");
    plot2 = $.jqplot('summaryGraphBottom', [taskData], {
        title: 'Tasks sorted by accuracy gain',
        series:[{
            renderer:$.jqplot.OHLCRenderer,
            rendererOptions:{ hlc:true }
        }],
        axesDefaults: {
            tickRenderer: $.jqplot.CanvasAxisTickRenderer,
        },
        axes: {
            xaxis: {
                renderer: $.jqplot.CategoryAxisRenderer,
                ticks: taskNames,
                tickOptions: {
                    angle: -70,
                    labelPosition: 'end'
                },
            },
            yaxis: {
                label: 'Accuracy gain (%)',
                max:50,
                min:-50
            }
        }
    });

}


function viewClassifiersOverview()
{
	//hide the original X button, then hack the titlebar so it contains the buttons you want
    $("#summaryPanel").dialog({ open: function(event, ui) { $(".ui-dialog-titlebar-close").hide();}});
    $("#dialogCloseButton").remove();
	$("#summaryPanel").prev().append("<span id='dialogCloseButton' class='dialogControlButton' title='Close'></span>");
    $("#dialogCloseButton").button({text:false, icons: { primary: "ui-icon-close"} }).click(function() {
        $("#summaryPanel").dialog('close');
    })
    .tooltip({
        predelay: 700,
        position: "top-15 center",
        opacity: 0.6
    });
    $("#dialogLinkButton").remove();
	$("#summaryPanel").prev().append("<span id='dialogLinkButton' class='dialogControlButton' title='Generate link'></span>");
    $("#dialogLinkButton").button({text:false, icons: { primary: "ui-icon-link"} }).click(function() {
            var linkVars       = new Object;
            linkVars.mode   = "viewClassifiersOverview";
            linkVars.parameters = "";
            loadLinkDialog(linkVars);
    })
    .tooltip({
        predelay: 700,
        position: "top-15 center",
        opacity: 0.6
    });
    $("#summaryPanel").dialog('open');
    $("#summaryPanel").dialog( "option", "width", 900);
    $("#summaryPanel").dialog( "option", "height", 600);
    $("#summaryPanel").dialog( "option", "title", "Classifiers Overview");
    $("#summaryPanel").dialog( "option", "modal", true);
    $("#summaryPanel" ).bind( "dialogbeforeclose", function(event, ui) {
        $("#summaryGraph").remove();
    });
    $(document).ready(function() {
        $("#summaryPanel").empty().html("<div id='loadingGif'><img src='css/images/ajax-loader.gif'></div>");
        $('html, body').animate({ scrollTop: $("#summaryPanel").offset().top - 40, duration:'slow'});
    });

    var dataVars       = new Object;
    dataVars.cl_mode   = "overviewClassifiers";
    $.ajax({
        type: "POST",
        url: "/cgi-bin/hgClassifications",
        data: dataVars,
        dataType: "json",
        success: loadClassifiersOverview,
        cache: true
    });
}

function loadClassifiersOverview(data)
{
    $("#loadingGif").remove();

    var classifiersArray=$(data).attr('classifiers');
    classifiersArray.sort(compareMedians);//sort by medians
    $("#summaryPanel").append("<div id=\"summaryGraph\"></div>");
    var classifierNames = [];
    var classifierData = [];
	var longestName = 0;
    $.each(classifiersArray, function(i){
        var name = $(this).attr('name');
        var parameters = $(this).attr('parameters');
        parameters = parameters.replace(/,dataDiscretizer=.+$/g, "");
        classifierNames.push(name+' '+parameters);
		if(classifierNames[i].length > longestName){
			longestName = classifierNames[i].length;
		}
        classifierData.push([(i+1), $(this).attr('max'), $(this).attr('min'), $(this).attr('median') ]);
    });
	var graphDivHeight = 6*longestName + 200;
    //plot the left-panel graph
    $("#summaryGraph").append("<div id=\"summaryGraphTop\" style='height:"+graphDivHeight+"px; width:700px; float:left'></div>");
    plot1 = $.jqplot('summaryGraphTop', [classifierData], {
        title: 'Summary of classifiers',
        series:[{
            renderer:$.jqplot.OHLCRenderer,
            rendererOptions:{ hlc:true }
        }],
        axesDefaults: {
            tickRenderer: $.jqplot.CanvasAxisTickRenderer,
        },
        axes: {
            xaxis: {
                renderer: $.jqplot.CategoryAxisRenderer,
                ticks: classifierNames,
                tickOptions: {
                    angle: -70,
                    labelPosition: 'end'
                },
            },
            yaxis: {
                label: 'Accuracy (%)',
                max:100,
                min:0
            }
        }
    });


	//re-sort and plot again
    classifiersArray.sort(compareMedianGains);//sort by medians
    $("#summaryGraph").append("<div id=\"summaryGraphBottom\" style='height:"+graphDivHeight+"px; width:700px; float:left'></div>");

    classifierNames = [];
    classifierData = [];
    $.each(classifiersArray, function(i){
        var name = $(this).attr('name');
        var parameters = $(this).attr('parameters');
        parameters = parameters.replace(/,dataDiscretizer=.+$/g, "");
        classifierNames.push(name+' '+parameters);
        classifierData.push([(i+1), $(this).attr('maxGain'), $(this).attr('minGain'), $(this).attr('medianGain') ]);
    });

    //plot the left-panel graph
    plot2 = $.jqplot('summaryGraphBottom', [classifierData], {
        title: 'Summary of classifiers sorted by accuracy gain over majority classifier',
        series:[{
            renderer:$.jqplot.OHLCRenderer,
            rendererOptions:{ hlc:true }
        }],
        axesDefaults: {
            tickRenderer: $.jqplot.CanvasAxisTickRenderer,
        },
        axes: {
            xaxis: {
                renderer: $.jqplot.CategoryAxisRenderer,
                ticks: classifierNames,
                tickOptions: {
                    angle: -70,
                    labelPosition: 'end'
                },
            },
            yaxis: {
                label: 'Accuracy gain (%)',
                max:50,
                min:-50
            }
        }
    });
}


function applyModel(taskId, jobId)
{
	var taskName = lookupTaskNameFromTaskId(taskId);
	$("#summaryPanel").empty();
	//hide the original X button, then hack the titlebar so it contains the buttons you want
    $("#summaryPanel").dialog({ open: function(event, ui) { $(".ui-dialog-titlebar-close").hide();}});
    $("#dialogCloseButton").remove();
	$("#summaryPanel").prev().append("<span id='dialogCloseButton' class='dialogControlButton' title='Close'></span>");
    $("#dialogCloseButton").button({text:false, icons: { primary: "ui-icon-close"} }).click(function() {
        $("#summaryGraph").remove();
        $("#summaryPanel").dialog('close');
		unhighlightRow();
    })
    .tooltip({
        predelay: 700,
        position: "top-15 center",
        opacity: 0.6
    });
    $("#dialogLinkButton").remove();
	$("#summaryPanel").prev().append("<span id='dialogLinkButton' class='dialogControlButton' title='Generate link'></span>");
    $("#dialogLinkButton").button({text:false, icons: { primary: "ui-icon-link"} }).click(function() {
            var linkVars       = new Object;
            linkVars.mode   = "applyModel";
            linkVars.parameters = taskName+","+jobId;
            loadLinkDialog(linkVars);
    })
    .tooltip({
        predelay: 700,
        position: "top-15 center",
        opacity: 0.6
    });
    $("#summaryPanel").dialog('open');
    $("#summaryPanel").dialog( "option", "width", 900);
    $("#summaryPanel").dialog( "option", "height", 600);
    $("#summaryPanel").dialog( "option", "title", "Apply Model");
    $("#summaryPanel").dialog( "option", "modal", true);
    $("#summaryPanel" ).bind( "dialogbeforeclose", function(event, ui) {
        $("#summaryGraph").remove();
        unhighlightRow();
    });
	$("#summaryPanel").append("<form id=\'dataSubmissionForm\' action=\'/cgi-bin/hgClassifications\' method=\'POST\' enctype=\'multipart/form-data\' encoding=\'multipart/form-data\'></form>");
	$("#dataSubmissionForm").append("<input type=\'hidden\' name=\'MAX_FILE_SIZE\' value=\'500000\'>");
    $("#dataSubmissionForm").append("<input type=\'hidden\' name=\'cl_mode\' value=\'applyModel\'>");
    $("#dataSubmissionForm").append("<input type=\'hidden\' name=\'cl_jobId\' value="+jobId+">");
	$("#dataSubmissionForm").append("<label>Please select your data file <br>[500KB max, tab separated labeled file of genes by samples]:<br><br></label>");
	$("#dataSubmissionForm").append("<input type=\'file\' name=\'cl_userData\'><br><br>");
//	$("#dataSubmissionForm").append("<label>Or provide a URL to your data file: </label><br>");
//	$("#dataSubmissionForm").append("<input type=\'text\' name=\'cl_userDataURL\' style=\'border-style:solid; border-color:#999999; border-width:1px\' size=60><br><br>");
	$("#dataSubmissionForm").append("<input type=\'submit\' class=\'tableButton\' value=\'Submit\'>");
	
    $(document).ready(function() {
		highlightRow(jobId);
		var options = { 
	        target: '#summaryPanel',
	        clearForm: true, 
	        resetForm: true,
			dataType: 'json',
			success: viewAppliedModel
    	};
		$('#dataSubmissionForm').submit(function(){
            $("#summaryPanel").empty().html("<div id='loadingGif'><img src='css/images/ajax-loader.gif'></div>");
            $('html, body').animate({ scrollTop: $("#summaryPanel").offset().top - 40 , duration:'slow'});
			highlightRow(jobId);
			$(this).ajaxSubmit(options); 
			return false;
		});
	});
}

function viewAppliedModel(data)
{
    $("#error").bind("ajaxError", function(){
        $(this).show();
    });
	
 	var usedFeatures = $(data).attr('matchedFeatures');
    var unusedFeatures = $(data).attr('modelFeatures') - $(data).attr('matchedFeatures');
	var largeCohort = $(data).attr('largeCohort');

	if(usedFeatures == 0){
		$("#summaryPanel").empty();
		$("#summaryPanel").append("This model can't be applied to your data, as there are no matching features");
	}else{
        $("#summaryPanel").empty();
        $("#summaryPanel").append("<div id=\"summaryGraph\"></div>");
		if(largeCohort > 0){
			$("#summaryGraph").append("<p>Warning: This model is from a large cohort. This display will only show max and min prediction scores from the training set.</p>")
		}

		var samplesArray = $(data).attr('samples');
		samplesArray.sort(function(a,b) {
			return (parseFloat(a.predictedClass) - parseFloat(b.predictedClass));
		});
		var userSamples = [];
		var trainingSamples = [];
		var sampleNames = [];
		var maxPrediction = samplesArray[0].predictedClass;
		var longestName = 0;
		$.each(samplesArray, function(i){
			var name = $(this).attr('name');
			sampleNames.push(name);
			if(longestName <  name.length){
				longestName = name.length;
			}
			var thisPrediction = $(this).attr('predictedClass');
			if($(this).attr('userSample') == 1){
                userSamples.push(thisPrediction);
				trainingSamples.push(thisPrediction);
			}else{
                trainingSamples.push(thisPrediction);
				userSamples.push(null);
			}
			if(Math.abs(thisPrediction) > maxPrediction){
				maxPrediction = Math.abs(thisPrediction);
			}
		});
		var graphDivWidth = 15*samplesArray.length+200;
		if(graphDivWidth < 400){
			graphDivWidth = 400;
		}
		var graphDivHeight = 6*longestName + 300;
		$("#summaryGraph").append("<div id=\"summaryGraphLeft\" style='width:"+graphDivWidth+"px; height:"+graphDivHeight+"px; float:left'></div>");
		plot1 = $.jqplot('summaryGraphLeft', [ trainingSamples, userSamples ], {
   			axes: {
        		yaxis: {
            		label: 'Prediction Score',
            		max:(1.5*maxPrediction), 
					min:(-1.5*maxPrediction),
					tickOptions: {
						formatString: '%.4f'
					},
        		},
        		xaxis: {
            		label: 'Sample',
            		renderer: $.jqplot.CategoryAxisRenderer,
            		ticks: sampleNames,
                	tickRenderer: $.jqplot.CanvasAxisTickRenderer,
                	tickOptions: {
                    	angle: -70
                	}
        		}
    		},
			series:[ 
            {
                label: "All Samples",
                lineWidth: 1,
                showMarker: false,
                fill: true,
                fillToZero: true,
                fillAxis: 'y',
                useNegativeColors: false,
                shadow: false,
                rendererOptions: {
                    highlightMouseOver: false,
                    highlightMouseDown: false,
                    highlightColor: null
                }
            },
           	{
                label: "User Submitted Samples",
                showLine: false,
                markerOptions: { size: 8, style:"x" },
                pointLabels: { show: true, location: 'n', edgeTolerance: -15, formatString: '%.4f' },
                rendererOptions: {
                    highlightMouseOver: true,
                    highlightMouseDown: true,
                },
            },
			],
			seriesColors: [ '#D4D4C9', '#53A6CF' ],
			legend: {show: true, location: 'nw'}
		});
	
	    var pieData = [];
	    if(unusedFeatures == 0){ //kludge to fix pies that don't render properly with zero data in them
	        pieData =  [['Model Features Used ('+usedFeatures+')', usedFeatures]];
	    }else{
	        pieData =  [['Model Features Used ('+usedFeatures+')', usedFeatures],
	                    ['Model Features Left Unused ('+ unusedFeatures +')', unusedFeatures]];
	    }
	    $("#summaryGraph").append("<div id=\"summaryGraphRight\" style='height:300px; width:200px; float:left'></div>");
	    plot2 = $.jqplot('summaryGraphRight', [pieData], {
	        title: 'Model Usage',
	        seriesDefaults:{
	            renderer:$.jqplot.PieRenderer,
	            rendererOptions:{
	                showDataLabels: true,
	                sliceMargin:2,
	                diameter: 90,
	                shadowAlpha:0}
	        },
	        legend:{show:true, location: 's'}
	    });
	}
}

// UTILITY FUNCTIONS
function compareMedians(a,b){
    var med1 = $(a).attr('median');
    var med2 = $(b).attr('median');
    return med2 - med1;
}

function compareMedianGains(a,b){
    var med1 = $(a).attr('medianGain');
    var med2 = $(b).attr('medianGain');
    return med2 - med1;
}


function roundVal(val){
	var dec = 2;
	var result = Math.round(val*Math.pow(10,dec))/Math.pow(10,dec);
	return result;
}

function selectDistinctColors(subgroupNumber)
//TODO: make this handle more than 6 distinct colors.
{
	if(subgroupNumber > 6){
    	$("#error").bind("ajaxError", function(){
        	$(this).show();
   		});
	}
	var palette = ["#333399","#339933", "#339999", "#993333", "#993399", "#999933"];
	var step = Math.floor(palette.length / subgroupNumber);
	var result = [];
	for(i = 0; i < subgroupNumber; i++)
	{
		result.push(palette[i*step]);		
	}
	return result;
}

function lightenHexColor(color)
//TODO: check no substring val gets over 255. 
{
	var addVal = 80;
	return "#"+toHex(parseInt((cutHex(color)).substring(0,2),16)+addVal)+toHex(parseInt((cutHex(color)).substring(2,4),16)+addVal)+toHex(parseInt((cutHex(color)).substring(4,6),16)+addVal);
}

function cutHex(h) {
	return (h.charAt(0)=="#") ? h.substring(1,7):h
}

function toHex(N) {
 if (N==null) return "00";
 N=parseInt(N); if (N==0 || isNaN(N)) return "00";
 N=Math.max(0,N); N=Math.min(N,255); N=Math.round(N);
 return "0123456789ABCDEF".charAt((N-N%16)/16)
      + "0123456789ABCDEF".charAt(N%16);
}

