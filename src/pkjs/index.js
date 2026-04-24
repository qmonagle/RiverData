// Initialize Clay
// Import the Clay package
var Clay = require('@rebble/clay');
// Load our Clay configuration file
var clayConfig = require('./config');
// Initialize Clay
var clay = new Clay(clayConfig);

var MonitoringLocation = '05454500'
var units = 'ft';
var unitsEnum = 0;
var timeSpan = 'P7D';
var timeSpanInt = 7;


function fetchWaterData() {
  //Pull monitoring location from local storage or default if it hasn't been initialized.
  var settings = JSON.parse(localStorage.getItem('clay-settings'));
  
  
  if (settings) {
  MonitoringLocation = settings['MONITORING_LOCATION']; // Access by messageKey defined in config.json
  console.log('Stored Monitoring Location is: ' + MonitoringLocation);
  //pull user selected units
  units = settings['UNITS'];
  timeSpan = settings['TIME_SPAN'];
  //console.log('Stored units are: ' + units);
  } else{
    MonitoringLocation = '05454500'
    units = 'ft';
    unitsEnum = 0;
    timeSpan = 'P7D';
  }
  
  if (units == 'ft'){
    unitsEnum = 0;
  } else if (units == 'm'){
    unitsEnum = 1;
  } else{
    unitsEnum = 0;
  }
  console.log('Stored units are: ' + unitsEnum);
  
  
  
  
  // USGS Water Services REST API URL for Gage Height (00065)
  var siteId = MonitoringLocation;
  var API_timeSpan = timeSpan;
  var url = 'https://waterservices.usgs.gov/nwis/iv/?format=json&sites=' + 
            siteId + '&parameterCd=00065&period=' + API_timeSpan;

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
      var dataBuffer16 = [];
      
      //Convert values for more efficient sending
      for (var i = 0; i < allValues.length; i+= skipFactor) {
        
        // Convert "4.52" -> 452
          var scaledValue = parseFloat(allValues[i].value) * 100;
          
         
          // Ensure we don't send negative numbers or weird floats
          dataBuffer.push(Math.max(0, Math.floor(scaledValue)));    
      }
      var dataBuffertemp = [];
      //convert data from 100 x feet to 100 x meters
      if (units == 'm'){
        for (var i = 0; i <dataBuffer.length; i++){
          //round data, need to send an int
          dataBuffer[i] = Math.round(dataBuffer[i] / 3.281);
          console.log('converted value: ' + dataBuffer[i]);
        }
      }
      
      
      //convert 16 bit values into two 8 bit values
      var j = 0;
      for (var i = 0; i < (dataBuffer.length * 2); i+= 2){
        dataBuffer16[i] = dataBuffer[j] >> 8;
        dataBuffer16[i+1] = dataBuffer[j] & 0xff;
        j++;
      }
      
     
      //Convert time span into enumeration that can be sent to watch
      //1 day = 1; 3 days = 3; 1 week = 7; 1 year = 52
      if (timeSpan === 'P1D'){
        timeSpanInt = 1;
      } else if(timeSpan === 'P3D'){
        timeSpanInt = 3;
      } else if(timeSpan === 'P7D'){
        timeSpanInt = 7;
      } else if (timeSpan ==='P30D'){
        timeSpanInt = 30;
      } else if (timeSpan ==='P365D'){
        timeSpanInt = 36;
      } else{
        timeSpanInt = 7;
      }
      console.log('Time span JS: ' + timeSpan);
      console.log('Time span Int JS: ' + timeSpanInt);
      
      Pebble.sendAppMessage({
          'CHART_DATA': dataBuffer16, 
          'UNITS' : unitsEnum,
          'TIME_SPAN' : timeSpanInt
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



// Listen for when the watchface is opened
Pebble.addEventListener('ready',
  function(e) {
    console.log('PebbleKit JS ready!');

    // Get the initial data
    fetchWaterData();
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
