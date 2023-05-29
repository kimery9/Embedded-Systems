# Code Readme

The code is divided into two main folders. The main folder houses the main code files that is flashed into the buggy, and the HTML folder houses the code that is used to implement the remote control start of the buggy.  
- ESP32/Main Folder:
  - The mcpwm_servo_control_example_main.c file houses all the code needed for the buggy as it impliments the the turning functions, alphanumerican display, PID Control, stopping functions, internal timers, and tasks that are need for the desired output to occur. The heart of the code itself appears in a while loop that uses imbedded if-else statements in order to logically decided what actions are needed. These logical statements are based on speed, and LIDAR distances which prompt the changes in the buggy. 

- HTML folder:
  - index.html: This file creates the html layout of the page as it initalizing the remote button.
  - server.js: This file creates a node server and is used to make the port of index.html 8082, and is also receiving the HTTP Request. Once it receive the request, it will forward to ESP32 using UDP.
  - client.js: This file uses an HTTP Request which is used to retrieve something from the HTML page, in this case it was used to obtain the value of the button.
