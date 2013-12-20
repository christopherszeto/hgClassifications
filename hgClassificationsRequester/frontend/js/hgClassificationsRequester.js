//GLOBALS
var timeOut;

//FUNCTIONS
function setUp()
{
	$("#mainPanel").append("<form id=\'requestForm\'></form>");
    $('#summaryPanel').dialog({ autoOpen: false });
	createEmptyForm();
}

function createEmptyForm(){
	$("#requestForm").empty();
	$("#requestForm").append("<table id=\'requestFormTable\' width=100%></table>");

	//add the dataset select panel
	$("#requestFormTable").append("<tr><td colspan=3><h3 class=\'taskTitle\'>Dataset Options</h3></td></tr>");
	$("#requestFormTable").append("<tr id=\'datasetOptionsPanel\'></tr>");
    $("#datasetOptionsPanel").append("<td id=\'datasetPanel\' valign=\'top\'><b>Dataset</b><br>");
	$("#datasetPanel").append("<select name=\'dataset\' id=\'dataset\' onchange=\'listTasks()\'></select></td>");
	listDatasets();
    //add the feature selection panel
    $("#datasetOptionsPanel").append("<td id=\'featureSelectionsPanel\' valign=\'top\'><b>Feature Selection</b><br></td>");
    listFeatureSelections();
    //list data discretizers 
	$("#datasetOptionsPanel").append("<td id=\'dataDiscretizationsPanel\' valign=\'top\'><b>Data Discretization</b><br></td>");
	listDataDiscretizations();
	
	//add the subgrouping seletion panel
	$("#requestFormTable").append("<tr><td colspan=3><h3 class=\'taskTitle\'>Subgrouping Options</h3></td></tr>");
    $("#requestFormTable").append("<tr id=\'subgroupingOptionsPanel\'></tr>");
	//add the task panel
	$("#subgroupingOptionsPanel").append("<td id=\'tasksPanel\' valign=\'top\'><b>Task</b><br><select name=\'task\' id=\'task\' onchange=\'loadTaskName(); viewSubgroups()\'></select></td>");
	$("#tasksPanel").append("<br><br><b>Task name</b><br><div id=\'taskNamePanel\'></div>");
	$("#taskNamePanel").append("<input id=\'taskName\' name=\'taskName\' type=\'text\' size=30 value=\'\'>");
	//add the subgrouping panel	
    $("#subgroupingOptionsPanel").append("<td id=\'subgroupingPanel\' valign=\'top\'><b>Subgrouping Type</b><br></td>");
    listSubgroupings();
	$("#subgroupingOptionsPanel").append("<td id=\'subgroupingPlotPanel\'></td>");

    //add the classifier options panel
	$("#requestFormTable").append("<tr><td colspan=3><h3 class=\'taskTitle\'>Classifier Options</h3></td></tr>");
    $("#requestFormTable").append("<tr id=\'classifierOptionsPanel\'></tr>");
    //add the classifier panel
    $("#classifierOptionsPanel").append("<td id=\'classifiersPanel\' valign=\'top\'><b>Classifier</b><br><select name=\'classifier\' id=\'classifier\'></select></td>");
	listClassifiers();
    //add the eval method 
    $("#classifierOptionsPanel").append("<td colspan=2 id=\'crossValidationsPanel\' valign=\'top\'><b>Cross-validation Method</b><br></td>");
    listCrossValidations();

	//show the submit button
	$("#requestFormTable").append("<tr style=\'text-align: center\' id=\'formButtonsPanel\'></tr>");
	showFormButtons();
}

function listDatasets(){
    var dataVars        = new Object;
    dataVars.rq_mode    = "listDatasets";
    $.ajax({
        type: "POST",
        url: "/cgi-bin/hgClassificationsRequester",
        data: dataVars,
        dataType: "json",
        success: loadDatasets,
        cache: true
    });
}

function loadDatasets(data)
//Parse the data JSON object (a list of tasks and their stats) into a sortable table.
{
    // bind for segfault type errors
    $("#error").bind("ajaxError", function(){
        $(this).show();
    });
	
    $("#dataset").append("<option value=\'NA\'></option>");
	var datasetsArray = $(data).attr('datasets');
    $.each(datasetsArray, function(i){
		var dataset = $(this).attr('dataset');
		$("#dataset").append("<option value=\'"+dataset+"\'>"+dataset+"</option>");
    });
}

