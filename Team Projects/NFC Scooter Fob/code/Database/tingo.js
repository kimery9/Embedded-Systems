/* Quest 5: NFC Scooter Key Fob
   BU ENG EC 444 Spring 2023
   Team 2: John Culley, Ananth Sanjay, Jeffrey Zhang, Kai Imery
   04/25/2023 
*/
var Engine = require('tingodb')(),
    assert = require('assert');

var db = new Engine.Db('./', {});


const fs = require('fs');

// Read the contents of the text file
const data = fs.readFileSync('smoke.txt', 'utf8');

// Split the text into rows and remove the header row
const rows = data.trim().split('\n').slice(1);

// Map each row to an object with property names corresponding to the header
const objects = rows.map(row => {
  const [time, id, smoke, temp] = row.trim().split('\t');
  return { time: parseInt(time), id: parseInt(id), smoke: parseInt(smoke), temp: parseFloat(temp) };
});


//console.log(objects);



//creating the database
var collection = db.collection("smokedatabase");



//inserting into the database
collection.insert(objects, function(err, result) {
  assert.equal(null, err);

  
});

collection.find({}).toArray(function (err, documents) {
  assert.equal(null, err);

  console.log(documents);
});