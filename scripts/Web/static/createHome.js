
function createBoardNames(boardName){

	var bName = document.createElement("LABEL");
	bName.innerHTML = boardName;
	bName.style.fontSize = "x-large";
	bName.style.marginRight= "12px";
	bName.style.marginLeft= "12px";

        var col1 = document.createElement("th");
	//col1.className = "col-4";
	col1.appendChild(bName);
	//document.getElementById("boards").appendChild(col1);
	
	return col1;

}


function createConfigButtons(boardName){
	var btn = document.createElement("BUTTON");
	btn.innerHTML = "CONFIG";
	btn.className= "btn btn-primary btn-settings";
	btn.id = "config" + boardName;
	btn.addEventListener("click", function(){
		CHILD_WINDOW = window.open("/config/"+ ($('input[name=configFileGroup]:checked').val())+ "/" + boardName, 'Configuration Data','replace=true,top=200,left=100,height=800,width=1000,scrollbars=yes,titlebar=yes');
		console.log("CHILD", CHILD_WINDOW);
	})	
	var col2 = document.createElement("th");
	//col2.className = "col-auto";
	col2.appendChild(btn);
	
	return col2;
	//document.getElementById("boards").appendChild(col2);
}



function createLogButtons(boardName){
	
	var btn = document.createElement("BUTTON");
	btn.innerHTML = "LOG";
	btn.className= "btn btn-info btn-settings";
	//window.location.href= "/log";
	//print("board Name: ", configComp[i].name);	
	btn.addEventListener("click", function(){
		//window.location.href = "/log/" + boardName;	
		window.open("/log/" + boardName, "replace=true");
	}, false);
	
	var col4 = document.createElement("th");
	
	//col4.className = "col-auto";
	col4.appendChild(btn);
	
	return col4;
	//document.getElementById("boards").appendChild(col3);
}

 
function createInfoButtons(boardName, boardType){
	var btn = document.createElement("SPAN");

	btn.innerHTML = 'INFO';
	btn.className = "btn btn-info btn-settings";
	btn.id = "info-" + boardName;
	btn.addEventListener("click", function(){
		window.open("/monitoring/info/" + boardType + "/" + boardName, "replace=true");
	}, false);
	
	var col3 = document.createElement("th");
	//col3.className = "col-auto";
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

	var col5 = document.createElement("th");
	//col5.className = "col-auto";
	col5.appendChild(btn);

	return col5;
	//document.getElementById("boards").appendChild(col4);
}
function createBoardContainer(config){
	//document.write("debugworked")
	var boardTable = document.getElementById("boards");
	while(boardTable.firstChild && boardTable.removeChild(boardTable.firstChild));	
	var configComp = config.components;

	
	//var x = document.createElement("BR");
	//boardContainer.appendChild(x);
	for (var i = 0; i < Object.keys(configComp).length; i++) {

		//start by br
		
		var row = document.createElement("tr");
		//row.className = "row";
			
		var boardName = configComp[i].name;
		var boardType = configComp[i].type;
		//first column which shows the name of the board
		var nameCol = createBoardNames(boardName);

		//second column which shows the config button
		var configCol = createConfigButtons(boardName);

		//third column which shows the information
		var infoCol =  createInfoButtons(boardName, boardType);

		//fourth column which shows the LOG button
		var logCol = createLogButtons(boardName);

		//fith column which shows the LOG button
		var statusCol = createStatusBadges(i);

		row.appendChild(nameCol);
		row.appendChild(configCol);
		row.appendChild(logCol);
		row.appendChild(infoCol);
		row.appendChild(statusCol);
		boardTable.appendChild(row);
		//two br
		//var x = document.createElement("BR");
		//boardTable.appendChild(x);
		//var x = document.createElement("BR");
		//document.getElementById("boards").appendChild(x);




	}
	return true;
}

function createRadioInputs(fileNames){
		//alert("here in create radio");
		var radioContainer = document.getElementById("configFilesRadioGroup"); 
		while(radioContainer.firstChild && radioContainer.removeChild(radioContainer.firstChild));	
		for(var i = 0; i < Object.keys(fileNames.configFileNames).length; i++){
			var div = document.createElement("DIV");
			div.className = "custom-control custom-radio";
			
			var input = document.createElement("INPUT");
			input.setAttribute("type", "radio");
			input.className = "custom-control-input";
			input.setAttribute("name", "configFileGroup");
			input.setAttribute("value", fileNames.configFileNames[i].name);
			input.id = "radio-" + fileNames.configFileNames[i].name;

			var label = document.createElement("LABEL");
			label.className = "custom-control-label";
			label.htmlFor = "radio-" + fileNames.configFileNames[i].name;
			label.innerHTML = fileNames.configFileNames[i].name;
			
			input.addEventListener("click", function(){
				//TODO : update the run information section
				fileName = this.id.slice(6);
				$.ajax({url:"/configurationFiles/" + fileName, async: false, success: function(config){
					console.log(config);
					
					if(CHILD_WINDOW && !CHILD_WINDOW.closed){
						CHILD_WINDOW.close();
					}
					if(config == "NOTJSON" ||  config == "NOTSCHEMA" || config == "NOSCHEMA"){
						
						document.getElementById("run-info-box").style.display = "none";
						document.getElementById("control-box").style.display = "none";
						document.getElementById("status&settings-box").style.display = "none";

						//window.open("/damaged", '_self');
						if(config == "NOTJSON"){
							alert("The selected json file is damaged. Check for missing brackets or mismatched paratheses");
						}
						else if(config == "NOTSCHEMA"){
							alert("The selected json file was not validated by the general schema.");
						}
						else if(config == "NOSCHEMA"){
							alert("The json-config.schema does not exist.");
						}
						
					}
					else{

						document.getElementById("run-info-box").style.display = "initial";
						document.getElementById("control-box").style.display = "initial";
						document.getElementById("status&settings-box").style.display = "initial";
						createBoardContainer(config);
					}
				}});

				//updateBoardContainer(this.id.slice(6));
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


