function listDataDiscretizations()
{
    $("#dataDiscretizationsPanel").append("<input type=\'radio\' name=\'dataDiscretization\' value=\'None\' checked=\'checked\'>None</input><br>");
	$("#dataDiscretizationsPanel").append("<input type=\'radio\' name=\'dataDiscretization\' value=\'sign\'>By sign</input><br>");
	$("#dataDiscretizationsPanel").append("<input type=\'radio\' name=\'dataDiscretization\' value=\'percent\'>Bin values into </input>");
    $("#dataDiscretizationsPanel").append("<input type=\'text\' name=\'discretizationRanks\' size=2 value=\'3\'> bins");
}

function listFeatureSelections()
{
    $("#featureSelectionsPanel").append("<input type=\'radio\' name=\'featureSelection\' value=\'None\'>None</input><br>");
	$("#featureSelectionsPanel").append("<input type=\'radio\' name=\'featureSelection\' value=\'filterByVarianceScore\'>Variance score \> ");
	$("#featureSelectionsPanel").append("<input type=\'text\' name=\'varianceScore\' id=\'varianceScore\' size=4 value=\'0.1\'><br>")
    $("#featureSelectionsPanel").append("<input type=\'radio\' name=\'featureSelection\' value=\'filterByVarianceRank\' checked=\'checked\'>Most variant features </input>");
	$("#featureSelectionsPanel").append("<input type=\'text\' name=\'varianceRank\' id=\'varianceRank\' size=4 value=\'200\'>");
}

function listTasks()
{
    if($("#dataset").val() != "NA"){
	    var dataVars        = new Object;
	    dataVars.rq_mode    = "listTasks";
		dataVars.rq_dataset = $("#dataset").val();
	    $.ajax({
	        type: "POST",
	        url: "/cgi-bin/hgClassificationsRequester",
	        data: dataVars,
	        dataType: "json",
	        success: loadTasks,
	        cache: true
	    });
	}
}

function loadTasks(data)
//Parse the data JSON object (a list of tasks and their stats) into a sortable table.
{
    // bind for segfault type errors
    $("#error").bind("ajaxError", function(){
        $(this).show();
    });

	$("#task").empty();
    $("#task").append("<option value=\'NA\'></option>");
    var tasksArray = $(data).attr('tasks');
    $.each(tasksArray, function(i){
        var label = $(this).attr('label');
		var task = $(this).attr('task');
        $("#task").append("<option value=\'"+task+"\'>"+label+"</option>");
    });
}

