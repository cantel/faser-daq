
function updateBoardContainer(fileName){
	$.ajax({url:"/configurationFiles/" + fileName, async: false, success: function(config){
		console.log(config);
		createBoardContainer(config);
	}});
}


function updateStatus(data){
	//alert("here");
	console.log(data);	
	$(document).ready(function(){
	for (var i = 0; i < Object.keys(data.allStatus).length; i++){
		//document.write("in for of update status");

		document.getElementById("status" + i.toString()).className = "badge badge-success";
		//console.log(data.allStatus[i].state);
		//alert(data.allStatus[i].state);

		//updating the run inforamation section if the status of the eventbuilder01 changes from RUN or to RUN
		if(data.allStatus[i].name == "eventbuilder01"){
			if(document.getElementById("status" + i.toString()).innerHTML == "RUN" && data.allStatus[i].state != "RUN"){
				clearInterval(intervalIndices[0]);
				var graphWindow = document.getElementById("graphWindow");
				//while(graphWindow.firstChild && graphWindow.removeChild(graphWindow.firstChild));
			}
			else if(document.getElementById("status" + i.toString()).innerHTML != "RUN" && data.allStatus[i].state == "RUN"){
				intervalIndices[0] = updateRunInformation(true);
			}

		
			//if(document.getElementById("status" + i.toString()).innerHTML != data.allStatus[i].state  && data.allStatus[i].state != "RUN")
			//	setDefaultValues();
			

		}
		document.getElementById("status" + i.toString()).innerHTML = data.allStatus[i].state;
		if(data.allStatus[i].state == "TIMEOUT"){
			document.getElementById("status" + i.toString()).className = "badge badge-danger";
		}

	}

	});
}

function updateCommandAvailability(data){
	var allDOWN = true;
	var allREADY = true;
	var allRUNNING = true;
	var allBooted = true;
	
	for(var i = 0; i < Object.keys(data.allStatus).length; i++){
		if( (data.allStatus[i].state) != "DOWN"){
			allDOWN = false;
		}

		if( (data.allStatus[i].state) != "READY"){
			allREADY = false;
		}

		if( (data.allStatus[i].state) != "RUN"){
			allRUNNING = false;
		}
}

	console.log("CHILD window while updating: ", CHILD_WINDOW);

	runningFileInfo = getRunningFileInfo();
	if( runningFileInfo.isRunning == 1 && (($('input[name=configFileGroup]:checked').val()) != runningFileInfo.fileName)  ){
		disableControls(true);
	}
	else{	
		if (allDOWN){
			document.getElementById("INITIALISE").disabled = false;
			document.getElementById("SHUTDOWN").disabled = true;
			/*if(CHILD_WINDOW && !CHILD_WINDOW.closed){
				$(CHILD_WINDOW.document).ready(function() {
					CHILD_WINDOW.disableAddingOrChangingBoards(false);
				});
			}*/
		}
		else{
			document.getElementById("INITIALISE").disabled = true;
			document.getElementById("SHUTDOWN").disabled = false;

			/*if(CHILD_WINDOW && !CHILD_WINDOW.closed)

				$(CHILD_WINDOW.document).ready(function() {
					CHILD_WINDOW.disableAddingOrChangingBoards(true);
				});*/
		}
		if(allREADY){
		document.getElementById("START").disabled = false;
		}
		else{
			document.getElementById("START").disabled = true;
		}

		if(allRUNNING){
			document.getElementById("STOP").disabled = false;
		}
		else{
			document.getElementById("STOP").disabled = true;
		}

	}
}

function updateCommandsAndStatus(){
	$.ajax({url: '/status', async: false, success: function(data){
		console.log(data);
		console.log("in update general funciton");
		updateStatus(data);
		updateCommandAvailability(data);
	}}
	);
}


function updateConfigFileChoices(){
	var fileNames;
	$.ajax({url : '/configurationFiles/fileNames', async:false, success : function(data){
		fileNames = data;
	}});
	createRadioInputs(fileNames);	
	
}


function updateRunInformation(){
	var graphWindow = document.getElementById("graphWindow");
	
	//setinterval for the run table
	var interval_updateRunNumbers = setInterval(function(){ updateRunNumbers();}, 1000);

	//load the iframe
	while(graphWindow.firstChild && graphWindow.removeChild(graphWindow.firstChild));	
	var graph = document.createElement("IFRAME");
	graph.className = "embed-responsive-item";
	graph.src = "/monitoring/graph";
	
	graphWindow.appendChild(graph);
	return interval_updateRunNumbers;
}
function updateRunNumbers(){
	$.ajax({url:"/monitoring/info/eventbuilder01/dataUpdate", async:false, success: function(data){
		//console.log("helllooooo", data);
		var date;
		for(var i = 0; i < Object.keys(data.values).length; i++){
			if(document.getElementById(data.values[i].key)){
				if(data.values[i].key == "RunStart"){
					console.log("time", data.values[i].value);
					date = convertToDate(data.values[i].value);
					document.getElementById(data.values[i].key).innerHTML = date;
				}
				else{
					document.getElementById(data.values[i].key).innerHTML = data.values[i].value;
				}
				
			}

		}
	}});
}
function updateColorOfInfoButtons(){
	
	$.ajax({url: "/monitoring/info/updateStatus", async: false, success: function(data){
		console.log(data);
		
		for(var i = 0; i < Object.keys(data.allStatus).length; i++){
			var source = data.allStatus[i].source;
			var statusVal = data.allStatus[i].statusVal;
			if(statusVal == 0)
				document.getElementById("info-"+source).className = "btn btn-success btn-settings";
			else if(statusVal == 1)
				document.getElementById("info-"+source).className = "btn btn-warning btn-settings";
			else if (statusVal == 2)
				document.getElementById("info-"+source).className = "btn btn-danger btn-settings";
		}
	}});
}
function updateRunningFile(){

	var data = getRunningFileInfo();
	if(data.isRunning == 0){
		document.getElementById("runningFile").innerHTML = "NO file running";
	}		
	else if(data.isRunning == 1){
		document.getElementById("runningFile").innerHTML = "File " + data.fileName + " is running";
	}

		
}

