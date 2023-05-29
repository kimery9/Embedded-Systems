/* Quest 5: NFC Scooter Key Fob
   BU ENG EC 444 Spring 2023
   Team 2: John Culley, Ananth Sanjay, Jeffrey Zhang, Kai Imery
   04/25/2023 
*/
const http = require('http');

var Engine = require('tingodb')(),
    assert = require('assert');

var db = new Engine.Db('./', {});
var data;

//creating the database
var collection = db.collection("smokedatabase");
collection.find({}).toArray(function (err, documents) {
    assert.equal(null, err);
    data = documents;
    console.log(documents);
});

const server = http.createServer(async (req, res) => {
    // Generate an HTML table from the data
    let html = "<table><thead><tr><th>key</th><th>time</th><th>id</th><th>smoke</th><th>Temp</th></tr></thead><tbody>";
    data.forEach(item => {
    html += `<tr><td>${item._id}</td><td>${item.time}</td><td>${item.id}</td><td>${item.smoke}</td><td>${item.temp}</td></tr>`;
    });
    html += "</tbody></table>";
    
    // Return the HTML table as a response
    res.setHeader('Content-Type', 'text/html');
    res.write(html);
    res.end();
});

server.listen(8080);
console.log('Server is running on port 8080...');