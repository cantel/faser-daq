function updateTableOfValues(data){
    var tableBody = document.getElementById("table-body");
    //console.log("values", data.values);
    for(var i = 0; i < data.length; i++){
	var valID = document.getElementById(data[i].key);
	if ((tableBody==null) && (valID==null)) continue;
	if (valID==null) {
	    var tr = document.createElement("tr");
	    var td1 = document.createElement("td");
	    td1.innerHTML=data[i].key;
	    var valID = document.createElement("td");
	    valID.id=data[i].key;
	    var td3 = document.createElement("td");
	    td3.id=data[i].key+"-time";
	    tr.appendChild(td1);
	    tr.appendChild(valID);
	    tr.appendChild(td3);
	    tableBody.appendChild(tr);
	}
	valID.innerHTML = data[i].value;
	timeID= document.getElementById(data[i].key+"-time");
	if (timeID!=null)
	    timeID.innerHTML = data[i].time;
    }
}

function addInfoTable(source) {
    const eventsource = new EventSource("/monitoring/infoTable/" +source);
    eventsource.onmessage = function (event) {
	const data = JSON.parse(event.data);
	console.log("got data from: "+source)
	updateTableOfValues(data);
    }
}
