function displaySchemaOptions(schemas, flag){
	console.log(flag);
	//alert(flag);
	if(flag == 0){
		//alert("first");
	}
	else if(flag == 1){	
		//alert("second");
		//console.log(schemas);
		var schemaBar = document.getElementById("schemaDropDown");	
		var dropDown = document.createElement("button");
		dropDown.className = "btn btn-primary dropdown-toggle btn-lg";
		dropDown.setAttribute("type", "button");
		dropDown.innerHTML = "Schema Options";
		dropDown.setAttribute("data-toggle","dropdown");	
		var caret = document.createElement("span");
		caret.className = "caret";
		dropDown.appendChild(caret);

		schemaBar.appendChild(dropDown);

		var u1 = document.createElement("u1");
		u1.className = "dropdown-menu";
		schemaBar.appendChild(u1);

		for(var i = 0; i < Object.keys(schemas.schemaChoices).length; i++){
			var li = document.createElement("li");
			//li.className = "nav-item";
	
			var a = document.createElement("a");
			//a.className = "nav-link";
			a.href = "#schema choice";
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
			u1.appendChild(li);
		}

		$(document).ready(function() {  
   			$(".dropdown .dropdown-menu li a")[0].click();
		});	
	}
	
}
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


