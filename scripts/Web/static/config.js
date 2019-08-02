function displaySchemaOptions(schemas, flag){
	console.log(flag);
	//alert(flag);
	if(flag == 0){
		//alert("first");
	}
	else if(flag == 1){	
		//alert("second");
		//console.log(schemas);
		var schemaBar = document.getElementById("schemaBar");	

		for(var i = 0; i < Object.keys(schemas.schemaChoices).length; i++){
			var li = document.createElement("li");
			li.className = "nav-item";
	
			var a = document.createElement("a");
			a.className = "nav-link";
			a.href = "#bla";
			var name = schemas.schemaChoices[i].name;
			var nameWithoutExt = name.split(".").slice(0, -1).join(".")
			a.innerHTML = nameWithoutExt;
			a.id = 'a-'+name;
			a.addEventListener("click", function(){
			//alert(this.id);
				//find the schema

				$.ajax({url : '/add/'+ this.id.slice(2)  ,async:false, success : function(schema){	
					loadSchema(schema, {});
				}});
			});
		
			li.appendChild(a);
			schemaBar.appendChild(li);
		}
	}
	
}
function applyChanges(editor, boardName){
	var errors = editor.validate();
	if(!errors.length){	
		var urlPath = '/config/'.concat(boardName);
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
		
				
function removeBoard(boardName){
	var urlPath = "/config/" + boardName;
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

