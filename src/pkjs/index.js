
// Initialize Clay
// Import the Clay package
var Clay = require('pebble-clay');
// Load our Clay configuration file
var clayConfig = require('./config');
// Initialize Clay
var clay = new Clay(clayConfig);

//Grab monitoring location from local storage

var settings = JSON.parse(localStorage.getItem('clay-settings'));

 



function fetchWaterData() {
  //Pull monitoring location from local storage or default if it hasn't been initialized.
  var settings = JSON.parse(localStorage.getItem('clay-settings'));
  
  if (settings) {
  var MonitoringLocation = settings['MONITORING_LOCATION']; // Access by messageKey defined in config.json
  console.log('Stored Monitoring Location is: ' + MonitoringLocation);
  } else{
  var MonitoringLocation = '05454500'
  }
  
  
  // USGS Water Services REST API URL for Gage Height (00065)
  var siteId = MonitoringLocation;
  var url = 'https://waterservices.usgs.gov/nwis/iv/?format=json&sites=' + 
            siteId + '&parameterCd=00065&period=P7D';

  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    try {
      // Parse the JSON response
      var json = JSON.parse(this.responseText);
      var allValues = json.value.timeSeries[0].values[0].value;
      
      var numPoints = Math.min(allValues.length, 96);
      
      var targetPoints = 96;
      var skipFactor = Math.ceil(allValues.length / targetPoints); // Usually 7 for a week
      var dataBuffer = [];
      
      //Convert values for more efficient sending
      for (var i = 0; i < allValues.length; i+= skipFactor) {
        
        // Convert "4.52" -> 452
          var scaledValue = parseFloat(allValues[i].value) * 10;
          
          // Ensure we don't send negative numbers or weird floats
          dataBuffer.push(Math.max(0, Math.floor(scaledValue))); 
      }
      
      //console.log('First point: ' + dataBuffer[0]);
      
      Pebble.sendAppMessage({
          'CHART_DATA': dataBuffer 
      }, function(e) {
          console.log('Successfully sent water data!');
      }, function(e) {
          console.log('Send failed: ' + JSON.stringify(e));
      });
        
        
//         // Convert "4.52" -> 4.5 -> 45 (fitting into one byte)
//         var scaledValue = Math.round(parseFloat(allValues[i].value) * 100);
//         console.log(scaledValue);
//         // Uint8 max is 255. Since the river is rarely 25ft deep, this is safe.
//         dataBuffer.push(Math.max(0, Math.floor(scaledValue)));
//       }
      
       
      
//       // Send to Pebble
//       Pebble.sendAppMessage({
//         'CHART_DATA': dataBuffer
//       });
        
        
        
        
       
    } catch (err) {
      console.log('Error parsing USGS JSON: ' + err);
    }
  };
  
  xhr.open('GET', url);
  xhr.send();
}



// Listen for when the watch requests an update
Pebble.addEventListener('appmessage', function(e) {
  console.log('AppMessage received!');
  fetchWaterData();
}
);


// Listen for when the watchface is opened
Pebble.addEventListener('ready',
  function(e) {
    console.log('PebbleKit JS ready!');

    // Get the initial weather
    fetchWaterData();
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log('AppMessage received!');
    // Check if this is a weather refresh request
    if (e.payload['REQUEST_WEATHER']) {
      fetchWaterData();
    }
  }
);

//Listen for when the setting webview is closed
Pebble.addEventListener('webviewclosed',
    function(e) {
      console.log('Webview closed');
      //fetch new data
      fetchWaterData();
      console.log('fetched new data');
    })
