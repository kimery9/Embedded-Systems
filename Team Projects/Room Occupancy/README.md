# Room and Occupancy

Authors: John Culley, Kai Imery, Ananth Sanjay, Jeffrey Zhang

Date: 2023-02-24

### Summary

To create the room occupancy check we first identified the key compononents of the project. The first task in the room occupancy check is to first ensure that all of the appropriate sensors are reading correct values. There are four sensors to complete the room occupancy check: ultrasonic, IR, thermistor, and solar. The next key step in our design process was being able to convert the sensor data into the appropriate engineering units and then display it with canvasJS. At this point, the final step of design was to develop an algorithm that utilized all of the sensors inputs and decided when to trigger the LED to alert that there is a user in the room. This result then would also be displayed on the canvasJS page as well. With all of the tasks identified, our team divided to complete all of the separate components. John and Ananth worked on the algorithm to determine if a person entered the room. Kai and Jeffrey worked on getting the sensor data onto canvasJS. Jeffrey also was in charge of doing the circuit component of the project. After each individual component was designed, we all brought our pieces together and began testing. From first glance it appeared that everything was working properly, but we realized that the algorithm wasn't working properly. We figured out that there was a slight logic error with an array and then modified it to fix the issue. Additionally, we were initially unsure of how often we should be refreshing the webpage, but we determined with some testing that one second was appropriate to map the change in the sensor data. At this point, our room occupancy check was able to properly determine whether a person entered the room. 

### Self-Assessment 

| Objective Criterion | Rating | Max Value  | 
|---------------------------------------------|:-----------:|:---------:|
| Objective One | 1 |  1     | 
| Objective Two | 1 |  1     | 
| Objective Three | 1 |  1     | 
| Objective Four | 1 |  1     | 
| Objective Five | 1 |  1     | 
| Objective Six | 1 |  1     | 


### Solution Design

There are four major components to the project functionality: sensor readings, creation of the csv file, canvasJS, and the algorithm to determine whether somebody is in the room. In ESP32, we use three different ADC pins to connect thermistor, ultrasonic sensor and solar panel for acquiring their data and then converting it into engineering units. Not only that, we also use i2c to get the long range measurements from LIDAR Lite v3 and then have an LED controlled by GPIO. Next, we output the sensor measurements on the ESP32 console in the established format (the data format suitable for csv files). We then use node.js to get these outputs from the serial port and save them in a data.csv file. Then, in the own-data.html file, we read the last 500 data from the data.csv file (reacting to a total of 100 time intervals, or 50 seconds of data) and plotted them in the graph. In the own-data.html file, an ajax call is used to pull in the data from a csv file. Ajax calls are an extremely efficient way to load data into web pages because they are asynchronous. This means that this data can load separately from the rest of the webpage which greatly impacts the optimization and efficiency because this file will need to be loaded very frequently as the data is being streamed real time. The ajax call, if successful in pulling the file, then calls a function called ‘processData’ with a parameter of the csv data from the file. Within the function ‘processData’, the csv file is split into five different elements: the solar reading, the ultrasonic reading, the IR rangefinder reading, the temperature reading, and the boolean value that stores whether somebody is inside the room or not. This is done by using the ‘split’ function and separating by commas because that is the benefit of using a CSV file which means that the values are separated by commas. These are then separated using a for loop and five corresponding if statements that then use JavaScript parse functions to store the data as x and y values, with the x value being the time and the y value being the appropriate sensor reading. Each of these individual data series are then attached to their corresponding segment of the graph with the appropriate titles, axes, and a legend. Then under the graph, there is a separate html div that then will take the reading and display on the page whether there is a person inside of the room or not. Within this separate div there is a script with JavaScript that determines which message to display then the message is displayed. The algorithm to determine whether the room is occupied or not starts by calling the sensor initialization functions and then reading each of their respective values by calling the functions ‘solar_read’, ‘thermistor_read’, ‘ultrasonic_read’, and ‘IR_read’. Next, boolean values for each of the sensors are initialized to false. Then there are four if statements that then check the sensor values to determine if there is somebody in the room. The ‘solar_volt’ is checked to see if it is greater than 0.15V, the ‘temp’ is checked to see if it is greater than 31 degrees Celsius, the ‘ultrasonic’ is checked to see if it is less than 2.0 meters, and the ‘IR’ is checked to see if it is less than 2.15 meters. If these individual if statements prove to be true, their corresponding boolean variables are declared as true. Next, a count variable is set to 0 and then all of the boolean variables are stored within a boolean array. The array is then looped through, and if the indexed value is true, count is then incremented. The function then performs a final check with an if statement that states if count is greater than or equal to 2, the ‘blink_led()’ function will be called turning on the LED to signify that a person has entered the room. The check for two different sensors is done as a precaution in case there is a faulty sensor or there is an extraneous circumstance that alters the sensor reading such as the heater turns on, somebody sets a box on a table, etc.


### Sketches/Diagrams
</p>
<img width="405" alt="Screen Shot 2023-02-24 at 2 15 42 PM" src="https://user-images.githubusercontent.com/91488781/221271047-31dcd253-c9fe-4cce-99e8-86478c5c72d2.png">

Simplified pseudocode of the our occupancy algorithm with ranges. If two or more sensors go out of the range, then the room is occupied.
</p>
<img width="462" alt="Screen Shot 2023-02-24 at 2 11 27 PM" src="https://user-images.githubusercontent.com/91488781/221270363-e35f4f61-a1ae-4328-8693-2ccced01a810.png">
An image of our simplified circiut with all of the sensors made with circuitio.
</p>
<img width="875" alt="Screen Shot 2023-02-24 at 2 12 15 PM" src="https://user-images.githubusercontent.com/91488781/221270489-5efaf7d0-d9d9-43ce-b84a-5a53e4369d90.png">
Diagram of the pseudo code with the ports on the circuit. The diagram explains how the esp 32 code interacts with the node.js code and the canvas.js code. 
</p>
<img width="809" alt="Screen Shot 2023-02-24 at 2 38 15 PM" src="https://user-images.githubusercontent.com/91488781/221275077-d211f384-bc4a-46ad-ab6d-78196d95229e.png">

Circuit diagram with ports, resistors, sensors, LED, and esp with correct wiring.
</p>
<img width="878" alt="Screen Shot 2023-02-24 at 2 13 26 PM" src="https://user-images.githubusercontent.com/91488781/221270677-2235a203-f2d1-49cf-89c8-42c60384e365.png">
Picture of the real circuit with all of the sensors connected.
</p>




### Supporting Artifacts
- [Link to video technical presentation](https://drive.google.com/file/d/1fv1DkznFqCfU3Npuyk9w1I-A5Rq4aI5k/view?usp=sharing). Not to exceed 120s
- [Link to video demo](https://drive.google.com/file/d/1ovlkVKdCJ2RRw5l3wZ_r_AvLheHWm6cJ/view?usp=sharing). Not to exceed 120s
- [Link to extra fun video](https://drive.google.com/file/d/19bXfsySWxw1WCMjh5VN6hOewvUmUo9qS/view?usp=sharing). 

### Modules, Tools, Source Used Including Attribution
ESP32, 
Visual Studio Code, 
CanvasJS, 
Node.js, 
ESP-IDF example Code, 
Ajax Calls, 
Lidar V3, 
Steinhart-Hart Equation, 
Thermistor, 
Maxbotics Ultrasonic Range Finder, 
Solar Panel, 
Red LED,
CanvasJS example code,
Node.js example code
### References
ESP-IDF Source Code, 
CanvasJS Source Code, 
Node.js Source Code

