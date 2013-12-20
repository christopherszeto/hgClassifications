//GLOBALS
var timeOut;
var totalEstimatedRunTime=-1;
var maxEstimatedRunTime=120;
var elapsedTime=0;
var thresholdFeatureUsage = 75;
var filterLowCoverageRows = 0;
var maxZ = 3;
var minZ = -3;
var hideSampleLabels = 0;
var hidePermutedSamples = 0;
var storedTable;
var permutationsTable;
var nonDisplayTable;
var nonDisplayPermsTable;
//variables for floating tasknames
var startX = 0;
var curX = startX;
var destX = curX;
//variables for floating samplenames
var startY = 0;
var curY = 0;
var destY = 0;
//slider controllers
var featureUsageSliderLocked = 0;
var userMinAccGainSliderLocked = 0;

//FUNCTIONS
function setUp()
{
    // bind for segfault type errors
    $("#error").bind("ajaxError", function(){
        $(this).hide();
    });
    $('#summaryPanel').dialog({ autoOpen: false });
	$("#mainPanel").empty();
    $("#progressBar").empty();
	
	//add buttons at top
	$("#mainPanel").append("<table><tr id=\'titleBarTable\'></tr></table>");
    $("#titleBarTable").append("<td><div style='padding: 10px'><img height=175px src='css/images/predictorPrintLogo.png'><br><b>Apply UCSC's <a href='../hgClassifications-cszeto'>TopModels</a> to your data</b></div></td>");
    $("#titleBarTable").append("<td><div class=\'tableButton\' style=\'width:150px; text-align:center;\' onclick=\'viewTutorial()\'><b>New here?</b><br>View a tutorial</div></td>");
    $("#titleBarTable").append("<td><div id=\'resetButton\' style=\'padding: 20px;\'><div class=\'tableButton\' style=\'width:150px; text-align:center;\' onclick=\'setUp()\'><b> Reset and resubmit </b></div></div></td>");
	$("#resetButton").hide();

	//HERE get rid of floating labels
	$("#mainPanel").append("<div id=\'heatmapDiv\'></div>");
	$("#mainPanel").append("<div id=\'optionButtonsDiv\' style=\'position: absolute; top:1px;\'><div>");
	loadOptionButtons();
	$("#optionButtonsDiv").hide();
		
    var dataVars        = new Object;
    dataVars.pp_mode    = "getDataTypeList";
    $.ajax({
        type: "POST",
        url: "/cgi-bin/hgPredictorPrint",
        data: dataVars,
        dataType: "json",
        success: showDataSubmissionForm,
        cache: true
    });
}

function viewTutorial(){
    $("#summaryPanel").dialog('open');
    $("#summaryPanel").dialog( "option", "width", 900);
    $("#summaryPanel").dialog( "option", "height", 600);
    $("#summaryPanel").dialog( "option", "title", "Welcome!");
    $("#summaryPanel").dialog( "option", "modal", true);
    $("#summaryPanel" ).bind( "dialogbeforeclose", function(event, ui) {
        $("#summaryGraph").remove();
    });
    $("#summaryPanel").empty();

    $("#summaryPanel").load("tutorial.htm").hide();

	$("#resetButton").show();
    $(document).ready(function() {
        $("#summaryPanel").show();
    });
}

function loadOptionButtons(){
    $("#optionButtonsDiv").append("<table><tr id=\'optionButtonsTable\'></tr></table>");
	$("#optionButtonsTable").append("<td><div style=\'padding: 20px;\'><div class=\'tableButton\' style=\'width:150px; text-align:center;\' onclick=\'viewDisplayConfiguration()\'><b> Configure Display </b></div></div></td>");    
	$("#optionButtonsTable").append("<td><div style=\'padding: 20px;\'><div class=\'tableButton\' style=\'width:150px; text-align:center;\' onclick=\'viewClusteringConfiguration()\'><b> Clustering Options </b></div></div></td>");
	$("#resetButton").show();
}

