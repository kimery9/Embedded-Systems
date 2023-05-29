// Quest 3: Cat Tracker
// BU ENG EC 444 Spring 2023
// Team 2: John Culley, Ananth Sanjay, Jeffrey Zhang, Kai Imery
// 03/21/2023 

// Required module
var dgram = require('dgram');

// Port and IP
var PORT = 3333;
var HOST = '192.168.1.15';

// Create socket
var server = dgram.createSocket('udp4');  

// Create server that listens on a port
server.on('listening', function () {
    var address = server.address();
    console.log('UDP Server listening on ' + address.address + ":" + address.port);
});

// On connection, print out received message
server.on('message', function (message, remote) {
    console.log(remote.address + ':' + remote.port +' - ' + message);
    WriteData(message);
    // Send Ok acknowledgement
    server.send("Received!",remote.port,remote.address,function(error){
      if(error){
        console.log('MEH!');
      }
      else{
        console.log('Sent: Received!');
      }
    });

});

// Bind server to port and IP
server.bind(PORT, HOST);

function WriteData(intemp) {
  var time = new Date();
  time.setHours(time.getHours() - 4); // Convert GMT to EST
  time = Date.parse(time);
  // Requiring fs module
  const fs = require("fs");
  // Storing the JSON format data in myObject
  var data = fs.readFileSync("data.json");
  var myObject = JSON.parse(data);
  // Defining new data to be added
  var newData = [time, parseFloat(intemp.toString("utf-8"))];
  // Adding the new data to our object
  myObject.datapoint.push(newData);
  // Writing to our JSON file
  data = JSON.stringify(myObject);
  fs.writeFile("data.json", data,(err)=>err&&console.error(err));
};