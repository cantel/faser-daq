function updateHistogram(id, histName) {
    // Create the charts
    const source = new EventSource("/histogramming/"+id+"/"+histName+"/lastValues/5.0");
    source.onmessage = function (event) {
	const histdata = JSON.parse(event.data);
        Plotly.react(id, histdata);
    };
}