function listSubgroupings() {
	//None
    $("#subgroupingPanel").append("<input type=\'radio\' name=\'subgrouping\' value=\'None\' onchange=\'viewSubgroups()\'>None</input><br>");
	//median
	$("#subgroupingPanel").append("<input type=\'radio\' name=\'subgrouping\' value=\'median\' checked=\'checked\' onchange=\'viewSubgroups()\'>Median</input><br>");
	//quartiles
    $("#subgroupingPanel").append("<input type=\'radio\' name=\'subgrouping\' value=\'quartile\' onchange=\'viewSubgroups()\'>Top-and-bottom quartile</input><br>");
	//thresholds
    $("#subgroupingPanel").append("<input type=\'radio\' name=\'subgrouping\' value=\'thresholds\' onchange=\'viewSubgroups()\'>Thresholds: low&le;</input>");
	$("#subgroupingPanel").append("<input id=\'lowThreshold\' type=\'text\' name=\'lowThreshold\' value=\'365\' size=3 onchange=\'viewSubgroups()\'>, high>");
    $("#subgroupingPanel").append("<input id=\'hiThreshold\' type=\'text\' name=\'hiThreshold\' value=\'366\' size=3 onchange=\'viewSubgroups()\'><br>");
	//classes
	$("#subgroupingPanel").append("<input type=\'radio\' name=\'subgrouping\' value=\'classes\' onchange=\'viewSubgroups()\'>Classes: low=</input>");
    $("#subgroupingPanel").append("<input id=\'lowClass\' type=\'text\' name=\'lowClass\' value=\'-1\' size=3 onchange=\'viewSubgroups()\'>, high=");
    $("#subgroupingPanel").append("<input id=\'hiClass\' type=\'text\' name=\'hiClass\' value=\'1\' size=3 onchange=\'viewSubgroups()\'><br>");
	//Expressions
	$("#subgroupingPanel").append("<input type=\'radio\' name=\'subgrouping\' value=\'expressions\' onchange=\'viewSubgroups()\'>Expressions: ");
	$("#subgroupingPanel").append("low<select name=\'expr1op\' id=\'expr1op\' onchange=\'viewSubgroups()\'></select>");
	$("#expr1op").append("<option onchange=\'viewSubgroups()\' value=\'<\'>\<</option>");
	$("#expr1op").append("<option onchange=\'viewSubgroups()\' value=\'<=\'>&le</option>");
	$("#expr1op").append("<option onchange=\'viewSubgroups()\' value=\'==\'>=</option>");
	$("#expr1op").append("<option onchange=\'viewSubgroups()\' value=\'>=\'>&ge</option>");
	$("#expr1op").append("<option onchange=\'viewSubgroups()\' value=\'>\'>\></option>");
	$("#expr1op").append("<option onchange=\'viewSubgroups()\' value=\'!=\'>!=</option>");
	$("#subgroupingPanel").append("<input id=\'expr1val\' type=\'text\' name=\'expr1val\' value=\'1\' size=3 onchange=\'viewSubgroups()\'></input>");
	$("#subgroupingPanel").append(",high <select name =\'expr2op\' id=\'expr2op\' onchange=\'viewSubgroups()\'></select>");
    $("#expr2op").append("<option onchange=\'viewSubgroups()\' value=\'<\'>\<</option>");
    $("#expr2op").append("<option onchange=\'viewSubgroups()\' value=\'<=\'>&le</option>");
    $("#expr2op").append("<option onchange=\'viewSubgroups()\' value=\'==\'>=</option>");
    $("#expr2op").append("<option onchange=\'viewSubgroups()\' value=\'>=\'>&ge</option>");
    $("#expr2op").append("<option onchange=\'viewSubgroups()\' value=\'>\'>\></option>");
    $("#expr2op").append("<option onchange=\'viewSubgroups()\' value=\'!=\'>!=</option>");
    $("#subgroupingPanel").append("<input id=\'expr2val\' type=\'text\' name=\'expr2val\' value=\'1\' size=3></input onchange=\'viewSubgroups()\'><br>");
}

function listClassifiers() {
    $("#classifier").append("<option value=\'NA\'></option>");
    $("#classifier").append("<option value=\'NMFpredictor\tNA\'>NMFpredictor</option>");
    $("#classifier").append("<option value=\'SVMlight\tt=0\'>Linear kernel SVM</option>");
    $("#classifier").append("<option value=\'SVMlight\tt=1,d=1\'>First order polynomial SVM</option>");
    $("#classifier").append("<option value=\'SVMlight\tt=1,d=2\'>Second order polynomial SVM</option>");
	$("#classifier").append("<option value=\'glmnet\tLasso\'>Lasso</option>");
	$("#classifier").append("<option value=\'glmnet\tElasticNet\'>Elastic Net</option>");
	$("#classifier").append("<option value=\'glmnet\tRidgeRegression\'>Ridge Regression</option>");
}

function listCrossValidations() {
    $("#crossValidationsPanel").append("<input type=\'radio\' name=\'crossValidation\' value=\'k-fold\' checked=\'checked\'>k-fold, k= </input>");
	$("#crossValidationsPanel").append("<input type=\'text\' name\'kfolds\' id=\'kfolds\' size=1 value=\'5\'>");
    $("#crossValidationsPanel").append("x<input type=\'text\' name\'foldMultiplier\' id=\'foldMultiplier\' size=1 value=\'1\'><br>");
	$("#crossValidationsPanel").append("<input type=\'radio\' name=\'crossValidation\' value=\'loo\'>Leave-one-out</input>")
}

