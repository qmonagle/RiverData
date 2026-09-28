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
//var timeSpan = 'P1D';
var timeSpanInt = 0;
var allValues = 0;

function parseJSON(){
  var numDays = 1;

  //convert timespan 'enum' to number of days
  if (timeSpanInt === 0){
    numDays = 1;
  } else if (timeSpanInt === 1){
    numDays = 3;
  } else if (timeSpanInt === 2){
    numDays = 7;
  } else if (timeSpanInt === 3){
    numDays = 30;
  } else if (timeSpanInt === 4){
    numDays = 365;
  } else{
    numDays = 1;
  }

  //console.log("Number of days: ", numDays);

  var numPoints = Math.min(allValues.length, 96);
  //console.log("number of points", numPoints);

  //set up variable for starting point in value array
  //Need to only use only most recent number of samples based on timespan
  //i.e. 1 day = most recent 96 samples out of full 365 days of samples.
  var sampleStart = (allValues.length - (numDays * 96)); //15 min increment = 96 sampes/day  


  var targetPoints = 96;
  var skipFactor = Math.ceil((allValues.length - sampleStart) / targetPoints); // Usually 7 for a week
  var dataBuffer = [];
  var dataBuffer16 = [];

  //set up variable for starting point in value array
  //Need to only use only most recent number of samples based on timespan
  //i.e. 1 day = most recent 96 samples out of full 365 days of samples.
  //include exception for numDays == 365.
  if (numDays == 365){
    var sampleStart = 0;
  }else{
    var sampleStart = (allValues.length - (numDays * 96)); //15 min increment = 96 sampes/day 
  }
  
  console.log("SampleStart value: ", sampleStart);
  //Convert values for more efficient sending
  for (var i = sampleStart; i < allValues.length; i+= skipFactor) {

    // Convert "4.52" -> 452
    var scaledValue = parseFloat(allValues[i].value) * 100;
    //console.log('value: ', scaledValue);


    // Ensure we don't send negative numbers or weird floats
    dataBuffer.push(Math.max(0, Math.floor(scaledValue)));    
  }
  var dataBuffertemp = [];
  //convert data from 100 x feet to 100 x meters
  if (units == 'm'){
    for (var i = 0; i <dataBuffer.length; i++){
      //round data, need to send an int
      dataBuffer[i] = Math.round(dataBuffer[i] / 3.281);
      //console.log('converted value: ' + dataBuffer[i]);
    }
  }


  //convert 16 bit values into two 8 bit values
  var j = 0;
  for (var i = 0; i < (dataBuffer.length * 2); i+= 2){
    dataBuffer16[i] = dataBuffer[j] >> 8;
    dataBuffer16[i+1] = dataBuffer[j] & 0xff;
    j++;
  }



  Pebble.sendAppMessage({
    'CHART_DATA': dataBuffer16, 
    'UNITS' : unitsEnum
  }, function(e) {
    console.log('Successfully sent water data!');
  }, function(e) {
    console.log('Send failed: ' + JSON.stringify(e));
  });
  
  
}


function fetchWaterData(fetch) {
  //Pull monitoring location from local storage or default if it hasn't been initialized.
  var settings = JSON.parse(localStorage.getItem('clay-settings'));
  
  
  if (settings) {
  MonitoringLocation = settings['MONITORING_LOCATION']; // Access by messageKey defined in config.json
  console.log('Stored Monitoring Location is: ' + MonitoringLocation);
  //pull user selected units
  units = settings['UNITS'];
  //timeSpan = settings['TIME_SPAN'];
  //console.log('Stored units are: ' + units);
  } else{
    MonitoringLocation = '05454500'
    units = 'ft';
    unitsEnum = 0;
    //timeSpan = 'P7D';
  }
  
  if (units == 'ft'){
    unitsEnum = 0;
  } else if (units == 'm'){
    unitsEnum = 1;
  } else{
    unitsEnum = 0;
  }
  //console.log('Stored units are: ' + unitsEnum);
  //console.log('time span is:' + timeSpanInt);
  
  

  // USGS Water Services REST API URL for Gage Height (00065)
  var siteId = MonitoringLocation;
  //var API_timeSpan = timeSpan;
  var url = 'https://waterservices.usgs.gov/nwis/iv/?format=json&sites=' + 
            siteId + '&parameterCd=00065&period=P365D';
  var xhr = new XMLHttpRequest();
  console.log("request sent");
    xhr.onload = function () {
      try {
        // Parse the JSON response
        var json = JSON.parse(this.responseText);
        allValues = json.value.timeSeries[0].values[0].value;
        //console.log("number of values", (allValues.length));
        parseJSON();
    

        
        } catch (err) {
          console.log('Error parsing USGS JSON: ' + err);
        
          Pebble.sendAppMessage({
            'REQUEST_ERROR': 1
          }, function(e) {
            console.log('Successfully sent error');
          }, function(e) {
            console.log('Error send failed');
          });
        }
        
      }
    xhr.open('GET', url);
    xhr.send();
}  




// Listen for when the watchface is opened
Pebble.addEventListener('ready', function(e) {
    console.log('PebbleKit JS ready!');

    // Get the initial data
    fetchWaterData();
  }
);

Pebble.addEventListener('appmessage', function(e) {
  console.log('Button press recieved')
  var dict = e.payload;
  
    if(dict['TIME_SPAN'] != undefined) {
    // The RequestData key is present, read the value
    timeSpanInt = dict['TIME_SPAN'];
    parseJSON();
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
    }
);