function showDataSubmissionForm(data)
//show the user the form they can submit their data to, and display the esitmated run time
{
    // bind for segfault type errors
    $("#error").bind("ajaxError", function(){
        $(this).show();
    });
	var dataTypesArray = $(data).attr('dataTypes');
    $("#summaryPanel").empty();
    $("#summaryPanel").dialog('open');
    $("#summaryPanel").dialog( "option", "width", 500);
    $("#summaryPanel").dialog( "option", "height", 400);
    $("#summaryPanel").dialog( "option", "title", "Apply Top Models");
    $("#summaryPanel").dialog( "option", "modal", true);
	$("#summaryPanel" ).bind( "dialogbeforeclose", function(event, ui) {
		$("#dataSubmissionForm").remove();
	});
    $("#summaryPanel").append("<form id=\'dataSubmissionForm\' action=\'/cgi-bin/hgPredictorPrint\' method=\'POST\' enctype=\'multipart/form-data\' encoding=\'multipart/form-data\'></form>");
    $("#dataSubmissionForm").append("<input type=\'hidden\' name=\'MAX_FILE_SIZE\' value=\'500000\'>");
    $("#dataSubmissionForm").append("<input type=\'hidden\' name=\'pp_mode\' value=\'applyTopModels\'>");
    $("#dataSubmissionForm").append("<label>Please select your data file <br>[500KB max, tab separated labeled file of genes by samples]:<br><br></label>");
    $("#dataSubmissionForm").append("<input type=\'file\' name=\'pp_userData\'><br><br>");
 	$("#dataSubmissionForm").append("<label>Or provide a URL to your data file: </label><br>");
	$("#dataSubmissionForm").append("<input type=\'text\' name=\'pp_userDataURL\' style=\'border-style:solid; border-color:#999999; border-width:1px\' size=40><br><br>");
	$("#dataSubmissionForm").append("<label>Select your data type:</label><br>");
 	$.each(dataTypesArray, function(i){
        var dataType = $(this).attr('name');
		var dataTypeId = $(this).attr('id');
		$("#dataSubmissionForm").append("<input type=\'radio\' name=\'pp_userDataTypeId\' value=\'"+dataTypeId+"\' onclick=loadEstimatedTotalRunTime("+dataTypeId+",0)>&nbsp"+dataType+"<br>");
	});
	$("#dataSubmissionForm").append("<br><label>[Optional] Set minimum accuracy gain:</label><br>");
	$("#dataSubmissionForm").append("<input id='userMinGainVal' type=\'text\' name=\'pp_userMinAccGain\' value=\'0\' style=\' font-size:small\'><br>");
	$("#dataSubmissionForm").append("<div id=\'userMinAccGainSlider\'></div>");
	$("#userMinAccGainSlider").slider({
		range: "min",
     	value: 0,
     	min: 0,
     	max: 50,
		slide: function( event, ui ) {
			userMinAccGainSliderLocked = 1;
			setTimeout( 'userMinAccGainSliderLocked=0', 30);
			setTimeout(function(){
				if(!userMinAccGainSliderLocked){
            		var dataTypeId = $("input[@name=\"pp_userDataTypeId\"]:checked").val();
            		var minAccGain = $("#userMinAccGainSlider").slider("option", "value");
					$("#userMinGainVal").val(  $("#userMinAccGainSlider").slider("option", "value"));
					loadEstimatedTotalRunTime(dataTypeId, minAccGain);
				}
			},300);
      	}
	});
	$("#dataSubmissionForm").append("<label id=\'timeEstimateLabel\'></label><br><br>");
	$("#timeEstimateLabel").hide();
    $("#dataSubmissionForm").append("<input id=\'submitButton\' type=\'submit\' class=\'tableButton\' value=\' Submit \'><br><br>");
	$("#submitButton").hide();

    $(document).ready(function() {
        var options = {
            target: '#mainPanel',
			beforeSubmit: dummyDataChecker, //this fixes submit function returning error status for some reason
        	type: "POST",
            clearForm: true,
            resetForm: true,
            dataType: 'json',
            success: storeAppliedTopModels,
			error: function(){ alert("Something has broken!! Sorry. We'll be back soon."); }
        };
        $('#dataSubmissionForm').submit(function(){
            updateTimer(estimatedTotalRunTime);
            $('html, body').animate({ scrollTop: $("#summaryPanel").offset().top - 40 , duration:'slow'});
            $(this).ajaxSubmit(options);
            return false;
        });
    });
}

function loadEstimatedTotalRunTime(dataTypeId, minAccGain){
	if(dataTypeId != undefined && minAccGain != undefined){
		dataVars         		= new Object;
	    dataVars.pp_mode        = "getEstimatedTotalRunTime";
	    dataVars.pp_userDataTypeId  = dataTypeId;
	    dataVars.pp_userMinAccGain = minAccGain;
	    $.ajax({
   	    	type: "POST",
        	url: "/cgi-bin/hgPredictorPrint",
        	data: dataVars,
        	dataType: "json",
        	success: updateTimeEstimate,
        	cache: true
   		});
	}
}

function updateTimeEstimate(data){
    var total = $(data).attr('estimatedRunTime');
	estimatedTotalRunTime=total
	if(total >= 0){
		var estMinutes = Math.floor(total/60);
		var estSeconds = Math.round(total%60);
		$("#timeEstimateLabel").empty();
		$("#timeEstimateLabel").append("<br>Processing is estimated to take "+estMinutes+"m:"+estSeconds+"s");
		if(estimatedTotalRunTime > maxEstimatedRunTime){
			$("#timeEstimateLabel").append("<br><div class='warning'>WARNING: This processing time is longer than most web browsers will wait. If you encounter errors please try adjusting the minimum accuracy filter up.</div>");
		}
		$("#submitButton").show();
	}else{
		$("#timeEstimateLabel").empty();
		$("#timeEstimateLabel").append("No topmodels match this datatype.\n");
		$("#submitButton").hide();
	}
	$("#timeEstimateLabel").show();
	userMinAccGainSliderLocked = 0;
}

/*Not sure why, but this function makes submssion by URL work again*/
function dummyDataChecker(formData, jqForm, options){
	var dataURL = jqForm.attr('pp_userDataURL');
	if(dataURL != undefined){
		;
	}else{
		alert("No data URL provided.");
	}
}	

function updateTimer(totalTime){
    $("#error").bind("ajaxError", function(){
        $(this).hide();
    });
	var percentComplete = roundVal(elapsedTime / totalTime)*100;
	if(percentComplete > 100){
		percentComplete = 100;
	}
	$("#summaryPanel").empty();
	$("#summaryPanel").dialog('close');
	$("#heatmapDiv").empty();
	$("#heatmapDiv").append("<br><br><p>Applying models to your data. Please wait...</p>");
	$("#heatmapDiv").append("<div id='progressBar' class='ui-progressbar ui-widget ui-widget-content ui-corner-all'></div>");
	$("#progressBar").progressbar({value: percentComplete});	

	if(percentComplete ==0){
		timeOut = setInterval("updateTimer("+totalTime+")", 1000);
		elapsedTime +=1;
	}else if(percentComplete == 100){
		elapsedTime=0;
		clearInterval(timeOut);
		$("#heatmapDiv").append("<p>Your results will display shortly..</p>");
	}else{
		elapsedTime +=1;
	}
	return false;
}

