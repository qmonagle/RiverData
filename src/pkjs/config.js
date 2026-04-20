module.exports =
[
  { 
    "type": "heading", 
    "defaultValue": "River Data Configuration" 
  }, 
  
  {
  "type": "input",
  "messageKey": "MONITORING_LOCATION",
  "defaultValue": "05454500",
  "label": "Enter the ID of your preferred USGS monitoring location. Location must provide gage height data. Please double check the ID if your graph doesn't update or shows up blank.",
  "attributes": {
    "placeholder": "eg: 05454500",
    "limit": 10,
    "type": "number"
  }
  }, 
  {
  "type": "toggle",
  "messageKey": "UNITS",
  "label": "Unit",
  "defaultValue": true
}
];