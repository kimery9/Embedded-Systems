# Code Readme

The code is divided into three folders in coordination with the processes needed to be achieved, the remote LED control, the tempature readinds from the ESP32, and the HTML portal for the front end. 
- HTML portal folder:
  - index.html: This file creates the html layout of the page as it initalizing the remote button, and displays the graphs. The file uses an embedded script to preform an AJAX call in order to pull the data from the data.json file. Once the AJAX promise if fulfilled the data is passed into the processData function where it is placed into a brush graph from ApexCharts.js in accordance with its documentation. The brush graph allows for scrolling and past data to be easily accessed and viewed. To impliment the real time data aspect we call the updateSeries function to refresh the AJAX call and update the graph data accordingly. 
  - data.json: This file save the data that received from ESP32 throuth node read_data.js.
  - read_data.js: This file is a UDP server that going to receive the temperature data from ESP32 and format to JSON formats and save into data.json file.
  - server.js: This file creates a node server and is used to make the port of index.html 8082, and is also receiving the HTTP Request
  - client.js: This file uses an HTTP Request which is used to retrieve something from a client, in this case it was used to obtain the value of the LED status
- ESP32 LED folder:
  - ./main/udp_server.c: This is ESP32 code that make the ESP32 as a UDP server to receive the message from the client that going to control the LED through GPIO.
- ESP32 Temp folder:
  - /main/udp_client.c: This is ESP32 code that make the ESP32 as a UDP client to read the temperature through ADC and sent it to the UDP server.
  
