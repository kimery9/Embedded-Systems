/*6. Module to save console data to a file on your host in text or JSON
***********************************************************************************/
const SerialPort = require("serialport").SerialPort;
var fs = require('fs');
var util = require('util');
var data_file = fs.createWriteStream('\data.csv', {flags : 'w'});

console.log = function(d) { //
  data_file.write(util.format(d));
};

const serialPort = new SerialPort({
  path: "COM9",
  baudRate: 115200,
  autoOpen: false,
});

serialPort.open(function (error) {
  // if (error) {
  //   console.log("failed to open: " + error);
  // } else {
  //   console.log("serial port opened");

    serialPort.on("data", function (data) {
      // get buffered data and parse it to an utf-8 string
      // console.log(data);
      message = data.toString("utf-8");
      console.log(message);
    });

    // serialPort.on("error", function (data) {
    //   console.log("Error: " + data);
    // });
  // }
});

var http = require('http');
var url = require('url');
var fs = require('fs');

http.createServer(function (req, res) {
  var q = url.parse(req.url, true);
  var filename = "." + q.pathname;
  fs.readFile(filename, function(err, data) {
    if (err) {
      res.writeHead(404, {'Content-Type': 'text/html'});
      return res.end("404 Not Found");
    } 
    res.writeHead(200, {'Content-Type': 'text/html'});
    res.write(data);
    return res.end();
  });
}).listen(8080);