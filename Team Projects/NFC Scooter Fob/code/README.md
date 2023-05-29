# Code Readme

The code is divided into four folders, coordinated with the processes to be implemented, namely the ESP32 Scooter, the ESP32 Fob, the database and the QR code scanner.
- QR folder:
  - QR.py: This file running a python code to use the pi camera to scan the QR code and then use HTTP request send to the nodejs server.
- Database folder:
  - smokedatabase: This is the database file to save all QR code scan logs. 
  - QRscanner.js: This file integrates all the nodejs code, which includes receiving HTTP requests from Python, storing the received data in the database, and sending keys to both ESP32s throuh UDP.
  - rest files are just spreate test module file.
- ESP32 Scooter folder:
  - ./blink_example_main.c: This is ESP32 code that make the ESP32 as a UDP server to receive the key from the client which is the auth server. Besides, it also run as a IR receiver to receive the key sent from the Fob.
- ESP32 Fob folder:
  - /main/ir-txrx.c: This is ESP32 code that make the ESP32 as a UDP server to receive the key from the client which is the auth server. Besides, it also run as a IR transmitter to send the key to the Scooter.
  