function storeAppliedTopModels(data){
	elapsedTime = 0;
	$("#progressBar").progressbar({value: 0});
	clearInterval(timeOut);
	var tasksArray = $(data).attr('tasks');
	//make a space to store table
	storedTable = new Object();
	storedTable.taskNames = new Array();
	storedTable.sampleNames = new Array();
	storedTable.jobIds = new Array();
	storedTable.featureUsages = new Array();
	storedTable.zScores = new Array();
    //make a similar space to store non-shown values
    nonDisplayTable = new Object();
    nonDisplayTable.taskNames = new Array();
    nonDisplayTable.jobIds = new Array();
    nonDisplayTable.featureUsages = new Array();
    nonDisplayTable.sampleNames = new Array();
    nonDisplayTable.zScores = new Array();

	//Make a space to store permuted versions
	permutationsTable = new Object();
    permutationsTable.zScores = new Array();
	nonDisplayPermsTable = new Object();
	nonDisplayPermsTable.zScores = new Array();

	//iterate, saving to various tables
	$.each(tasksArray, function(i){
		storedTable.taskNames[i] = $(this).attr('taskName');
		storedTable.jobIds[i] = $(this).attr('jobId');
		storedTable.featureUsages[i] = $(this).attr('featureUsage');
		storedTable.zScores[i] = new Array();
		permutationsTable.zScores[i] = new Array();
		zScores = $(this).attr('zScores');
		var pCount = 0;
		$.each(zScores, function(j){
			var thisSampleName = $(this).attr('sampleName');
			var thisZscore = $(this).attr('zScore');
			if(!thisSampleName.match(/^PermutedSample/)){
				storedTable.zScores[i][j] = thisZscore;
				if(i == 0){
					storedTable.sampleNames[j] = thisSampleName;	
				}
				j++
			}else{
				permutationsTable.zScores[i][pCount++] = thisZscore;
			}
		});
		i++;
	});
	viewAppliedTopModels();
}

function floatLabels()
{
  var pX = document.body.scrollLeft;
  destX = pX + startX;
  if(pX < ($("#taskNamesDiv").width()/2)){
		destX = startX;
  }
  var el = document.getElementById("taskNamesDiv");
  if(el != undefined){
		el.style.left = destX+"px";
		
		var pY = parseInt(document.body.scrollTop);
		if(pY <= startY){
			destY = startY;
		}else{
			destY = (pY+1);
		}
		el = document.getElementById("sampleNamesDiv");
		el.style.top = destY+"px";
	}
}

