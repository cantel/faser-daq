function addRateGraph(dataName,tabName){
    chart=Highcharts.stockChart(tabName, {
	time: {
            useUTC: false //FIXME: eventually should probable use UTC
	},

	rangeSelector: {
            buttons: [{
		count: 1,
		type: 'minute',
		text: '1M'
            }, {
		count: 5,
		type: 'minute',
		text: '5M'
            }, {
		count: 30,
		type: 'minute',
		text: '30M'
            }],
            inputEnabled: false,
            selected: 0
	},

	exporting: {
            enabled: false
	},

	series: [{
            name: dataName,
            data: (function () {
		var data3 = [];
		$.ajax({
		    url: '/monitoring/data/'+dataName,
		    async: false,
		    dataType: 'json',
		    success: function (json) {   
			assignVariable(json);
		    }
		});
		
		function assignVariable(data) {
		    data3 = data;
		}
		return data3;
            }())
	}]
    });
    return chart;
}

function addRates() {
    // Create the charts
    var rateTypes=["Physics","Calibration","TLBMonitoring"];
    var series=[];
    var graphs=[];
    for (ii = 0; ii<rateTypes.length; ii++) {
	graphs.push(addRateGraph('History:eventbuilder01_Event_rate_'+rateTypes[ii],rateTypes[ii]+'RateGraph'));
	series.push(graphs[ii].series[0]);
    }
    const source = new EventSource("/monitoring/lastRates/1.0");
    source.onmessage = function (event) {
	const data = JSON.parse(event.data);
	for (ii = 0; ii<rateTypes.length; ii++) {
	    if ('Event_rate_'+rateTypes[ii] in data) {
		series[ii].addPoint(data['Event_rate_'+rateTypes[ii]],true,false);
		document.getElementById(rateTypes[ii]+"Rate").innerHTML=data['Event_rate_'+rateTypes[ii]][1];
	    }
	    if ('Events_received_'+rateTypes[ii] in data) {
		document.getElementById(rateTypes[ii]+"Events").innerHTML=data['Events_received_'+rateTypes[ii]][1];
	    }
	}
    };
}
