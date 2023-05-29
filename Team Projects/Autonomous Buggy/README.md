# Rally Car

Authors: John Culley, Kai Imery, Ananth Sanjay, Jeffrey Zhang

Date: 2023-04-29

### Summary

To create our Rally Car there were a few crucial components: the buggy control with an ESP32, the QR scanner with a Raspberry Pi, the live stream with a Raspberry Pi, a node.js server responsible for managing the database, a node.js server responsible for communicating with the ESP32, and a front end which has the display. Jeffrey was in charge of the buggy control with the ESP32. John was in charge of the QR scanner, the live stream, and the database. Kai and Ananth were responsible for the front end and the communication with the ESP32. Because of the clear division of tasks, each component was able to be completely tested individually to ensure that they worked properly. One key issue was figuring out how to get the live stream onto an HTML front end page. Another issue was figuring out how to communicate with a constant stream of data between the python file for QR scanning and the node.js server for processing the QR scan and storing it in the database. When these issues were solved, we combined all of the components of the vehicle and began testing. There were very little issues when combining all of the components and the Rally Car was completing all necessary functionalities. 


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

**QR Scanner to Node.js Server (Database):** 
The QR Scanner is a python file that is run within one of the two Raspberry Pi’s. The python file works by first instantiating the camera and taking a JPEG image every half a second. For each JPEG image that is captured, zbarlight python package has a QR scanning function which scans the JPEG to try to decode the image. When the QR scanner doesn’t scan a QR code it sends a JSON object containing a 0 to the node.js server using an HTTP Post. When the QR scanner successfully scans a code it stores the variable in a JSON object and sends it to the node.js server using an HTTP Post. When the node.js server responsible for the database receives the JSON object it processes it. There are two different tables within the database that are used for this portion of the Rally Car, the first is ‘timeTable’ which is responsible for logging when a QR code is scanned as part of the course. If the QR code sent is ‘G’, 1, 2, 3, or 4 it will create a JSON object with the current time, QR code, and checkpoint. This JSON object is then inserted into ‘timeTable’ only the first time that it scans each lap. This avoids creating redundant records within ‘timeTable’. The other table is called ‘totalTimes’. This database is used to display our times on the front end. The node.js server starts a checkpoint timer and a total timer when it first scans ‘G’. The checkpoint timer stores its value within variables for the time between each checkpoint. So the time between ‘G’ and 1, 1 and 2, 2 and 3, and 3 and 4 are all stored in variables. When 4 is scanned, all of the timers stop and a JSON object is created with 5 different times: ‘G’ to 1, 1 to 2, 2 to 3, 3 to 4, and total time. All of these values are stored as seconds. At this point the JSON object is inserted into ‘totalTimes’. ‘totalTimes’ is later accessed to display on the front end page and updates live as a route is completed. 
<img width="727" alt="Screen Shot 2023-05-03 at 1 20 31 AM" src="https://user-images.githubusercontent.com/91488781/235837641-4b8080b2-ffe9-4d0f-bf3d-9f5896ba1bc8.png">


**QR Scanner to Node.js Server (Start):** 
The QR scanner is the same file explained above, but there is a second node.js server that the HTTP Post sends to. The other node.js server is responsible for determining which message needs to be sent to the ESP32. The reason that the QR code being scanned must be sent to this node.js server is because to start the race, there is a QR code that scans ‘G’ which should start the timer on the ESP32 and start the race. Additionally, the file looks for 4 which stops the ESP32 timer. The node.js server does this by receiving the QR data from the python file, and if it is equal to ‘G’, a start message is sent to the ESP32 initiating the timer. If the QR data from the python is 4, a finish message is sent to the ESP32 to stop the timer. The method that was chosen to send the data to the ESP32 was websockets. The benefit of websockets is that messages are sent constantly to the ESP32 which is able to combat some of the delay that is associated with other protocols such as UDP. Websockets are explained in more detail in the section below labeled ‘Front End to Node.js Server’. 