function viewAppliedTopModels(){
	$("#heatmapDiv").empty();
	//set cell parameters used to size everything
    var maxChars  = 11;
	var cellHeight="1.5em";
	var miniCellWidth=30;
	var fullCellWidth=120;

	//make the floating task names panel
    $("#heatmapDiv").append("<div id=\'taskNamesDiv\' style=\'position: absolute; left:"+startX+"px; z-index:2; background: transparent url(css/images/semiTransBg.gif);\'></div>");
	$("#taskNamesDiv").append("<table id=\'taskNamesTable\'></table>")
    $("#taskNamesTable").append("<tr><td colspan=3 nowrap=\'nowrap\' style=\'height:"+cellHeight+"; text-align:center;\'>Click to sort</td></tr>");
    var row= 0;
	for(i = 0; i < storedTable.zScores.length; i++){
        var taskName = storedTable.taskNames[i];
        var jobId = storedTable.jobIds[i];
        var featureUsage = storedTable.featureUsages[i];
        if((featureUsage > thresholdFeatureUsage) || !filterLowCoverageRows){            //add label to row names table
            $("#taskNamesTable").append("<tr id=\'taskNamesTable_"+row+"\'></tr>");
            $("#taskNamesTable_"+row).append("<td><div class=\'ui-state-default ui-corner-all\' onclick=\'removeRow("+i+")\' style=\'cursor: pointer\'><div class=\"ui-icon ui-icon-closethick\"></div></td>");
            $("#taskNamesTable_"+row).append("<td style=\'height:"+cellHeight+";\' nowrap=\'nowrap\'><div class=\'tableButton\' style=\'text-align:center; height:"+(cellHeight-8)+"px;\' onclick=\'sortByTask("+i+")\'> "+taskName+" </div></td>");
            $("#taskNamesTable_"+row).append("<td><div style=\'text-align:center; cursor: pointer;\' onclick=\'loadTopModelDetails("+jobId+")\'><img src=\'css/images/question_mark_icon.gif\'></div></td>");
			row++;
		}
	}

	//make the floating sample names panel
    startY = parseInt($("#taskNamesDiv").offset().top);
	curY = startY;
	destY = curY;
	var heatmapLeftEdge =  $("#taskNamesDiv").width()+5;
	$("#heatmapDiv").append("<div id=\'sampleNamesDiv\' style=\'position: absolute; left:"+heatmapLeftEdge+"px; top:"+startY+"px; z-index:1; background: transparent url(css/images/semiTransBg.gif);\'></div>");
	$("#sampleNamesDiv").append("<table id=\'sampleNamesTable\'><tr id=\'sampleNamesRow\'></tr></table>");
	if(!hidePermutedSamples){
		if(!hideSampleLabels){
			$("#sampleNamesRow").append("<td nowrap=\'nowrap\' colspan="+permutationsTable.zScores[0].length+" style=\'text-align:center; height:"+cellHeight+"; width:"+fullCellWidth+"px;\'>Permutations</td>");
		}else{
			$("#sampleNamesRow").append("<td nowrap=\'nowrap\' colspan="+permutationsTable.zScores[0].length+" style=\'width:"+(3*miniCellWidth)+"px; height:"+cellHeight+"\'> &nbsp </td>");
		}
	}
	for(i=0; i < storedTable.sampleNames.length;i++){
		if(!hideSampleLabels){
			//make sure labels aren't longer than max char, so they align properly
			if(storedTable.sampleNames[i].length < maxChars){
				$("#sampleNamesRow").append("<td nowrap=\'nowrap\' onclick=\'sortBySample("+i+")\' class=\'tableButton\' style=\'text-align:center; height:"+cellHeight+"; width:"+fullCellWidth+"px;\'>"+storedTable.sampleNames[i]+"</td>");
			}else{
				$("#sampleNamesRow").append("<td nowrap=\'nowrap\' onclick=\'sortBySample("+i+")\' class=\'tableButton\' style=\'text-align:center; height:"+cellHeight+"; width:"+fullCellWidth+"px;\'>"+storedTable.sampleNames[i].substring(0,(maxChars-3))+"...</td>");
			}
		}else{
			$("#sampleNamesRow").append("<td nowrap=\'nowrap\' style=\'text-align:center;height:"+cellHeight+"; width:"+miniCellWidth+"px;\'><div class=\'tableButton\' onclick=\'sortBySample("+i+")\' width="+(miniCellWidth-8)+"px><img src=\'css/images/icon_descending.png\'></div></td>");
		}
	}
	//tell the labels to scroll when the window does
 	window.onscroll = floatLabels;
	
	//make the heatmap panel
    var heatmapTopEdge = $("#taskNamesTable_0").offset().top;
	$("#heatmapDiv").append("<div id=\'heatmapCellsDiv\' style=\'float:left; position: absolute; left:"+heatmapLeftEdge+"px; top:"+heatmapTopEdge+"px\'></div>");
	$("#heatmapCellsDiv").append("<table id=\'resultsHeatmap\'></table>");
	row= 0;
	var col=0;
	for(i = 0; i < storedTable.zScores.length; i++){
		var featureUsage = storedTable.featureUsages[i];
        if((featureUsage > thresholdFeatureUsage) || !filterLowCoverageRows){
			//add row in heatmap for this data
			$("#resultsHeatmap").append("<tr id=\'row"+row+"\'></tr>");
			var j = 0;
			col = 0;
			//put in permuations if user has them on
			if(!hidePermutedSamples){
				for(j = 0; j < permutationsTable.zScores[i].length; j++){
					var zScore = permutationsTable.zScores[i][j];
					var permChar = "<font color=\'white\'>&#x2713</font>";
					if(zScore < minZ || zScore > maxZ){
						permChar = "<font color=\'black\'>&#x2717</font>";
					}
	                var color;
	                if(featureUsage > thresholdFeatureUsage){
	                    color = zScoreToHexColor(zScore);
	                }else{
	                    color = "#999999";
	                }
	                if(hideSampleLabels){
	                    $("#row"+row).append("<td nowrap=\'nowrap\' id=\'cell"+row+"_"+col+"\' bgcolor="+color+" style=\'text-align: center; height:"+cellHeight+"; width:"+miniCellWidth+"px;\' title=\'Permutation"+(j+1)+":"+zScore+"\'>"+permChar+"</td>");
	                }else{
	                    $("#row"+row).append("<td nowrap=\'nowrap\' id=\'cell"+row+"_"+col+"\' bgcolor="+color+" style=\'text-align: center; height:"+cellHeight+"; width:"+(fullCellWidth/permutationsTable.zScores[i].length)+"px;\' title=\'"+zScore+"\'>"+permChar+"</td>");
	                }
	                $("#cell"+row+"_"+col).tooltip({ predelay: 200, opacity: 0.8});
	                col++;
				}
			}
			//Put non-permutation values in to table
			for(j = 0; j < storedTable.zScores[i].length; j++){
				var zScore = storedTable.zScores[i][j];
				var color;
				if(featureUsage > thresholdFeatureUsage){
					color = zScoreToHexColor(zScore);
				}else{
					color = "#999999";
				}
				if(hideSampleLabels){
					$("#row"+row).append("<td nowrap=\'nowrap\' class=\'heatmapCell\' id=\'cell"+row+"_"+col+"\' bgcolor="+color+" title=\'"+storedTable.sampleNames[j]+":"+zScore+"\' style=\' height:"+cellHeight+"; width:"+miniCellWidth+"px;\'>&nbsp</td>");
				}else{
					$("#row"+row).append("<td nowrap=\'nowrap\' class=\'heatmapCell\' id=\'cell"+row+"_"+col+"\' bgcolor="+color+" title=\'"+zScore+"\' style=\'height:"+cellHeight+"; width:"+fullCellWidth+"px\'>&nbsp</td>");
				}
				$("#cell"+row+"_"+col).tooltip({ predelay: 200, opacity: 0.8});
				col++;
			}
		row++;
		}
	}
	//make a selector for tasks to put back in after removal
	$("#heatmapDiv").append("<div id=\'replaceRowsSelectorDiv\' style=\'position:absolute; top:"+($("#taskNamesDiv").offset().top + $("#taskNamesDiv").height() + 10)+"px;\'></div>");
	if(nonDisplayTable.taskNames.length){
		$("#replaceRowsSelectorDiv").append("<label>Select rows to replace:</label><br>");
		$("#replaceRowsSelectorDiv").append("<select id=\'replaceRowSelector\' name=\'replaceRowSelector\'></select>");
		$("#replaceRowSelector").append("<option value=\'-1\'></option>");
		for(i = 0; i < nonDisplayTable.taskNames.length; i++){
			$("#replaceRowSelector").append("<option value=\""+i+"\">"+nonDisplayTable.taskNames[i]+"</option>");
		}
		$("#replaceRowSelector").change( function(){
			replaceRow($("#replaceRowSelector").val());
		});
	}
	//put the options buttons in
	var optionButtonsTopEdge = $("#replaceRowsSelectorDiv").offset().top + $("#replaceRowsSelectorDiv").height() + 10;
	var optionButtonEl = document.getElementById("optionButtonsDiv");
	optionButtonEl.style.top = optionButtonsTopEdge+"px";
    $("#optionButtonsDiv").show();
}

