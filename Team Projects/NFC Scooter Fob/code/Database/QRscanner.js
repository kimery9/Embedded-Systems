/* Quest 5: NFC Scooter Key Fob
   BU ENG EC 444 Spring 2023
   Team 2: John Culley, Ananth Sanjay, Jeffrey Zhang, Kai Imery
   04/25/2023 
*/
const http = require('http');

const hostname = '192.168.1.15';
const port = 8081;

const dgram = require('dgram');

const client = dgram.createSocket('udp4');
const serverPort = 3000; // the port number that the server is listening on
const serverHost1 = '192.168.1.38'; // the receiver / scooter
const serverHost2 = '192.168.1.12'; // the trasmiter / fob

var QR;

var Engine = require('tingodb')(),
    assert = require('assert');

var db = new Engine.Db('./', {});
var datalog;

var collection = db.collection("authenticate");
var objects = { "scooterID": 20, "fobID": 5, "key" : 30 };
collection.insert(objects, function (err, result) {
    assert.equal(null, err);
});

var collection2 = db.collection("log");

const server = http.createServer((req, res) => {
    if (req.method === 'POST' && req.url === '/receive_qr_code_data') {
        let data = '';
        req.on('data', chunk => {
            data += chunk.toString();
        });
        req.on('end', () => {
            try {
                const jsonData = JSON.parse(data);
                const qrCodeData = jsonData.qrCodeData;
                QR = qrCodeData;
                console.log('Received QR code data:', qrCodeData);
                // Do something with the QR code data
                res.statusCode = 200;
                res.setHeader('Content-Type', 'text/plain');
                res.end('QR code data received successfully\n');
                var now = new Date();
                var year = now.getFullYear();
                var month = now.getMonth() + 1; // add 1 because getMonth() returns 0-based index
                var day = now.getDate();
                var hours = now.getHours();
                var minutes = now.getMinutes();
                var seconds = now.getSeconds();

                var datetimeString = year + '-' + month + '-' + day + ' ' + hours + ':' + minutes + ':' + seconds;
                var entry = { "time": datetimeString, "scooterID": QR, "fobID": 5 };
                collection2.insert(entry, function (err, result) {
                    assert.equal(null, err);
                });

                var correctRecord;
                collection.find({ scooterID: QR }).toArray(function (err, documents) {
                    assert.equal(null, err);
                    correctRecord = documents;
                    console.log(documents);
                });
                var message;
                if (correctRecord != null) {
                 message = correctRecord[0].key;
                }

                client.send(message, serverPort, serverHost1, (err) => {
                    if (err) {
                        console.log('Error sending message:', err);
                    } else {
                        console.log(`Message "${message}" sent to server at ${serverHost}:${serverPort}`);
                    }
                    client.close();
                });


                client.send(message, serverPort, serverHost2, (err) => {
                    if (err) {
                        console.log('Error sending message:', err);
                    } else {
                        console.log(`Message "${message}" sent to server at ${serverHost}:${serverPort}`);
                    }
                    client.close();
                });


            } catch (err) {
                console.error(err);
                res.statusCode = 400;
                res.setHeader('Content-Type', 'text/plain');
                res.end('Error: Invalid JSON data received\n');
            }
        });
    } else {
        res.statusCode = 404;
        res.setHeader('Content-Type', 'text/plain');
        res.end('Error: Invalid endpoint\n');
    }
});

server.listen(port, hostname, () => {
    console.log(`Server running at http://${hostname}:${port}/`);
});



//---------------------------------------------------------------------------------







//----------------------------------------------------------------------------------------------


collection2.find({}).toArray(function (err, documents) {
    assert.equal(null, err);
    datalog = documents;
    console.log(documents);
});


//HTML server to display the log data
const server2 = http.createServer(async (req, res) => {
    // Generate an HTML table from the data
    let html = "<table><thead><tr><th>key</th><th>time</th><th>id</th><th>smoke</th><th>Temp</th></tr></thead><tbody>";
    datalog.forEach(item => {
        html += `<tr><td>${item._id}</td><td>${item.time}</td><td>${item.id}</td><td>${item.smoke}</td><td>${item.temp}</td></tr>`;
    });
    html += "</tbody></table>";

    // Return the HTML table as a response
    res.setHeader('Content-Type', 'text/html');
    res.write(html);
    res.end();
});

server2.listen(8080);
console.log('Server is running on port 8080...');
