
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
	btn.className= "btn btn-primary btn-lg";
	btn.id = "config" + boardName;
	btn.addEventListener("click", function(){
		window.open("/config/" + boardName, 'Configuration Data','replace=true,top=200,left=100,height=800,width=1000,scrollbars=yes,titlebar=yes');
	})	
	var col2 = document.createElement("DIV");
	col2.className = "col-3";
	col2.appendChild(btn);
	
	return col2;
	//document.getElementById("boards").appendChild(col2);
}


function createLogButtons(boardName){

	var btn = document.createElement("BUTTON");
	btn.innerHTML = "LOG";
	btn.className= "btn btn-info btn-lg";
	//window.location.href= "/log";
	//print("board Name: ", configComp[i].name);	
	btn.addEventListener("click", function(){
		//window.location.href = "/log/" + boardName;	
		window.open("/log/" + boardName, "replace=true");
	}, false);
	
	var col3 = document.createElement("DIV");
	
	col3.className = "col-2";
	col3.appendChild(btn);
	
	return col3;
	//document.getElementById("boards").appendChild(col3);
}


function createStatusBadges(i){

	var btn = document.createElement("SPAN");


	btn.innerHTML = 'LOAD';
	btn.className= "badge badge-success";
	btn.style.fontSize= "xx-large";
	btn.style.background= "Green";
	btn.id = "status" + i.toString();

	var col4 = document.createElement("DIV");
	col4.className = "col-3";
	col4.appendChild(btn);

	return col4;
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
		//third column which shows the LOG button
		var logCol = createLogButtons(boardName);
		//fourth column which shows the LOG button
		var statusCol = createStatusBadges(i);

		row.appendChild(nameCol);
		row.appendChild(configCol);
		row.appendChild(logCol);
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
	for (var i = 0; i < Object.keys(data.allStatus).length; i++){
		//document.write("in for of update status");

		document.getElementById("status" + i.toString()).className = "badge badge-success";
		//console.log(data.allStatus[i].state);
		//alert(data.allStatus[i].state);
		document.getElementById("status" + i.toString()).innerHTML = data.allStatus[i].state;
		if(data.allStatus[i].state == "TIMEOUT"){
			document.getElementById("status" + i.toString()).className = "badge badge-danger";
		}

	}
}

function disableFileChoice(bool){
	
	$.ajax({url : '/configFileNames', async:false, success : function(data){
		fileNames = data;
		//alert(da.configFileNames[0].name);	
		//alert(da);
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
	
	if (allDOWN){
		document.getElementById("INITIALISE").disabled = false;
		document.getElementById("SHUTDOWN").disabled = true;
		disableFileChoice(false);
		disableConfigButtons(data, false);
	}
	else{
		disableFileChoice(true);
		disableConfigButtons(data, true);
		document.getElementById("INITIALISE").disabled = true;
		document.getElementById("SHUTDOWN").disabled = false;
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

function updateCommandsAndStatus(){
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

			//if(fileNames.configFileNames[i].name == "current.json"){
				//input.checked = "checked";
			//}
		
			var label = document.createElement("LABEL");
			label.className = "custom-control-label";
			label.htmlFor = "radio-" + fileNames.configFileNames[i].name;
			label.innerHTML = fileNames.configFileNames[i].name;
			
			input.addEventListener("click", function(){
				//alert(this.id);
				updateBoardContainer(this.id.slice(6));
				if(this.id == "radio-current.json")
					displaySaveButton();
				else{
					var saveContainer = document.getElementById("saveNewFile");
					while(saveContainer.firstChild && saveContainer.removeChild(saveContainer.firstChild));
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
		//alert(da.configFileNames[0].name);	
		//alert(da);
	}});
	//alert(da.configFileNames[0].name);	
	createRadioInputs(fileNames);	

}

function goToCurrent(){
	//alert('simulating click');
	document.getElementById('radio-current.json').checked = true;
	//alert("success");
	updateBoardContainer('current.json');
	
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
		//alert("here");
		var save = document.createElement("button");
		save.className = "btn btn-primary";
		save.innerHTML = "SAVE";
		save.style.cssFloat = "right";
		save.addEventListener("click", function(){
			saveConfigFile();
		});
		
		document.getElementById("saveNewFile").appendChild(save);
	}

}