function viewClusteringConfiguration(){
    $("#summaryPanel").dialog('open');
    $("#summaryPanel").dialog( "option", "width", 700);
    $("#summaryPanel").dialog( "option", "height", 250);
    $("#summaryPanel").dialog( "option", "title", "Clustering options");
    $("#summaryPanel").dialog( "option", "modal", true);
    $("#summaryPanel" ).bind( "dialogbeforeclose", function(event, ui) {
        $("#summaryPanel").empty();
    });
	$("#summaryPanel").append("<label>Linkage method: </label>");
	$("#summaryPanel").append("<input type='radio' name='linkageMethodRadioButtons' value=\"0\"> Single-linkage </input>");
    $("#summaryPanel").append("<input type='radio' name='linkageMethodRadioButtons' value=\"1\"> Complete-linkage </input>");
    $("#summaryPanel").append("<input type='radio' name='linkageMethodRadioButtons' value=\"2\" checked=\"checked\"> Average-linkage </input>");
	$("#summaryPanel").append("<br><br>");
    $("#summaryPanel").append("<label>Distance: </label>");
    $("#summaryPanel").append("<input type='radio' name='distanceMethodRadioButtons' value=\"0\" checked=\"checked\"> Euclidian </input>");
    $("#summaryPanel").append("<input type='radio' name='distanceMethodRadioButtons' value=\"1\"> Manhattan </input>");
    $("#summaryPanel").append("<input type='radio' name='distanceMethodRadioButtons' value=\"2\"> Maximum </input>");
    $("#summaryPanel").append("<br><br>");
    $("#summaryPanel").append("<label>Clustering direction: </label>");
    $("#summaryPanel").append("<input type='radio' name='clusteringDirectionRadioButtons' value=\"0\"> Rows </input>");
    $("#summaryPanel").append("<input type='radio' name='clusteringDirectionRadioButtons' value=\"1\"> Columns </input>");
    $("#summaryPanel").append("<input type='radio' name='clusteringDirectionRadioButtons' value=\"2\" checked=\"checked\"> Both </input>");
    $("#summaryPanel").append("<br><br>");
	
	$(document).ready(function(){
		$("#summaryPanel").append("<div id='runClusteringButton' class='tableButton' style='text-align: center; width: 150px;'><b> Cluster! </b></div>");
		$("#runClusteringButton").click(function(){
		    var linkage = $("input:radio[name=linkageMethodRadioButtons]:checked").val();
    		var distance = $("input:radio[name=distanceMethodRadioButtons]:checked").val();
    		var clusteringDirection = $("input:radio[name=clusteringDirectionRadioButtons]:checked").val();
			$("#summaryPanel").empty().html("<div id='loadingGif'><img src='css/images/ajax-loader.gif'></div>");
			runClustering(linkage,distance, clusteringDirection);
		});
	});
}

function runClustering(linkage,distance, clusteringDirection){
	//make space to save a filtered copy of stored data, and a transpose of it
	var zScoresCopy = new Array(storedTable.zScores.length);
	var t_zScoresCopy = new Array(storedTable.zScores[0].length);
	var i;
	var j;
	for(i = 0; i < storedTable.zScores.length; i++){
		zScoresCopy[i] = new Array(storedTable.zScores[0].length);
	}
	for(i = 0; i < storedTable.zScores[0].length; i++){
		t_zScoresCopy[i] = new Array(storedTable.zScores.length);
	}
	//copy the data into the local copys
	for(i = 0; i < storedTable.zScores.length; i++){
		for(j = 0; j < storedTable.zScores[i].length; j++){
			if(storedTable.featureUsages[i] > thresholdFeatureUsage){
				zScoresCopy[i][j] = storedTable.zScores[i][j];
				t_zScoresCopy[j][i] = storedTable.zScores[i][j];
			}else{
				zScoresCopy[i][j] = 0;
				t_zScoresCopy[j][i] = 0;
			}
		}
	}
	if(clusteringDirection == 0 || clusteringDirection == 2){
		//find task order
		var root = figue.agglomerate(storedTable.taskNames, zScoresCopy, distance, linkage);
		var labelOrder = new Array();
		dfs(root, labelOrder);
   		reorderStoredTable(labelOrder, "rows");
	}
	if(clusteringDirection == 1 ||  clusteringDirection == 2){
		//find sample order
	    var root = figue.agglomerate(storedTable.sampleNames, t_zScoresCopy, distance, linkage);
	    var labelOrder = new Array();
	    dfs(root, labelOrder);
		reorderStoredTable(labelOrder, "columns");
	}
	//redraw
	viewAppliedTopModels();
	
	$("#summaryPanel").empty().dialog("close");
}