**Front End to Node.js Server:**
The front end to node connection is implimented in the form of continous Websockets. The front end asks as the client and the server as the server. With this implementation we pass the keyboard inputs by sending diffrent values for each key press. We used Q to stop the buggy, WASD to control the direction, and 1,2,3 for the speed controls. The Websockets sends message-based data, similar to UDP. WebSocket uses HTTP as the initial transport mechanism, but keeps the TCP connection alive after the HTTP response is received so that it can be used for sending messages between client and server, allowing for "real-time" data to be sent.
<img width="828" alt="Screen Shot 2023-05-03 at 1 19 44 AM" src="https://user-images.githubusercontent.com/91488781/235837567-30682c84-990f-4a11-aa56-551736def7a4.png">

**Node.js Server to ESP32:** 
 The Node.js server communicates with an ESP32 microcontroller over UDP protocol. The server uses the dgram module to create a UDP socket client that sends messages to the ESP32. The sendMessage() function constructs a UDP packet containing the message to be sent and sends it to the ESP32's IP address and port using the client.send() method. The WebSocket module is used to handle WebSocket connections, and the http and express modules are used to create a web server that serves static files and listens for POST requests
<img width="340" alt="Screen Shot 2023-05-03 at 1 21 47 AM" src="https://user-images.githubusercontent.com/91488781/235837777-c11f7500-3520-4e1a-98e0-0776042eca67.png">

**Live Stream to Front End:** 
The live stream is recorded with a second Raspberry Pi and camera. Two Raspberry Pi’s are necessary because only one instance of a camera can be used at one time and the other Pi is being used for QR code scanning. The live stream is created within a python file on the Raspberry Pi by using motion JPEG. This means that the pi camera captures 24 JPEG images a second and uses motion to put them into a video stream. This video stream is then posted to port 8000. Within the front end, this port is able to be instantiated and retrieve the live feed from the Raspberry Pi. 


**Buggy Control:** 
Our Buggy implements numerous features. First, it acts as a UDP server that is always listening for commands from Nodejs, and once it receives them, it performs the appropriate actions, such as moving forward, moving backward, spinning the wheel, and changing the speed. In addition, we use a timer that will start working once we receive the QR code message with a "G" to record our race time and display it on the 7 segment alphabet display. It was also given some "autopilot" skills. The first is the speed retention system. We use a Photo Sensor to get the speed of the wheels to calculate the speed of Buggy and use PID control to maintain the set speed so that Buggy can adapt to any road surface. Secondly, we have a collision avoidance system. We have a front radar sensor for distance measurement. Once there is an obstacle in front (distance less than 30 cm), it will trigger emergency obstacle avoidance and make Buggy stop and back up automatically until the obstacle in front disappears (distance more than 50 cm).

<img width="676" alt="Screen Shot 2023-05-03 at 1 20 51 AM" src="https://user-images.githubusercontent.com/91488781/235837670-57d3a9f7-3b82-4fe7-a1a2-1233d6cf1bb8.png">


<img width="628" alt="Screen Shot 2023-05-03 at 1 23 21 AM" src="https://user-images.githubusercontent.com/91488781/235837937-5a18785d-8a1e-42aa-a98d-37e17a220bdb.png">


### Supporting Artifacts
- [Link to video technical presentation](https://drive.google.com/file/d/1oelqTPgsCMk4U8ZUj-nTvE2k9dPWyIjL/view?usp=sharing). Not to exceed 120s
- [Link to video demo](https://drive.google.com/file/d/1C2pgzzbGSP4ygIYKwfRgdq_ZJaGhokn0/view?usp=sharing). Not to exceed 120s


### Modules, Tools, Source Used Including Attribution
ESP32, Raspberry Pi, Buggy, Lidar, Alphanumeric Display, Pi Camera, zbarlight, node.js, tingoDB, AJAX, HTTP Posts, Web Sockets, JSON

### References
No references. 

