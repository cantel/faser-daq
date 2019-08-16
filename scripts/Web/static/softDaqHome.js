
function createBoardNames(boardName){

	var bName = document.createElement("LABEL");
	bName.innerHTML = boardName;
	bName.style.fontSize = "x-large";

        var col1 = document.createElement("DIV");
	col1.className = "col-4";
	col1.appendChild(bName);
	document.getElementById("boards").appendChild(col1);
	
	return col1;

}


function createConfigButtons(boardName){
	var btn = document.createElement("BUTTON");
	btn.innerHTML = "CONFIG";
	btn.className= "btn btn-primary";
	btn.id = "config" + boardName;
	btn.addEventListener("click", function(){
		CHILD_WINDOW = window.open("/config/" + boardName, 'Configuration Data','replace=true,top=200,left=100,height=800,width=1000,scrollbars=yes,titlebar=yes');
		console.log("CHILD", CHILD_WINDOW);
	})	
	var col2 = document.createElement("DIV");
	col2.className = "col-auto";
	col2.appendChild(btn);
	
	return col2;
	//document.getElementById("boards").appendChild(col2);
}



function createLogButtons(boardName){

	var btn = document.createElement("BUTTON");
	btn.innerHTML = "LOG";
	btn.className= "btn btn-info";
	//window.location.href= "/log";
	//print("board Name: ", configComp[i].name);	
	btn.addEventListener("click", function(){
		//window.location.href = "/log/" + boardName;	
		window.open("/log/" + boardName, "replace=true");
	}, false);
	
	var col4 = document.createElement("DIV");
	
	col4.className = "col-auto";
	col4.appendChild(btn);
	
	return col4;
	//document.getElementById("boards").appendChild(col3);
}

 
function createInfoButtons(boardName){
	var btn = document.createElement("SPAN");

	btn.innerHTML = 'INFO';
	btn.className = "btn btn-info";
	btn.id = "info-" + boardName;
	btn.addEventListener("click", function(){
		window.open("/monitoring/info/" + boardName, "replace=true");
	}, false);
	
	var col3 = document.createElement("DIV");
	col3.className = "col-auto";
	col3.appendChild(btn);

	return col3;
}
function createStatusBadges(i){

	var btn = document.createElement("SPAN");


	btn.innerHTML = 'LOAD';
	btn.className= "badge badge-success";
	btn.style.fontSize= "x-large";
	btn.style.background= "Green";
	btn.id = "status" + i.toString();

	var col5 = document.createElement("DIV");
	col5.className = "col-auto";
	col5.appendChild(btn);

	return col5;
	//document.getElementById("boards").appendChild(col4);
}
function updateBoardContainer(fileName){
	$.ajax({url:"/configurationFile/" + fileName, async: false, success: function(config){
		createBoardContainer(config);
	}});
}
function createBoardContainer(config){
	//document.write("debugworked")
	var boardContainer = document.getElementById("boards");
	while(boardContainer.firstChild && boardContainer.removeChild(boardContainer.firstChild));	
	var configComp = config.components;

	
	var x = document.createElement("BR");
	boardContainer.appendChild(x);
	for (var i = 0; i < Object.keys(configComp).length; i++) {

		//start by br
		
		var row = document.createElement("DIV");
		row.className = "row";	
			
		var boardName = configComp[i].name;
		//first column which shows the name of the board
		var nameCol = createBoardNames(boardName);

		//second column which shows the config button
		var configCol = createConfigButtons(boardName);

		//third column which shows the information
		var infoCol =  createInfoButtons(boardName);

		//fourth column which shows the LOG button
		var logCol = createLogButtons(boardName);

		//fith column which shows the LOG button
		var statusCol = createStatusBadges(i);

		row.appendChild(nameCol);
		row.appendChild(configCol);
		row.appendChild(logCol);
		row.appendChild(infoCol);
		row.appendChild(statusCol);
		boardContainer.appendChild(row);
		//two br
		var x = document.createElement("BR");
		boardContainer.appendChild(x);
		//var x = document.createElement("BR");
		//document.getElementById("boards").appendChild(x);




	}
	return true;
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

function shutdown(){
	$.get('/shutdown');
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

function disableFileChoice(bool){
	
	$.ajax({url : '/configFileNames', async:false, success : function(data){
		fileNames = data;
	}});


	for(var i = 0; i < Object.keys(fileNames.configFileNames).length; i++){
		document.getElementById("radio-" + fileNames.configFileNames[i].name).disabled = bool;
	}
}

function disableConfigButtons(data, bool){
	
	for(var i = 0; i < Object.keys(data.allStatus).length; i++){
		document.getElementById("config" + data.allStatus[i].name).disabled = bool;
	}
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

		//if( (data.allStatus[i].state) != "b'booted'"){
		//	allBOOTED = false;
		//}
	}
	console.log("CHILD window while updating: ", CHILD_WINDOW);
	if (allDOWN){
		document.getElementById("INITIALISE").disabled = false;
		document.getElementById("SHUTDOWN").disabled = true;
		//disableFileChoice(false);
		//disableConfigButtons(data, false);
		if(CHILD_WINDOW && !CHILD_WINDOW.closed){
			$(CHILD_WINDOW.document).ready(function() {
  				 // $(win.document).contents().find("...").doStuff();
	
				//$(CHILD_WINDOW.document).contents().disableAddingOrChangingBoards(false);

				CHILD_WINDOW.disableAddingOrChangingBoards(false);
			});
		}
	}
	else{
		//disableFileChoice(true);
		//disableConfigButtons(data, true);
		document.getElementById("INITIALISE").disabled = true;
		document.getElementById("SHUTDOWN").disabled = false;

		if(CHILD_WINDOW && !CHILD_WINDOW.closed)

			$(CHILD_WINDOW.document).ready(function() {
  				 // $(win.document).contents().find("...").doStuff();
	
				//$(CHILD_WINDOW.document).contents().disableAddingOrChangingBoards(true);

				CHILD_WINDOW.disableAddingOrChangingBoards(true);
			});
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

	
	//if(allBOOTED){
	//	document.getElementById("INISIALISE").disabled = false;
	//}
	//else{
		
	//	document.getElementById("INISIALISE").disabled = true;
	//}

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


function updateCommandsAndStatusAndButtons(){
	//var dat;
	//document.write('in updateStaus');
	//document.write(Object.keys(config.components).length);
	$.ajax({url: '/status', async: false, success: function(data){
		//dat  = data.allStatus[0].state;
		//document.write(dat);
		console.log(data);
		console.log("in update general funciton");
		updateStatus(data);
		updateCommandAvailability(data);
	}}
	);
	//document.write(dat);		
	//document.write(dat.allStatus[0].status);
	updateColorOfInfoButtons();
}

function createRadioInputs(fileNames){
		var radioContainer = document.getElementById("configFilesRadioGroup"); 
		while(radioContainer.firstChild && radioContainer.removeChild(radioContainer.firstChild));	
		for(var i = 0; i < Object.keys(fileNames.configFileNames).length; i++){
			var div = document.createElement("DIV");
			div.className = "custom-control custom-radio";
			
			var input = document.createElement("INPUT");
			input.setAttribute("type", "radio");
			input.className = "custom-control-input";
			input.setAttribute("name", "configFileGroup");

			input.id = "radio-" + fileNames.configFileNames[i].name;

			var label = document.createElement("LABEL");
			label.className = "custom-control-label";
			label.htmlFor = "radio-" + fileNames.configFileNames[i].name;
			label.innerHTML = fileNames.configFileNames[i].name;
			
			input.addEventListener("click", function(){
				//TODO : update the run information section
				updateBoardContainer(this.id.slice(6));
				if(this.id == "radio-current.json"){
					document.getElementById("save-button").style.display = "initial";
				}
				else{
					document.getElementById("save-button").style.display = "none";
				}
			});
			div.appendChild(input);
			div.appendChild(label);
			
			radioContainer.appendChild(div);
		
		}

}
function updateConfigFileChoices(){
	var fileNames;
	$.ajax({url : '/configFileNames', async:false, success : function(data){
		fileNames = data;
	}});
	createRadioInputs(fileNames);	
	
}

function goToCurrent(){

	document.getElementById('radio-current.json').click();
}

function addBoard(){
	
	window.open("/add", 'Configuration Data','replace=true,top=200,left=100,height=800,width=1200,scrollbars=yes,titlebar=yes');
}	


function saveConfigFile(){
	var configFileName = prompt("Please enter your new configuration filename:", "configXXX");
	if (configFileName == null || configFileName == "") {
		alert("You canceled saving the file.");
	} else {

		
		$.ajax({url:"/saveConfigFile/" + configFileName, async: false, success: function(result){
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

function updateRunInformation(){
/*	$.ajax({url: "/status/eventbuilder01", async: false, success: function(data){
		console.log("status of eventBuilder", data);	
			
	}});  */

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

function setDefaultValues(){
	
		var graphWindow = document.getElementById("graphWindow");

		while(graphWindow.firstChild && graphWindow.removeChild(graphWindow.firstChild));
		var noInfo = document.createElement("H1");
		//var t = document.createTextNode("THE EVENTBUILDER IS NOT RUNNING. NO INFO AVAILABLE.");
		//noInfo.appendChild(t);
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
function updateRunNumbers(){
	$.ajax({url:"/monitoring/info/eventbuilder01/dataUpdate", async:false, success: function(data){
		//alert("updating run numbers");	
		console.log("helllooooo", data);
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
			//alert("source: ", source, " statusVal: ",statusVal);
			if(statusVal == 0)
				document.getElementById("info-"+source).className = "btn btn-success";
			else if(statusVal == 1)
				document.getElementById("info-"+source).className = "btn btn-warning";
			else if (statusVal == 2)
				document.getElementById("info-"+source).className = "btn btn-danger";
		}
	}});
}
