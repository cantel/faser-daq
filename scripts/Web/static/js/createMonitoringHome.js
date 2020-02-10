function createComponentButton(componentName){

	var btn = document.createElement("BUTTON");
	btn.innerHTML = componentName;
	btn.className= "btn btn-info btn-settings";
	//window.location.href= "/log";
	//print("board Name: ", configComp[i].name);	
	btn.addEventListener("click", function(){
		window.open("/histograms/" + componentName.lower(), "replace=true");
	}, false);
   
        return btn
    
}

function addComponentButtons(){

        var row = document.createElement("tr");	
        var tlb_btn = createComponentButton("TLB")
        row.appendChild(tlb_btn)

        var tracker_btn = createComponentButton("Tracker")
        row.appendChild(tracker_btn)
	
	var home = document.getElementById("monitoringHome")
        home.appendChild(row)
}

