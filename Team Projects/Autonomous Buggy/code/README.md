# Code Readme

- HTML Folder:

In this folder we create index.html, server.js, timeDB.js, and client.js for the implementation of the client webpage and node servers. The index.html file provides the structure to the webpage as it displays the instructions of how to control the buggy, the live database of previous time trials, and the livestream. The database is read by using an AJAX call to recieve the data.json file that the database is copied into in the server file. It is then read and structured for easy display. The livestream is implemented by passing it to another port on the device and forwarding the connection into the page. The client.js file's main purpose was to handle the keypresses as well as establish and send the correct websocket messages. By using a switch statement and a on keydown function we were able to read the key being pressed by the user and send the desired message to the server.js file for further communcation with the ESP. In the server.js file we open a UDP socker for communication with the ESP, as well as creating he server implementation of the websocket. We then use a http post request to create the server and check the if the QRscanner is reading 'G' or '4' in order to send the start or finish message to the buggy. Also the server.js file takes the inputs from the Websocket and passes them into the UDP client allowing for the ESP to have access to the inputs of the user. Finally the file uses a setInterval function to repeatedly call the database and write it into a data.json file allowing for the display page to be updated in realtime when the new data entries occur. The timeDB.js file creates an HTTP server  and listens for incoming POST requests from a the QR reader Python script. The script uses the TingoDB library to create and interact with two collections: "timeTable" and "totalTimes". It also sets up several variables to track time and checkpoint counts.When a POST request is received, the script reads the data and parses it as JSON. Depending on the data received, the script will insert a new document into the "timeTable" collection with a timestamp, QR code, and checkpoint value. The script will also update the various time and checkpoint count variables depending on the QR code value. When the script receives a QR code of 4, it will calculate the total time it took to complete the route and insert a document into the "totalTimes" collection with the various time values.

- LiveFeed Folder:

In this folder, we have a liveFeed.py file which is used to create a livestream with the Pi camera. It is posted to port 8000. This file must be run with python 3.

- QR Folder:

In this folder, we have a scanQRpy.py file which is used to scan QR codes and send the data to ports 3000 and 4000. This file is run with python.

- ESP32 Folder:

Under ./main folder, the file mcpwm_servo_control_example_main.c is the file that used to control the buggy. There are 5 modules in this file: PID speed control, 7 segment alphabet display, UDP_server, Timer, and emergency obstacle avoidance.