function showFormButtons() {
	$("#formButtonsPanel").append("<td><input id=\'resetButton\' type=\'button\' class=\'tableButton\' style=\'font-size:15px\' onclick=createEmptyForm() value=' Reset Form '></td>");
	$("#formButtonsPanel").append("<td>&nbsp</td>");
	$("#formButtonsPanel").append("<td><input id=\'submitButton\' type=\'button\' class=\'tableButton\' style=\'font-size:20px\' onclick=submitForm() value=' Submit Request '></td>");
}

function submitForm() {
    $("#error").bind("ajaxError", function(){
        $(this).hide();
    });

	var usrPredConfig = new Object();
	usrPredConfig.rq_name = $("#task").val()+"prediction_usrConfig";
    usrPredConfig.rq_profile="localDb";
    usrPredConfig.rq_db = "bioInt";
	usrPredConfig.rq_task = $("#taskName").val();
	usrPredConfig.rq_inputType = "bioInt";
	usrPredConfig.rq_dataFilepath = "";
	usrPredConfig.rq_metadataFilepath = "";
	usrPredConfig.rq_tableName= $("#dataset").val();
	usrPredConfig.rq_clinField= $("#task").val();

	var crossValidation = getCheckedValues("crossValidation");
	if(crossValidation == "k-fold"){
		usrPredConfig.rq_crossValidation = "k-fold";
        usrPredConfig.rq_folds = $("#kfolds").val();
        usrPredConfig.rq_foldMultiplier = $("#foldMultiplier").val();
	}else if(crossValidation == "loo"){
		usrPredConfig.rq_crossValidation = "loo";
        usrPredConfig.rq_folds = "1";
        usrPredConfig.rq_foldMultiplier = "1";
	}else{
		$("#error").show();
	}

	var classifier = $("#classifier").val().split("\t", 2);
	if(classifier[0] == "NMFpredictor"){
		usrPredConfig.rq_outputType="flatfiles";
		usrPredConfig.rq_classifier = "NMFpredictor";
    	usrPredConfig.rq_parameters = classifier[1];
	}else if(classifier[0] == "SVMlight"){
		usrPredConfig.rq_outputType="SVMlight";
		usrPredConfig.rq_classifier="SVMlight";
   		usrPredConfig.rq_parameters = classifier[1];
	}else if(classifier[0] == "glmnet"){
        usrPredConfig.rq_outputType="flatfiles";
        usrPredConfig.rq_classifier="glmnet";
        usrPredConfig.rq_parameters = classifier[1];
	}else{
		$("#error").show();
	}

    var dataDiscretizer = getCheckedValues("dataDiscretization");
	if(dataDiscretizer == "None"){
		usrPredConfig.rq_dataDiscretizer = dataDiscretizer;
    	usrPredConfig.rq_dataDiscretizerParameters = "NA";
	}else if(dataDiscretizer == "sign"){
		usrPredConfig.rq_dataDiscretizer = dataDiscretizer;
        usrPredConfig.rq_dataDiscretizerParameters = "NA";
	}else if(dataDiscretizer == "percent"){
        usrPredConfig.rq_dataDiscretizer = dataDiscretizer;
		percent = 100 / $("#discretizationRanks").val();
        usrPredConfig.rq_dataDiscretizerParameters = percent;
	}else{
		$("#error").show();
	}

    var featureSelection = getCheckedValues("featureSelection");
    if(featureSelection == "None"){
        usrPredConfig.rq_featureSelection = featureSelection;
        usrPredConfig.rq_featureSelectionParameters = "NA";
    }else if(featureSelection == "filterByVarianceScore"){
        usrPredConfig.rq_featureSelection = featureSelection;
        usrPredConfig.rq_featureSelectionParameters = $("#varianceScore").val();
    }else if(featureSelection == "filterByVarianceRank"){
        usrPredConfig.rq_featureSelection = featureSelection;
        usrPredConfig.rq_featureSelectionParameters = $("#varianceRank").val();
    }else{
        $("#error").show();
    }
	
    var clinDiscretizer = getCheckedValues("subgrouping");
	if(clinDiscretizer == "None"){
		usrPredConfig.rq_clinDiscretizer = clinDiscretizer;
		usrPredConfig.rq_clinDiscretizerParameters = "NA";
    }else if(clinDiscretizer == "median"){
        usrPredConfig.rq_clinDiscretizer = clinDiscretizer;
        usrPredConfig.rq_clinDiscretizerParameters = "NA";
    }else if(clinDiscretizer == "quartile"){
        usrPredConfig.rq_clinDiscretizer = clinDiscretizer;
        usrPredConfig.rq_clinDiscretizerParameters = "NA";
	}else if(clinDiscretizer == "thresholds"){
		usrPredConfig.rq_clinDiscretizer = clinDiscretizer;
		usrPredConfig.rq_clinDiscretizerParameters = $("#lowThreshold").val()+","+$("#hiThreshold").val();
    }else if(clinDiscretizer == "classes"){
        usrPredConfig.rq_clinDiscretizer = clinDiscretizer;
        usrPredConfig.rq_clinDiscretizerParameters = $("#lowClass").val()+","+$("#hiClass").val();
	}else if(clinDiscretizer == "expressions"){
		usrPredConfig.rq_clinDiscretizer = clinDiscretizer;
		usrPredConfig.rq_clinDiscretizerParameters = $("#expr1op").val()+$("#expr1val").val()+","+$("#expr2op").val()+$("#expr2val").val();
	}else{
		$("#error").show();
	}
	
	//reset the page
    createEmptyForm();
	
    $("#summaryPanel").dialog('open');
    $("#summaryPanel").dialog( "option", "width", 600);
    $("#summaryPanel").dialog( "option", "height", 300);
    $("#summaryPanel").dialog( "option", "title", "Submission Status");
    $("#summaryPanel").dialog( "option", "modal", true);
    $("#summaryPanel").empty();

    $(document).ready(function() {
        $("#summaryPanel").empty().html("<div id='loadingGif'><img src='css/images/ajax-loader.gif'></div>");
        $('html, body').animate({ scrollTop: $("#summaryPanel").offset().top - 40 , duration:'slow'});
    });

	//ajax submit the job to the db, and if it works open the window to review
    usrPredConfig.rq_mode = "submitRequest";
    $.ajax({
		type: "POST",
        url: "/cgi-bin/hgClassificationsRequester",
        data: usrPredConfig,
		dataType: "json",
		success: reviewRequest,
        cache: true
    });
}