function goToCurrent(){

	document.getElementById('radio-current.json').click();
}

function addBoard(){
	
	CHILD_WINDOW = window.open("/add", 'Configuration Data','replace=true,top=200,left=100,height=800,width=1200,scrollbars=yes,titlebar=yes');
}
function saveConfigFile(){
	var configFileName = prompt("Please enter your new configuration filename without json at the end:", "configXXX");
	if (configFileName == null || configFileName == "") {
		alert("You canceled saving the file.");
	} else {

		
		$.ajax({url:"/configurationFiles/save/" + configFileName, async: false, success: function(result){
			if(result.message){
				var mes = "File " + configFileName + " has been successfully added";
				 updateConfigFileChoices();
				document.getElementById('radio-current.json').checked = true;
				alert(mes);
			}
			else{
				alert("Oops! This file already exists");
			}

		}});
	}
}


function isDOWN(){
	var allDOWN = true;
	$.ajax({url: '/status', async: false, success: function(data){
		
		for(var i = 0; i < Object.keys(data.allStatus).length; i++){
			if( (data.allStatus[i].state) != "DOWN"){
				allDOWN = false;
			}
		}

	}}
	);
	return allDOWN;
}

function initialise(){
	$.get('/initialise');
}

function start(){
	$.get('/start');
}

function stop(){
	$.get('/stop');
}

function getRunningFileInfo(){
	var info;
	$.ajax({url: "/runningFile", async: false, success: function(data){
		info = data;
	}});
	console.log(info);
	return info;
		
}
function shutdown(){
	$.get('/shutdown');
	var inter = setInterval(function(){
	$.ajax({url:"/shutDownRunningFile", async:false, success: function(data){
		if(data == "true"){
			clearInterval(inter);	
		}
		//else if(data == "true"){
		//}
	}
	
	});
	},1000);
}

function disableControls(bool){
	document.getElementById("INITIALISE").disabled = bool;
	document.getElementById("START").disabled = bool;
	document.getElementById("STOP").disabled = bool;
	document.getElementById("SHUTDOWN").disabled = bool;
}

function convertToDate(timestamp){
	time = new Date(timestamp * 1000);
	var year = time.getFullYear();
	var month = time.getMonth() + 1;
	var day = time.getDate();
	var hour = time.getHours();
	var minutes = "0" + time.getMinutes();
	var seconds = "0" + time.getSeconds();	
	var convertedTime = month + "/" + day + "/" + year + "  " + hour + ":" + minutes.substr(-2) + ":" + seconds.substr(-2);
	return convertedTime;
}
/*
function setDefaultValues(){
	
		var graphWindow = document.getElementById("graphWindow");

		while(graphWindow.firstChild && graphWindow.removeChild(graphWindow.firstChild));
		var noInfo = document.createElement("H1");
		noInfo.innerHTML = "THE EVENTBUILDER IS NOT RUNNING. NO INFO AVAILABLE.";
		graphWindow.appendChild(noInfo);

		document.getElementById("PhysicsEvents").innerHTML = "";
		document.getElementById("PhysicsRate").innerHTML = "";
		document.getElementById("MonitoringEvents").innerHTML = "";
		document.getElementById("MonitoringRate").innerHTML = "";
		document.getElementById("CalibrationEvents").innerHTML = "";
		document.getElementById("CalibrationRate").innerHTML = "";
		document.getElementById("RunNumber").innerHTML = "";
		document.getElementById("RunStart").innerHTML = "";
}
*/
/*
function displaySaveButton(){

	
	if(document.getElementById('radio-current.json').checked == true){
		var save = document.createElement("button");
		save.className = "btn btn-primary";
		save.innerHTML = "SAVE";
		save.id = "save-button";
		save.style.cssFloat = "right";
		save.addEventListener("click", function(){
			saveConfigFile();
		});
		
		document.getElementById("saveNewFile").appendChild(save);
	}

}

*/
/*
function disableFileChoice(bool){
	
	$.ajax({url : '/configurationFiles/fileNames', async:false, success : function(data){
		fileNames = data;
	}});


	for(var i = 0; i < Object.keys(fileNames.configFileNames).length; i++){
		document.getElementById("radio-" + fileNames.configFileNames[i].name).disabled = bool;
	}
}
*/
/*
function disableConfigButtons(data, bool){
	
	for(var i = 0; i < Object.keys(data.allStatus).length; i++){
		document.getElementById("config" + data.allStatus[i].name).disabled = bool;
	}
}
*/

