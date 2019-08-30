function addGraph(names, tabName){
  $(document).ready(function() {
    var series = [];
    for (i in names) {
      series.push({
        name: names[i],
        marker: {
          enabled: true,
          radius: 5
        },
        data: (function () {
          var data3 = [];
          $.ajax({
            url: '/monitoring/data/'+names[i],
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
      })

    }
    jQuery.noConflict();
    var theme = 'default';
    (function($){ // encapsulate jQuery
      // Create the chart
      Highcharts.stockChart(tabName, {
        chart: {
          events: {
            load: function () {
              // set up the updating of the chart each second
              var series = this.series;
              setInterval(function () {
                for (i in names) {
                  var data2 = last_values[names[i]];
                  var x = data2[0], 
                  y = data2[1];

                  var last_index = series[i].data.length;
                  if (typeof x !== 'undefined') {
                    if(series[i].data[last_index-1].x !== x){
                      //if(series[i].data.length < 1000){
                        series[i].addPoint([x, y]);
                      //}
                      //else{
                      //  series[i].addPoint([x, y], true, true);
                      //}
                    }
                  }
                }
              }, 1000);
            }
          }
        },

        time: {
          useUTC: false
        },

        exporting: {
          enabled: true
        },

        plotOptions: {
          line: {
            animation: false
          }
        },

        xAxis: {
          type: 'datarate',
          dateTimeLabelFormats: { // don't display the dummy year
            month: '%e. %b',
            year: '%b'
          },
          title: {
            text: 'Time'
          },
          ordinal: false
        },
        rangeSelector: {
	  enabled :false,
	  allButtonsEnabled: false
	/*
          buttons: [{
            count: 1,
            type: 'minute',
            text: '1min'
          }, {
            count: 5,
            type: 'minute',
            text: '5min'
          }, {
            count: 10,
            type: 'minute',
            text: '10min'
          }, {
            type: 'all',
            text: 'All'
          }],
          selected: 0*/
        },
	
	navigator: {
		enabled: false
	},
        title: {
          text: tabName
        },

        exporting: {
          enabled: false
        },

        series: series

      });
    })(jQuery);


  });

}

