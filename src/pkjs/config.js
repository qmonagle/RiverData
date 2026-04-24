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
  "type": "select",
  "messageKey": "UNITS",
  "defaultValue": "ft",
  "label": "Depth Units",
  "options": [
    { 
      "label": "Meters", 
      "value": "m" 
    },
    { 
      "label": "Feet",
      "value": "ft" 
    }
  ]
},
    {
  "type": "select",
  "messageKey": "TIME_SPAN",
  "defaultValue": "P7D",
  "label": "Graph Time Span",
  "options": [
    { 
      "label": "1 Day", 
      "value": "P1D" 
    },
    { 
      "label": "3 Days",
      "value": "P3D" 
    },
    {
      "label": "1 Week",
      "value": "P7D"
    },
    {
     "label": "30 Days",
      "value": "P30D" 
    },
    {
      "label": "1 Year",
      "value": "P365D"
    }
  ]
},
  {
  "type": "submit",
  "defaultValue": "Save"
}
];