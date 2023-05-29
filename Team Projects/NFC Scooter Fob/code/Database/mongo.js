/* Quest 5: NFC Scooter Key Fob
   BU ENG EC 444 Spring 2023
   Team 2: John Culley, Ananth Sanjay, Jeffrey Zhang, Kai Imery
   04/25/2023 
*/
const { MongoClient } = require('mongodb');

const uri = "mongodb+srv://group2:smartsys@cluster0.c38elly.mongodb.net/?retryWrites=true&w=majority";
const client = new MongoClient(uri, { useNewUrlParser: true, useUnifiedTopology: true });

async function run() {
  try {
    // Connect the client to the server
    await client.connect();

    // Get a reference to the collection
    const collection = client.db("mydatabase").collection("mycollection");

    // Insert a document into the collection
    const result = await collection.insertOne({scotter_id: 1, fob_id: 1});
    console.log(`Inserted document with _id: ${result.insertedId}`);

  } catch (err) {
    console.log(err);
  } finally {
    // Close the client connection
    await client.close();
  }
}

run();