function reviewRequest(data)
{
    $("#error").bind("ajaxError", function(){
        $(this).hide();
    });
	$("#loadingGif").remove();

	var requestId = $(data). attr('requestId');
	var timeEstimate = $(data).attr('estimatedRunTime');
	if(timeEstimate < 120){
		$("#summaryPanel").append("<p>Your jobs will take about "+timeEstimate+" seconds to complete. Would you like to submit?</p>");
		$("#summaryPanel").append("<div class=\'tableButton\' style=\'font-size:20px; float:left;\' onclick=\'submitRequest("+requestId+")\'>Yes</div>");
		$("#summaryPanel").append("<div style=\'float:left;\'>&nbsp</div>");
		$("#summaryPanel").append("<div class=\'tableButton\' style=\'font-size:20px; float:left;\'>No</div>");
	}else{
		$("#summaryPanel").append("<p>This job is too large to run without cluster support.</p>");
		$("#summaryPanel").append("<br><p>Your request has been logged, and may be run manually on our cluster computers at a later date.</p>");
		$("#summaryPanel").append("<br><p>Please check back at the prediction viewer in the near future for the results.</p>");
		$("#summaryPanel").append("<div class=\'tableButton\' style=\'font-size:20px; text-align:center;\' onclick=\'viewResults("+requestId+")\'>Back to Prediction Viewer</div>");
	}
}

function submitRequest(requestId)
{
    $(document).ready(function() {
        $("#summaryPanel").empty().html("<div id='loadingGif'><img src='css/images/ajax-loader.gif'></div>");
		$("#loadingGif").remove();
        $('html, body').animate({ scrollTop: $("#summaryPanel").offset().top - 40 , duration:'slow'});
  		timeOut = setInterval("viewSubmissionStatus("+requestId+")", 1000);
    });
	
	var dataVars = new Object;
    dataVars.rq_mode = "runRequest";
	dataVars.rq_requestId = requestId;
	$.post("/cgi-bin/hgClassificationsRequester", dataVars);
}