function removeRow(taskIx){
	var i, j;
	var nextIx = nonDisplayTable.taskNames.length;
	nonDisplayTable.taskNames[nextIx] = storedTable.taskNames[taskIx];
	storedTable.taskNames.splice(taskIx,1);
	nonDisplayTable.featureUsages[nextIx] = storedTable.featureUsages[taskIx];
    storedTable.featureUsages.splice(taskIx,1);
	nonDisplayTable.jobIds[nextIx] = storedTable.jobIds[taskIx];
	storedTable.jobIds.splice(taskIx,1);
	nonDisplayTable.zScores[nextIx] = new Array();
	nonDisplayPermsTable.zScores[nextIx] = new Array();
	//store permutations columns
    for(j = 0; j < permutationsTable.zScores[0].length; j++){
        nonDisplayPermsTable.zScores[nextIx][j] = permutationsTable.zScores[taskIx][j];
    }
	//if you have sampleNames already, make sure to save zScores to the right samples
	if(nonDisplayTable.sampleNames.length){
		for(i = 0; i < storedTable.sampleNames.length; i++){
			for(j = 0; j < nonDisplayTable.sampleNames.length; j++){
				if(nonDisplayTable.sampleNames[j] == storedTable.sampleNames[i]){
					break;
				}
			}
			nonDisplayTable.zScores[nextIx][j] = storedTable.zScores[taskIx][i];
		}
	}else{ //otherwise, save sampleNames and zScores.
		for(i = 0; i < storedTable.sampleNames.length; i++){
			nonDisplayTable.zScores[nextIx][i] = storedTable.zScores[taskIx][i];
			nonDisplayTable.sampleNames[i] = storedTable.sampleNames[i];
		}
	}
	storedTable.zScores.splice(taskIx,1);
	permutationsTable.zScores.splice(taskIx,1);
	viewAppliedTopModels();
}

function replaceRow(taskIx){
	var i, j;
	var nextIx = storedTable.taskNames.length;
    storedTable.taskNames[nextIx] = nonDisplayTable.taskNames[taskIx];
    nonDisplayTable.taskNames.splice(taskIx,1);
    storedTable.featureUsages[nextIx] = nonDisplayTable.featureUsages[taskIx];
    nonDisplayTable.featureUsages.splice(taskIx,1);
    storedTable.jobIds[nextIx] = nonDisplayTable.jobIds[taskIx];
    nonDisplayTable.jobIds.splice(taskIx,1);
    storedTable.zScores[nextIx] = new Array();
	permutationsTable.zScores[nextIx] = new Array();
	for(j = 0; j < nonDisplayPermsTable.zScores[0].length; j++){
		permutationsTable.zScores[nextIx][j] = nonDisplayPermsTable.zScores[taskIx][j];
	}
    for(i = 0; i < nonDisplayTable.sampleNames.length; i++){
		var j = 0;
		for(j = 0; j < storedTable.sampleNames.length; j++){
			if(storedTable.sampleNames[j] == nonDisplayTable.sampleNames[i]){
				break;
			}
		}
        storedTable.zScores[nextIx][j] = nonDisplayTable.zScores[taskIx][i];
    }
    nonDisplayTable.zScores.splice(taskIx,1);
	nonDisplayPermsTable.zScores.splice(taskIx, 1);
    viewAppliedTopModels();
}


function sortByTask(taskIx){
    //make a copy of the pertinent data
    var i;
    var dataCopy = new Array();
    for(i = 0; i < storedTable.sampleNames.length; i++){
        dataCopy[i] = storedTable.zScores[taskIx][i];
    }
    //iterate through the list, finding the order of the top-to-bottom values
    var labelOrder = new Array();
    for(i = 0; i < dataCopy.length; i++){
        maxIx = i;
        for(j = 0; j < dataCopy.length; j++){
            if(dataCopy[j] > dataCopy[maxIx]){
                maxIx = j;
            }
        }
        dataCopy[maxIx] = -999999;
        labelOrder[i] = storedTable.sampleNames[maxIx];
    }
    reorderStoredTable(labelOrder, "columns");
    viewAppliedTopModels();
}

function sortBySample(sampleIx){
	//make a copy of the pertinent data
	var i;
	var dataCopy = new Array();
	for(i = 0; i < storedTable.taskNames.length; i++){
		dataCopy[i] = storedTable.zScores[i][sampleIx];
	}
	//iterate through the list, finding the order of the top-to-bottom values
	var labelOrder = new Array();
	for(i = 0; i < dataCopy.length; i++){
		maxIx = i;
		for(j = 0; j < dataCopy.length; j++){
			if(dataCopy[j] > dataCopy[maxIx]){
				maxIx = j;
			}
		}
		dataCopy[maxIx] = -999999;
		labelOrder[i] = storedTable.taskNames[maxIx];
	}
	reorderStoredTable(labelOrder, "rows");
	viewAppliedTopModels();
}

function reorderStoredTable(labelOrder, direction){
	var labelIx = 0;
	if(direction == "rows"){
	    for(labelIx = 0; labelIx < labelOrder.length; labelIx++){
	        for(i = 0; i < storedTable.taskNames.length; i++){
	            if(labelOrder[labelIx] == storedTable.taskNames[i]){
	                var tmp = storedTable.taskNames[labelIx];
	                storedTable.taskNames[labelIx] = storedTable.taskNames[i];
	                storedTable.taskNames[i] = tmp;
	                var tmp = storedTable.jobIds[labelIx];
	                storedTable.jobIds[labelIx] = storedTable.jobIds[i];
	                storedTable.jobIds[i] = tmp;
	                var tmp = storedTable.featureUsages[labelIx];
	                storedTable.featureUsages[labelIx] = storedTable.featureUsages[i];
	                storedTable.featureUsages[i] = tmp;
	                for(j = 0; j < storedTable.zScores[labelIx].length; j++){
	                    var tmp = storedTable.zScores[i][j];
	                    storedTable.zScores[i][j] = storedTable.zScores[labelIx][j];
	                    storedTable.zScores[labelIx][j] = tmp;
					}
					for(j = 0; j < permutationsTable.zScores[labelIx].length; j++){
						var tmp = permutationsTable.zScores[i][j];
                        permutationsTable.zScores[i][j] = permutationsTable.zScores[labelIx][j];
                        permutationsTable.zScores[labelIx][j] = tmp;
	                }
	                break;
	            }
	        }
	    }
	}else if(direction == "columns"){
	    for(labelIx = 0; labelIx < labelOrder.length; labelIx++){
	        for(j = 0; j < storedTable.sampleNames.length; j++){
	            if(labelOrder[labelIx] == storedTable.sampleNames[j]){
	                var tmp = storedTable.sampleNames[labelIx];
	                storedTable.sampleNames[labelIx] = storedTable.sampleNames[j];
	                storedTable.sampleNames[j] = tmp;
	                for(i = 0; i < storedTable.zScores.length; i++){
	                    var tmp = storedTable.zScores[i][j];
	                    storedTable.zScores[i][j] = storedTable.zScores[i][labelIx];
	                    storedTable.zScores[i][labelIx] = tmp;
	                }
	                break;
	            }
	        }
	    }
	}
}

