/* Quest 6: Rally
   BU ENG EC 444 Spring 2023
   Team 2: John Culley, Ananth Sanjay, Jeffrey Zhang, Kai Imery
   04/04/2023 
*/
const http = require('http');
var Engine = require('tingodb')(),
    assert = require('assert');
var db = new Engine.Db('./', {});
var timeTable = db.collection("timeTable");
var totalTimes = db.collection("totalTimes");

var count = 0;
var checkpointCount = 0;
var startToC1 = 0;
var C1toC2 = 0;
var C2toC3 = 0;
var C3toC4 = 0;
var startToC4 = 0;
var prevQR = 0;

function checkpointTimer(){
    checkpointCount++;
}
function totalTimer(){
    count++;
}

setInterval(checkpointTimer,1000);
setInterval(totalTimer, 1000);

//create the server on port 3000 which is listening for the HTTP Post from the python file
const server = http.createServer((req, res) => {
    // Handle incoming POST requests
    
    if (req.method === 'POST') {
        let body = '';
        req.on('data', chunk => {
            body += chunk.toString();
        });
        req.on('end', () => {
            // Parse the data sent from the Python script
            var data = JSON.parse(body);
            var data2 = JSON.parse(data);
            // Do something with the data
            console.log(data2.qrCodeData);
            if (data2.qrCodeData == 'G' && data2.qrCodeData != prevQR){
                console.log("Entered G");
                var now = new Date();
                var hours = now.getHours();
                var minutes = now.getMinutes();
                var seconds = now.getSeconds();
                var datetimeString = hours + ':' + minutes + ':' + seconds;
                var entry = { "time": datetimeString, "QRcode": data2.qrCodeData, "Checkpoint" : 0};
                timeTable.insert(entry, function (err, result) {
                     assert.equal(null, err);
                });
                prevQR = data2.qrCodeData;
                checkpointCount = 0;
                count = 0;
                startToC1 = 0;
                C1toC2 = 0;
                C2toC3 = 0;
                C3toC4 = 0;
                startToC4 = 0;
            }
            else if (data2.qrCodeData == 1 && data2.qrCodeData != prevQR){
                console.log("Entered 1");
                var now = new Date();
                var hours = now.getHours();
                var minutes = now.getMinutes();
                var seconds = now.getSeconds();
                var datetimeString = hours + ':' + minutes + ':' + seconds;
                var entry = { "time": datetimeString, "QRcode": data2.qrCodeData, "Checkpoint": data2.qrCodeData };
                timeTable.insert(entry, function (err, result) {
                    assert.equal(null, err);
                });
                prevQR = data2.qrCodeData;
                startToC1 = checkpointCount;
                checkpointCount = 0;
            }
            else if (data2.qrCodeData == 2 && data2.qrCodeData != prevQR) {
                console.log("Entered 2");
                var now = new Date();
                var hours = now.getHours();
                var minutes = now.getMinutes();
                var seconds = now.getSeconds();
                var datetimeString = hours + ':' + minutes + ':' + seconds;
                var entry = { "time": datetimeString, "QRcode": data2.qrCodeData, "Checkpoint": data2.qrCodeData };
                timeTable.insert(entry, function (err, result) {
                    assert.equal(null, err);
                });
                prevQR = data2.qrCodeData;
                C1toC2 = checkpointCount;
                checkpointCount = 0;
            }
            else if (data2.qrCodeData == 3 && data2.qrCodeData != prevQR) {
                console.log("Entered 3");
                var now = new Date();
                var hours = now.getHours();
                var minutes = now.getMinutes();
                var seconds = now.getSeconds();
                var datetimeString = hours + ':' + minutes + ':' + seconds;
                var entry = { "time": datetimeString, "QRcode": data2.qrCodeData, "Checkpoint": data2.qrCodeData };
                timeTable.insert(entry, function (err, result) {
                    assert.equal(null, err);
                });
                prevQR = data2.qrCodeData;
                C2toC3 = checkpointCount;
                checkpointCount = 0;
            }
            else if (data2.qrCodeData == 4 && data2.qrCodeData != prevQR) {
                console.log("Entered 4");
                var now = new Date();
                var hours = now.getHours();
                var minutes = now.getMinutes();
                var seconds = now.getSeconds();
                var datetimeString = hours + ':' + minutes + ':' + seconds;
                var entry = { "time": datetimeString, "QRcode": data2.qrCodeData, "Checkpoint": data2.qrCodeData };
                timeTable.insert(entry, function (err, result) {
                    assert.equal(null, err);
                });
                prevQR = data2.qrCodeData;
                C3toC4 = checkpointCount;
                startToC4 = count;
                checkpointCount = 0;
                count = 0;
                var allTimes = {"StoC1" : startToC1, "C1toC2" : C1toC2, "C2toC3" : C2toC3, "C3toC4" : C3toC4, "Total" : startToC4};
                totalTimes.insert(allTimes, function (err, result) {
                    assert.equal(null, err);
                });
            }
            else{
                console.log("waiting for record\n");
            }

            console.log(data2);
            res.end('OK');
        });
    } else {
        res.statusCode = 404;
        res.end('Not found');
    }
});

//----------------------------------------------------------------------------------------







server.listen(3000, () => {
    console.log('Server listening on port 3000');
});