function viewSubmissionStatus(requestId)
{
    $("#error").bind("ajaxError", function(){
        $(this).hide();
    });
	
	var dataVars = new Object;
	dataVars.rq_mode = "viewSubmissionStatus";
	dataVars.rq_requestId = requestId;
	$.ajax({
        type: "POST",
        url: "/cgi-bin/hgClassificationsRequester",
        data: dataVars,
		dataType: "json",
		success: loadViewSubmissionStatus,
        cache: true
    });
}

function loadViewSubmissionStatus(data)
{
    $("#error").bind("ajaxError", function(){
        $(this).hide();
    });

    var requestId = $(data).attr('requestId');
    var jobStatus = $(data).attr('status');
    var elapsedTime = $(data).attr('elapsedTime');
	var estimatedRunTime = $(data).attr('estimatedRunTime');
    var percentComplete = roundVal(elapsedTime/estimatedRunTime) * 100;
    if(percentComplete >= 100){
        percentComplete = 100;
    }

	$("#summaryPanel").empty();
	$("#summaryPanel").append("<div id='progressBar' class='ui-progressbar ui-widget ui-widget-content ui-corner-all'></div>");
	$("#progressBar").progressbar({value: percentComplete});
	$("#summaryPanel").append("<div id=statusText></div>");
	
    $(document).ready(function() {
	    if(jobStatus == "Processed"){
	        clearInterval(timeOut);
  			$("#progressBar").progressbar({value: 100});
			$("#statusText").append("Job is finished. It took "+$(data).attr('elapsedTime')+" seconds<br><br>");
			$("#statusText").append("Your results are now available in the prediction viewer<br><br>");
			$("#statusText").append("<div class=\'tableButton\' style=\'font-size:20px; text-align:center;\' onclick=viewResults("+requestId+")>Back to Prediction Viewer</div>");
	    }else if(percentComplete >= 100){
        	$("#statusText").append("Status: "+jobStatus+"<br>Job has been running "+elapsedTime+" seconds<br>");
			$("#statusText").append("Estimated run time was "+estimatedRunTime+" seconds.<br>Job should finish shortly.");
	    }
    });
}


function viewResults(requestId){
    $(document).ready(function() {
		window.open("../hgClassifications-cszeto/")
	});
}

function viewTutorial()
{
    $("#summaryPanel").dialog('open');
    $("#summaryPanel").dialog( "option", "width", 900);
    $("#summaryPanel").dialog( "option", "height", 600);
    $("#summaryPanel").dialog( "option", "title", "Welcome!");
    $("#summaryPanel").dialog( "option", "modal", true);
    $("#summaryPanel").empty();

    $(document).ready(function() {
        $("#summaryPanel").append("<iframe src='tutorial.html' width='100%' height='100%'><p>Your browser does not support iframes</p></iframe>");
    });
}

function loadTaskName(){
	var currTask = $("#task").val();
	$("#taskNamePanel").empty().append("<input id=\'taskName\' name=\'taskName\' type=\'text\' size=30 value=\'"+currTask+"\'>");
}