function dfs(tree, labels){
	if(tree.size == 1){
		labels[labels.length] = tree.label;
		return;
	}
	dfs(tree.left, labels);
	dfs(tree.right, labels);
}	

function viewDisplayConfiguration(){
    $("#summaryPanel").dialog('open');
    $("#summaryPanel").dialog( "option", "width", 500);
    $("#summaryPanel").dialog( "option", "height", 500);
    $("#summaryPanel").dialog( "option", "title", "Configure Display");
    $("#summaryPanel").dialog( "option", "modal", true);
    $("#summaryPanel" ).bind( "dialogbeforeclose", function(event, ui) {
        $("#summaryPanel").empty();
    });
    $(document).ready(function() {
		$("#summaryPanel").append("<p id=\'featureUsagePicker\'><label>Select minimum model feature coverage</label><br></p>");
		$("#featureUsagePicker").append("<input id=\'usageThresholdDisplay\' type=\'text\' value=\'"+thresholdFeatureUsage+"%\' style=\'text-align: center; border:0; font-weight:bold; width:420px; color:#999999;\'>");
		$("#summaryPanel").append("<div id=\'featureUsageSlider\' style=\'width:420px;\'><div>");
		$("#featureUsageSlider").slider({
			max: 100, 
			min: 0, 
			value: thresholdFeatureUsage, 
			slide: function(){
				featureUsageSliderLocked=1;
				setTimeout('featureUsageSliderLocked=0', 30);
				setTimeout(function(){
					if(!featureUsageSliderLocked){
						thresholdFeatureUsage = $("#featureUsageSlider").slider("value");
                		$("#usageThresholdDisplay").val(thresholdFeatureUsage+"%");
						viewAppliedTopModels();
					}
				},300);
			} 
		});

		//Filtering button
		if(filterLowCoverageRows){
			$("#summaryPanel").append("<input type=\'checkbox\' checked id=\'filterLowCoverageRowsCheckbox\'>");
		}else{
        	$("#summaryPanel").append("<input type=\'checkbox\' id=\'filterLowCoverageRowsCheckbox\'>");
		}
		$("#summaryPanel").append("<label> Do not display rows below minimum coverage</label>");
		$("#filterLowCoverageRowsCheckbox").change(function(){
			if($("#filterLowCoverageRowsCheckbox:checked").val() != undefined){
				filterLowCoverageRows = 1;
			}else{
				filterLowCoverageRows = 0;
			}
			viewAppliedTopModels();
		});
        
		//hide permutations button
		if(hidePermutedSamples){
            $("#summaryPanel").append("<br><br><input type=\'checkbox\' checked id=\'hidePermutedSamplesCheckbox\'>");
        }else{
            $("#summaryPanel").append("<br><br><input type=\'checkbox\' id=\'hidePermutedSamplesCheckbox\'>");
        }
        $("#summaryPanel").append("<label> Hide permuted background columns </label>");
        $("#hidePermutedSamplesCheckbox").change(function(){
            if($("#hidePermutedSamplesCheckbox:checked").val() != undefined){
                hidePermutedSamples = 1;
            }else{
                hidePermutedSamples = 0;
            }
            viewAppliedTopModels();
        });

		//hide sample labels button
       	if(hideSampleLabels){
            $("#summaryPanel").append("<br><br><input type=\'checkbox\' checked id=\'hideSampleLabelsCheckbox\'>");
        }else{
            $("#summaryPanel").append("<br><br><input type=\'checkbox\' id=\'hideSampleLabelsCheckbox\'>");
        }
 		$("#summaryPanel").append("<label> Hide sample labels for more horizontal space </label>");
        $("#hideSampleLabelsCheckbox").change(function(){
            if($("#hideSampleLabelsCheckbox:checked").val() != undefined){
                hideSampleLabels = 1;
            }else{
                hideSampleLabels = 0;
            }
            viewAppliedTopModels();
        });
		$("#summaryPanel").append("<br><br><br>");
        $("#summaryPanel").append("<p id=\'zScorePicker\'></p>");
		$("#zScorePicker").append("<label>Select Z-scores where colors saturate</label><br>");
		$("#zScorePicker").append("<input type=\'text\' value=\'"+minZ+"\' id=\'minZdisplay\' style=\'border:0; font-weight:bold; width: 200px; color:#00FF00;\'>");
		$("#zScorePicker").append("<input type=\'text\' value=\'"+maxZ+"\' id=\'maxZdisplay\' style=\'border:0; font-weight:bold; text-align:right; width: 220px; color:#FF0000\'>");
        $("#summaryPanel").append("<div id=\'minColorSlider\' style=\'float: left; width: 200px;\'><div>");
        $("#minColorSlider").slider({
			range: "max",
            max: 0,
            min: -10,
            step: 0.1,
			value: minZ,
            slide: function(){
				minZ = $("#minColorSlider").slider("value");
				$("#minZdisplay").val(minZ);
				viewAppliedTopModels();
        	}
        });
		$("#summaryPanel").append("<div style=\'float: left; width:20px; text-align: center;\'><b>0</b></div>");
		$("#summaryPanel").append("<div id=\'maxColorSlider\' style=\' float:left; width: 200px;\'><div>");
        $("#maxColorSlider").slider({
            range: "min",
			minRange: 0.01,
            max: 10,
            min: 0,
            step: 0.1,
            value: maxZ,
            slide: function(){
				maxZ = $("#maxColorSlider").slider("value");
				$("#maxZdisplay").val(maxZ);
                viewAppliedTopModels();
            }
        });
	});
    $("#summaryPanel").append("<br><br><div style=\'padding: 20px;\'><div class=\'tableButton\' style=\'width: 150px; text-align: center\' onclick=\'resetDefaultDisplayOptions()\'><b> Reset to defaults </b></div></div>");
}

