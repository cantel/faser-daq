
function updateBoardContainer(fileName){
	$.ajax({url:"/configurationFiles/" + fileName, async: false, success: function(config){
		//console.log(config);
		createBoardContainer(config);
	}});
}


function updateStatus(data){
    for (var i = 0; i < Object.keys(data.allStatus).length; i++){
	statusElement=document.getElementById("status" + i.toString());
	if (statusElement==null) continue;
	statusElement.innerHTML = data.allStatus[i].state;
	if(data.allStatus[i].state == "TIMEOUT"){
	    document.getElementById("status" + i.toString()).className = "badge badge-danger";
	} else {
	    document.getElementById("status" + i.toString()).className = "badge badge-success";
	}
	var source = data.allStatus[i].name;
	var statusVal = data.allStatus[i].infoState;
	if(statusVal == 0)
	    document.getElementById("info-"+source).className = "btn btn-success btn-settings";
	else if(statusVal == 1)
	    document.getElementById("info-"+source).className = "btn btn-warning btn-settings";
	else if (statusVal == 2)
	    document.getElementById("info-"+source).className = "btn btn-danger btn-settings";
    }
}

function updateCommandAvailability(data){
    document.getElementById("runningState").innerHTML =data.globalStatus;
    document.getElementById("runningFile").innerHTML = data.runState.fileName;
    var buttonEnableStates= {
	"INITIALISE": ["DOWN"],
	"START": ["READY","PAUSED"],
	"STOP": ["RUN","PAUSED"],
	"SHUTDOWN": ["READY","RUN","PAUSED","IN TRANSITION"],
	"PAUSE": ["RUN"],
	"ECR": ["PAUSED"] };
    var runningFileInfo=data.runState;
    if( data.globalStatus!="DOWN" && (($('input[name=configFileGroup]:checked').val()) != runningFileInfo.fileName)  ){
	disableControls(true);
    }
    else{	
	for (const [button, states] of Object.entries(buttonEnableStates)) {
	    el=document.getElementById(button);
	    el.disabled = ! states.includes(data.globalStatus);
	    if (data.globalStatus!="IN TRANSITION" && data.globalStatus!="ADDED") el.children[0].style="display: none";
	}
    }
}

function updateRunNumber(data){
    document.getElementById("RunNumber").innerHTML=data.runNumber;
    document.getElementById("RunStart").innerHTML=convertToDate(data.runStart);

}

function updateCommandsAndStatus(){
    const source = new EventSource("/state");
    
    source.onmessage = function (event) {
	const data = JSON.parse(event.data);
	//console.log(data);
	updateStatus(data);
	updateCommandAvailability(data);
	updateRunNumber(data);
    }
    source.onerror = function (event) {
	console.log(event);
	console.log($("#reloadModal"));
	$("#reloadModal").modal();	
	event.srcElement.onerror=null;
    }
}


function updateConfigFileChoices(){
	var fileNames;
	$.ajax({url : '/configurationFiles/fileNames', async:false, success : function(data){
		fileNames = data;
	}});
	createRadioInputs(fileNames);	
	
}

function goToCurrent(){

	document.getElementById('radio-current.json').click();
}

function addBoard(boardType){
    console.log($('input[name=configFileGroup]:checked').val());
    CHILD_WINDOW = window.open("/add/"+boardType, 'Configuration Data','replace=true,top=200,left=100,height=800,width=1200,scrollbars=yes,titlebar=yes');
    
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


function selectedFile() {
    return $('input[name=configFileGroup]:checked').val()
}

function isDOWN(){
    return document.getElementById("runningState").innerHTML=="DOWN";
}

function initialise(self){
    self.children[0].style="display: inline-block";
    $.get('/initialise');
}

function start(self){
    self.children[0].style="display: inline-block";
    if (document.getElementById("ECR").disabled) {
	$.get('/start');
    } else {
	$.get('/unpause');
    }
}

function pause(self){
    self.children[0].style="display: inline-block";
	$.get('/pause');
}

function sendECR(self){
    self.children[0].style="display: inline-block";
	$.get('/ecr');
}

function stop(self){
    self.children[0].style="display: inline-block";
	$.get('/stop');
}

function shutdown(self){
    self.children[0].style="display: inline-block";
	$.get('/shutdown');
}

function disableControls(bool){
	document.getElementById("INITIALISE").disabled = bool;
	document.getElementById("START").disabled = bool;
	document.getElementById("STOP").disabled = bool;
	document.getElementById("SHUTDOWN").disabled = bool;
	document.getElementById("PAUSE").disabled = bool;
	document.getElementById("ECR").disabled = bool;
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