function viewSubgroups()
{
var dataVars        = new Object;
dataVars.rq_mode    = "viewSubgroups";
dataVars.rq_dataset = $("#dataset").val();
dataVars.rq_task = $("#task").val();
var clinDiscretizer = getCheckedValues("subgrouping");
if(clinDiscretizer == "None"){
    dataVars.rq_clinDiscretizer = clinDiscretizer;
    dataVars.rq_clinDiscretizerParameters = "NA";
}else if(clinDiscretizer == "median"){
    dataVars.rq_clinDiscretizer = clinDiscretizer;
    dataVars.rq_clinDiscretizerParameters = "NA";
}else if(clinDiscretizer == "quartile"){
    dataVars.rq_clinDiscretizer = clinDiscretizer;
    dataVars.rq_clinDiscretizerParameters = "NA";
}else if(clinDiscretizer == "thresholds"){
    dataVars.rq_clinDiscretizer = clinDiscretizer;
    dataVars.rq_clinDiscretizerParameters = $("#lowThreshold").val()+","+$("#hiThreshold").val();
}else if(clinDiscretizer == "classes"){
    dataVars.rq_clinDiscretizer = clinDiscretizer;
    dataVars.rq_clinDiscretizerParameters = $("#lowClass").val()+","+$("#hiClass").val();
}else if(clinDiscretizer == "expressions"){
	dataVars.rq_clinDiscretizer = clinDiscretizer;
	//if the expression is ==, change it to mysql = operation here
	var expr1op = $("#expr1op").val();
	if(expr1op == "=="){
		expr1op = "=";
	}
	var expr2op = $("#expr2op").val();
	if(expr2op == "=="){
		expr2op = "=";
	}
	dataVars.rq_clinDiscretizerParameters = expr1op+$("#expr1val").val()+","+expr2op+$("#expr2val").val();
}else{
    $("#error").show();
}

$("#subgroupingPlotPanel").empty().html("<div id='loadingGif'><img src='css/images/ajax-loader.gif'></div>");
$.ajax({
	type: "POST",
	url: "/cgi-bin/hgClassificationsRequester",
	data: dataVars,
	dataType: "json",
	success: loadSubgroups,
	cache: true
	});
}

function loadSubgroups(data)
{
    // bind for segfault type errors
    $("#error").bind("ajaxError", function(){
        $(this).show();
    });

	var binsArray = $(data).attr('bins');
	var binNames = [];
	var binValues = [];
	var longestName = 0;
	$.each(binsArray, function(i){
		var binName = $(this).attr('name');
		if(binName.length > longestName){
			longestName = binName.length;
		}
		binNames.push(binName);
		binValues.push($(this).attr('value'));
	});

	$("#subgroupingPlotPanel").empty();
	var graphDivWidth=binValues.length*15+100;
	var graphDivHeight=longestName*6+200;
	$("#subgroupingPlotPanel").append("<div id=\"subgroupingPlotLeft\" style=\'width:"+graphDivWidth+"px; height:"+graphDivHeight+"px; float:left\'></div>");
    plot1 = $.jqplot('subgroupingPlotLeft', [binValues], {
	    title: 'Value Distribution',
	    seriesDefaults:{
	        renderer:$.jqplot.BarRenderer,
	        rendererOptions:{barPadding:0, barMargin:2, shadowAlpha:0}
	        },
	    axes: {
	        xaxis: {
	            label: 'Values',
	            renderer: $.jqplot.CategoryAxisRenderer,
	            ticks: binNames,
            	tickRenderer: $.jqplot.CanvasAxisTickRenderer,
            	tickOptions: {
               		angle: -70
           	 	}
	        },
	        yaxis: {
	            label: 'Counts',
				min: 0,
				tickOptions:{
					formatString: '%d'
				}
	        }
    	}
    });
	
	var subgroupCounts = [$(data).attr('subgroup1count'), $(data).attr('subgroup2count')];
	var subgroupNames = ['Low','High'];
	$("#subgroupingPlotPanel").append("<div id=\"subgroupingPlotRight\" style=\'width:200px; height:200px; float:left\'></div>");
    plot1 = $.jqplot('subgroupingPlotRight', [subgroupCounts], {
        title: 'Current subgroups',
        seriesDefaults:{
            renderer:$.jqplot.BarRenderer,
            rendererOptions:{barPadding:0, barMargin:2, shadowAlpha:0}
            },
        axes: {
            xaxis: {
                label: 'Subgroup',
                renderer: $.jqplot.CategoryAxisRenderer,
                ticks: subgroupNames
            },
            yaxis: {
                label: 'Counts',
				min:0,
				tickOptions:{
					formatString: '%d'
				}
            }
        }
    });


}

//HELPER FUNCTIONS
function getCheckedValues(objName)
    {   
        var arr = new Array();
        arr = document.getElementsByName(objName);
        for(var i = 0; i < arr.length; i++)
        {
            var obj = document.getElementsByName(objName).item(i);

            if(obj.checked)
            {
                return(obj.value);
            }
        }
    }

function roundVal(val){
    var dec = 2;
    var result = Math.round(val*Math.pow(10,dec))/Math.pow(10,dec);
    return result;
}

