/* Quest 6: Rally
   BU ENG EC 444 Spring 2023
   Team 2: John Culley, Ananth Sanjay, Jeffrey Zhang, Kai Imery
   04/04/2023 
*/
const fs = require('fs'); // module to handle the file system
const http = require('http');
const express = require('express'); // webserver module
const bodyParser = require('body-parser'); // parse JSON encoded strings
const WebSocket = require('ws'); // WebSocket module
const app = express();
const safe = require('safe');
const debug = require('debug')('myserver'); // debug module
const UDP = require('dgram');
var Engine = require('tingodb')(),
    assert = require('assert');
var db = new Engine.Db('./', {});
var totalTimes = db.collection("totalTimes");
var timeTable = db.collection("timeTable");


var msg;

const FOR = "1", BACK = "2", RIGHT = "3", LEFT = "4", STOP = "5", LOW = "0", SLOW = "6", MED = "7", FAST = "8", RESET = "9", START = "G", FINISH = "S";
const IP_ADDR = '0.0.0.0', PORT_ADDR = 8082;
const PORT = 3333, HOSTNAME = '192.168.1.29';

let client = UDP.createSocket('udp4');

// Using Express to create a server
app.use(express.static(__dirname)); // add the directory where HTML and CSS files are
app.use(bodyParser.urlencoded({ extended: true })); // to support URL-encoded bodies, must come before routing

// Create a WebSocket server and attach it to the Express server
const wss = new WebSocket.Server({
    server: app.listen(PORT_ADDR, IP_ADDR, () => {
        const host = wss.address().address;
        const port = wss.address().port;
        const family = wss.address().family;
        debug('Express server listening at http://%s:%s %s', host, port, family);
    })
});

// var message2;
// var stmsg;
// setInterval(() => {
//     timeTable = db.collection("timeTable");
//     timeTable.find({}).sort({ _id: -1 }).limit(1).toArray(function (err, documents) {
//         assert.equal(null, err);
//         stmsg = documents;
//         console.log(documents);
//         //console.log(stmsg[0].QRcode);
//         msg = stmsg[0].QRcode;
//         console.log(msg);
//         if (msg == 'G') {
//             console.log("Start")
//             message2 = START;
//             sendMessage(message2);
//         }
//         else if (msg == 4) {
//             console.log("Finish");
//             message2 = FINISH;
//             sendMessage(message2);
//         }
//     });
// }, 1000);
var message2;
const server = http.createServer((req, res) => {
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
        if(data2.qrCodeData == 'G'){
            message2 = START;
            console.log("Start");
            sendMessage(message2);
        }
        else if(data2.qrCodeData == 4){
            message2 = FINISH;
            console.log("Finish");
            sendMessage(message2);
        }
        res.end('OK');
    });
} else {
    res.statusCode = 404;
    res.end('Not found');
}
});

// Handle WebSocket connections
wss.on('connection', (ws) => {
    console.log('WebSocket connected');

    // Handle WebSocket messages
    ws.on('message', (data) => {
        const btn = JSON.parse(data).id;
        const val = JSON.parse(data).val;

        console.log(val);

        let message;
        console.log(msg);

        switch (val) {
            case '1':
                message = FOR;
                break;
            case '2':
                message = BACK;
                break;
            case '3':
                message = RIGHT;
                break;
            case '4':
                message = LEFT;
                break;
            case '5':
                message = STOP;
                break;
            case '6':
                message = SLOW;
                break;
            case '7':
                message = MED;
                break;
            case '8':
                message = FAST;
                break;
            case '9':
                message = RESET;
                break;
            case '10':
                message = START;
                break;
            case '11':
                message = FINISH;
                break;
            default:
                message = LOW;
        }

        sendMessage(message);
    });
});

function sendMessage(message) {
    const packet = Buffer.from(message);

    client.send(packet, PORT, HOSTNAME, (err) => {
        if (err) {
            console.error('Failed to send packet!');
        } else {
            console.log('Packet sent!');
        }
    });
}

client.on('error', (err) => {
    console.log(`Error in UDP client: ${err.message}`);
});

client.on('message', (message, info) => {
    // get the information about server address, port, and size of packet received.
    console.log('Address: ', info.address, 'Port: ', info.port, 'Size: ', info.size);

    //read message from server
    console.log('Message from server', message.toString());
});


var datalog;
var jsonData;
setInterval(() => {
    totalTimes.find({}).toArray(function (err, documents) {
        assert.equal(null, err);
        datalog = documents;
        //console.log(documents);


        jsonData = JSON.stringify(datalog);
        fs.writeFile('data.json', jsonData, (err) => {
            if (err) {
                //console.log("here");
                //console.error(err);
            } else {
                console.log('JSON created')
            }
        });

    });
}, 1000);

server.listen(4000, () => {
    console.log('Server listening on port 3000');
});

