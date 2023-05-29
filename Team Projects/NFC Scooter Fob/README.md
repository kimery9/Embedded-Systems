# NFC Scooter Fob

Authors: John Culley, Kai Imery, Ananth Sanjay, Jeffery Zhang

Date: 2023-04-25

### Summary

To create our NFC scooter and fob, there were a couple of key hardware pieces required for the functionality. The “scooter” element of the design is made up of an ESP32 which will act as a receiver for the IR/TX communication. Also the scooter will have a QR code which when scanned will reveal the scooter ID. The “fob” element of the design is comprised of a RaspberryPi which is responsible for scanning the QR code, and a ESP32 that will act as the transmitter. The third component is an authentication database which is created with a Raspberry Pi. 

With all of these separate components identified, our team split into different groups to begin completing the different tasks. John and Jeffrey worked on the “Scooter” component. Ananth, John, and Jeffrey all worked together on the “Fob” component. Kai and John worked on the authentication database. After all of the separate components were constructed, we brought them back together and began testing. A couple of the key difficulties we found was communicating between the different devices over the Wifi network. It took a lot of testing to get the scanned QR code value from the python script to the node.js server, and then to the ESP32s. However, after a lot of testing, our components were finally all functioning together in unison and our project was complete. 


### Self-Assessment 

| Objective Criterion | Rating | Max Value  | 
|---------------------------------------------|:-----------:|:---------:|
| Objective One | 1 |  1     | 
| Objective Two | 1 |  1     | 
| Objective Three | 1 |  1     | 
| Objective Four | 1 |  1     | 
| Objective Five | 1 |  1     | 
| Objective Six | 1 |  1     | 
| Objective Seven | 1 |  1     | 


### Solution Design

**Scan QR Code:**

To begin the process of unlocking the scooter, the first step is to scan the QR code with the “fob”. The QR Code was printed and has a value of 20, which represents the scooter_ID. The “fob” is a Raspberry Pi which has a webcam connected. To scan the QR Code, we used a python script that rapidly captures images and scans them attempting to decode the QR Code. The scanning of the QR is done with an external library, zbarlight, which allows for an image to be decoded using the defined keywords. Upon a successful scan, the python script stores the value and uses an HTTP Post to transfer the data over to the node.js server. The HTTP Post works by establishing a connection to the node.js server and sending an HTTP Post request. The request includes the request line, headers, and the QR code data. The request line specifies the method as POST, and the URL of the resource that will handle the request. The URL in our python file is defined as the node.js server IP Address. The headers contain the content type and length of the message body. The message body contains the QR code data which is sent using JSON format. Within our node.js server, it must act as the server because the python file acts as the client. The node.js server accepts the HTTP Post, and processes the information, storing the QR code value into a variable. 

<img width="874" alt="Screen Shot 2023-04-25 at 11 45 23 PM" src="https://user-images.githubusercontent.com/91488781/234464852-34cd68f3-9c25-4be5-b98a-924178451ccf.png">

Authenticate the QR Code and Log the Instance:

For this specific instance, there are two tables within the database: authentication and log. With the new QR code value now properly accepted within the node.js server, the first step is to log the instance. A time variable is created by accessing the internal clock within JavaScript, and the time, scooterID(QR Code), and fobID are inserted as a new record into the log table within the database. The other crucial step to the design is the authentication. To ensure that the Fob should be allowed to access the specific scooter, the QR code value and the fob ID are used as parameters for a query into the authentication table within the database. The authentication table stores all of the legal pairings between the scooters and fobs, so if an object is returned from the query, that means that the fob is allowed to unlock the scooter. There is a specific key also attached to each record within the authentication table, so when the record is returned the key is the value that will be sent to the ESP32s which control the mechanical aspect of the fob and scooter. 
<img width="397" alt="Screen Shot 2023-04-25 at 11 46 33 PM" src="https://user-images.githubusercontent.com/91488781/234464964-e8afe0bc-cf78-45b7-b106-941dc653cb86.png">


**Send Key to Fob and Scooter:**

With the newly acquired key within the node.js server, the next step is to use UDP to send the key to both the scooter ESP32 and the fob ESP32. In this instance, the node.js server acts as the client and the ESP32s act as the server. Using the UDP protocol, the node.js server sends the key to both of the ESP32s. Within the code for both of the ESP32s, there is the appropriate server code for the scooter and fob to properly receive the data from the node.js server. The key value that is sent is then stored within a ‘verification_key’ variable that is an integer. 

**Use IR/TX to Unlock the Scooter with the Fob:**

Now that both the scooter and the fob have received the same key value, they can use IR/TX to confirm with each other that their keys are the same, and unlock the scooter. The first step is on the transmitter (fob) ESP32. Now that the fob has the correct key, the next step is to send that key to the scooter using IR/TX. Using IR/TX protocol, the fob sends the key value to the scooter. The receiver (scooter) ESP32 uses the corresponding IR/TX protocol to receive the data from the fob. The scooter then processes the value it receives and stores it as an integer in a variable “key”. The value received via IR/TX is then compared to the value that was received via the node.js server and UDP, and if the values are the same, that means that the scooter should unlock. To accomplish this we compared the values and if they were equivalent, the green LED was activated and the red LED was turned off, signifying that the scooter was unlocked. 
<img width="832" alt="Screen Shot 2023-04-25 at 11 44 01 PM" src="https://user-images.githubusercontent.com/91488781/234464673-ba478c8f-08ef-43b5-96e4-485309c4d8ec.png">


**Displaying the Log Table on a Web Portal:**

To display the log of unlock attempts, the first step is to access all of the records from the MongoDB database table: log. To query these results, the node.js function ‘find’ is used to extract all records and they are appropriately stored into a variable called ‘datalog’. Next, a string of HTML is created with the headers: time, scooterID, and fobID. Then a ‘foreach’ function is utilized to iterate through all of the records within the JSON Object that is storing all of the records from the database. After the string containing the HTML is finished, it is then rendered onto port 8080 where we could then see the complete record of unlock attempts. 


### Sketches/Diagrams
<img width="866" alt="Screen Shot 2023-04-25 at 11 45 54 PM" src="https://user-images.githubusercontent.com/91488781/234464899-70dbbe24-0bc5-47ab-ac41-fb0d3966db91.png">
Circuit Diagram
<img width="837" alt="Screen Shot 2023-04-25 at 11 46 14 PM" src="https://user-images.githubusercontent.com/91488781/234464934-a54700cf-b5d1-45ac-9119-aa798b1625df.png">
Flow Chart
</p>



### Supporting Artifacts
- [Link to video technical presentation](https://drive.google.com/file/d/1WD8spxYwRmpdJWVq5QAtUW4-Yg_30M_z/view?usp=sharing). Not to exceed 120s
- [Link to video demo](https://drive.google.com/file/d/1DBGTXO5_bLmHzAh_bq64LmH5OjxAZD0C/view?usp=sharing). Not to exceed 120s


### Modules, Tools, Source Used Including Attribution
Visual Studio Code, IR/TX, Ajax, HTTP Post, Raspberry PI, ESP32, ESP-IDF, UDP, node.js, TingoDB, Pi Webcam

### References
No references. 

