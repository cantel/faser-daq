
function applyChanges(editor, fileName, boardName){
	var errors = editor.validate();
	if(!errors.length){	
		var urlPath = '/config/'+ fileName + "/" +  boardName;
		urlPath = urlPath.concat('/changeConfigFile');
		//alert(urlPath);
		value = editor.getValue();
		//console.log(value);	
		$.ajax({url: urlPath, async : false, type:'POST', dataType: 'json', data : JSON.stringify(value), contentType:"application/json"});	
		closeWindow();
	}
	else{
		alert("Errors in submitted form!!");
		console.log(errors);
	}
}	
		
				
function removeBoard(fileName, boardName){
	var urlPath = "/config/" + fileName + "/"+ boardName;
	urlPath = urlPath.concat('/removeBoard');
	//alert(urlPath);
	$.ajax({url: urlPath, async: false});
	closeWindow();
}



function closeWindow(){
	window.opener.goToCurrent();
	//alert("done");
	window.close();
}

//takes the general schema and replaces the settings, name, type
function improveSchema(generalSchema, schema){

	generalSchema = generalSchema.properties.components.items;
	//return generalSchema;
	var improvedSchema = generalSchema;
	//console.log("schema properties name", schema.properties.name);	
	improvedSchema.properties.name = schema.properties.name;
	//improvedSchema.properties.host = schema.properties.host;
	improvedSchema.properties.type = schema.properties.type;
	improvedSchema.properties.settings = schema.properties.settings;
        improvedSchema.title = schema.title;
        improvedSchema.required = improvedSchema.required + ["components"];
	//console.log("the improved schema: ",improvedSchema);
	return improvedSchema;
}




function loadSchema(schema, value){
    //console.log(schema);
    var element = document.getElementById('editor-holder');
    //JSONEditor.defaults.editors.object.options.hidden = true;
    //JSONEditor.defaults.editors.object.options.disable_edit_json = true;	
    //JSONEditor.defaults.editors.object.options.disable_properties = true;
    while(element.firstChild && element.removeChild(element.firstChild));	
    editor = new JSONEditor(element, {
	schema:schema,
	theme:'tailwind',
	iconlib:'fontawesome4',
	startval:value,
	required_by_default:true 
			
    });
    //return editor;
}

function loadButtons(flag, fileName, boardName){
    var buttonsContainer = document.getElementById("buttons");
    //config
    //alert(flag);
    if(flag == 0){
	var remove = document.createElement("button");
	remove.className = "inline-block align-middle text-center text-sm bg-blue-700 text-white py-1 pr-1 m-2 shadow select-none whitespace-no-wrap rounded json-editor-btn-delete json-editor-btntype-deletelast"; 
	remove.style.cssFloat = "right";
	remove.innerHTML = "REMOVE BOARD";
	//remove.style.marginLeft = "10px";
	//remove.style.marginRight = "10px";
	remove.addEventListener("click", function(){
	    removeBoard(fileName, boardName);
	});
	remove.id = "remove-button";
						
	/*if(window.opener.isDOWN()){
	  remove.disabled = false;
	  }*/
	buttonsContainer.appendChild(remove);
	
	buttonsContainer.appendChild(document.createTextNode("   "));
						
	var apply = document.createElement("button");
	apply.className = "inline-block align-middle text-center text-sm bg-blue-700 text-white py-1 pr-1 m-2 shadow select-none whitespace-no-wrap rounded json-editor-btn-add json-editor-btntype-add";
	apply.style.cssFloat = "right";
	apply.innerHTML = "APPLY CHANGES";
	apply.style.marginLeft = "10px";
	apply.style.marginRight = "10px";
	apply.addEventListener("click", function(){
	    //	if(window.opener.isDOWN())
	    applyChanges(editor, fileName, boardName);	
	    //	else
	    //		alert("not in DOWN mode");
	});
	
	apply.id = "apply-button";
	buttonsContainer.appendChild(apply);
	
    }
    else if (flag == 1){
	
	var add = document.createElement("button");
	add.className = "inline-block align-middle text-center text-sm bg-blue-700 text-white py-1 pr-1 m-2 shadow select-none whitespace-no-wrap rounded json-editor-btn-add json-editor-btntype-add";
	add.style.cssFloat = "right";
	add.innerHTML = "ADD BOARD";
	add.addEventListener("click", function(){
	    //if(window.opener.isDOWN())
	    addBoard(editor);
	    //else
	    //alert("not in DOWN mode");
	});
	
	add.id = "add-button";
	buttonsContainer.appendChild(add);
	
    }
}


function addBoard(){
    currentFile=window.opener.selectedFile();
    console.log("Add to file: "+currentFile);
    var urlPath = '/add/addBoard/'+currentFile;
    //alert(urlPath);
    var errors = editor.validate();
    console.log("Errors: "+errors);
    if(!errors.length){
	
	var value = editor.getValue();
	//console.log(value);	
	$.ajax({url: urlPath, async : false, type:'POST', dataType: 'json', data : JSON.stringify(value), contentType:"application/json", success : function(data){
	    var message = data.message;
	    alert(message);
	}});	
	
	window.opener.goToCurrent();
    }
    else{
	alert("errors in submitted form, check console for more details");
    }
}


function disableConfigWindow(){
    
    var disable = !(window.opener.isDOWN());
    
    if(document.getElementById("remove-button"))	
	document.getElementById("remove-button").disabled = disable;	
    
    if(document.getElementById("apply-button"))
	document.getElementById("apply-button").disabled = disable;
    
    if(document.getElementById("add-button"))
	document.getElementById("add-button").disabled = disable;
    
    if(editor){
	if(disable){
	    editor.disable();
	}
	else{	
	    editor.enable();
	}
    }
}