function resetDefaultDisplayOptions(){
	thresholdFeatureUsage = 75;
	filterLowCoverageRows = 0;
	maxZ = 3;
	minZ = -3;
	hideSampleLabels= 0;
	hidePermutedSamples=0;
	viewAppliedTopModels();
	$("#summaryPanel").empty().dialog("close");
	viewDisplayConfiguration();
}

function loadTopModelDetails(jobId)
{
    $("#summaryPanel").dialog('open');
    $("#summaryPanel").dialog( "option", "width", 700);
    $("#summaryPanel").dialog( "option", "height", 300);
    $("#summaryPanel").dialog( "option", "title", "Top model details");
    $("#summaryPanel").dialog( "option", "modal", true);
    $("#summaryPanel" ).bind( "dialogbeforeclose", function(event, ui) {
        $("#summaryPanel").empty();
    });
    $(document).ready(function() {
        $("#summaryPanel").empty().html("<div id='loadingGif'><img src='css/images/ajax-loader.gif'></div>");
        $('html, body').animate({ scrollTop: $("#summaryPanel").offset().top - 40 , duration:'slow'});
    });

    var dataVars       = new Object;
    dataVars.pp_mode   = "loadTopModelDetails";
    dataVars.pp_jobId = jobId;
    $.ajax({
        type: "POST",
        url: "/cgi-bin/hgPredictorPrint",
        data: dataVars,
        dataType: "json",
        success: viewTopModelDetails,
        cache: true
    });
}

function viewTopModelDetails(data){
	$("#summaryPanel").empty();
	var task = $(data).attr('task');
	var classifier = $(data).attr('classifier');
	var trainingDataset = $(data).attr('trainingDataset');
    var subgrouping = $(data).attr('subgrouping');
	var featureSelection = $(data).attr('featureSelection');
	var transformation = $(data).attr('transformation');
	var featureCount = $(data).attr('featureCount');
	var avgTestingAccuracy = roundVal($(data).attr('avgTestingAccuracy') * 100);
	var avgTestingAccuracyGain = roundVal($(data).attr('avgTestingAccuracyGain') * 100);
	var accuracyType = $(data).attr('accuracyType');
	var URL = $(data).attr('URL');
	$("#summaryPanel").append("<table id=\'modelDetailsTable\' cellpadding=\'100\'></table>");
	$("#summaryPanel").append("<tr><td><b>Task</b></td><td>"+task+"</td></tr>");
    $("#summaryPanel").append("<tr><td><b>Classifier</b></td><td>"+classifier+"</td></tr>");
    $("#summaryPanel").append("<tr><td><b>Training dataset</b></td><td>"+trainingDataset+"</td></tr>");
    $("#summaryPanel").append("<tr><td><b>Subgrouping</b></td><td>"+subgrouping+"</td></tr>");
    $("#summaryPanel").append("<tr><td><b>Feature selection</b></td><td>"+featureSelection+"</td></tr>");
    $("#summaryPanel").append("<tr><td><b>Data transformation</b></td><td>"+transformation+"</td></tr>");
    $("#summaryPanel").append("<tr><td><b>Number of genomic features</b></td><td>"+featureCount+"</td></tr>");
    $("#summaryPanel").append("<tr><td><b>Avg. accuracy</b></td><td>"+avgTestingAccuracy+"%</td></tr>");
    $("#summaryPanel").append("<tr><td><b>Avg. accuracy gain</b></td><td>"+avgTestingAccuracyGain+"%</td></tr>");
    $("#summaryPanel").append("<tr><td><b>Accuracy metric</b></td><td>"+accuracyType+"</td></tr>");
	$("#summaryPanel").append("<tr><td><b>Link to topmodel</b></td><td><a href='"+URL+"'>Link</a></td></tr>");
}


//HELPER FUNCTIONS
function roundVal(val){
    var dec = 2;
    var result = Math.round(val*Math.pow(10,dec))/Math.pow(10,dec);
    return result;
}

function componentToHex(c) {
    var hex = c.toString(16);
    return hex.length == 1 ? "0" + hex : hex;
}

function rgbToHex(r, g, b) {
    return "#" + componentToHex(r) + componentToHex(g) + componentToHex(b);
}

function zScoreToHexColor(val){
	var max = maxZ;
	var min = minZ;
	var shades = 50;
	var colorStep = 255 / shades;
	var currWindowLim;
	var greenness;
	var redness;
	if(val <= 0){
		var valueStep = -(min) / shades;
		greenness=255;
		for(currWindowLim=min; (currWindowLim < val) && (currWindowLim <= 0); currWindowLim = (currWindowLim+valueStep)){
			greenness -= colorStep;
		}
		redness=0;
	}else{
		var valueStep = max / shades;
		redness=255;
        for(currWindowLim=max; (currWindowLim >= val) && (currWindowLim >= 0); currWindowLim = (currWindowLim-valueStep)){
            redness -= colorStep;
        }
		greenness=0;
	}
	greenness = Math.floor(greenness);
	redness = Math.floor(redness);
	color = rgbToHex(redness, greenness, 0);
	return color;
}
