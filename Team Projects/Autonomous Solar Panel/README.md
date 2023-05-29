# Solar Tracker

Authors: John Culley, Ananth Sanjay, Jeffrey Zhang, Kai Imery

Date: 2023-02-07

### Summary

To tackle the solar tracker as a group we first identified the key components to the design. These components were the voltage reader, the algorithm to maximize the voltage, the servos, and the hardware timer. The next challenge was to identify the flow of the project. The first task was to receive the input voltage, the next task is to find the maximum voltage and the corresponding azmuth and altitude, during that task the servos will be rotated meaning that these functions will be occuring at the same time. The last component is the hardware timer which we set to 20 seconds which will provide enough time to run all of the functions. The task was then divided amongst our team and each member completed a different component. John and Ananth worked on the algorithm to determine which angle would hold the maximum voltage. Kai worked with the servos to make sure that the double servo unit was functioning properly. Jeffrey was in charge of the circuitry, the timer, and the display. At this point, we combined all aspects of our project and began the proper setup. After the integration, the servos weren't working properly as a unit so we had to adapt the code so that the serovs didn't conflict with each other. Also, the wiring for our sensor was inaccurate which was giving improper readings so we checked the wiring and corrected it. Finally, we initially had a timer cycle of ten seconds, but upon testing determined that this was not enough time to properly complete all of the functions so we changed the cycle to twenty seconds which was enough time to do the task accordingly.

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

There are four major components to the code functionality: i2c display, servos, voltmeter, and timer. Each component was constructed with their functionality contained within functions with an efficiently laid out sequence. The first function of the code is to read the voltage, which is contained within the 'read_voltage' function. This function uses the adc port of the ESP32 to read the raw data which is then transferred to mV. The voltage is what is returned from the function. The voltage function is then used extensively within the 'find_azm_alt' function. The 'find_azm_alt' function needs predefined variables to store the previous and current state of voltage, the current azmuth and altitude values, the max voltage, and azmuth and altitude step and range values which will be passed to the servos function. Within the 'find_azm_alt' function it begins by checking if the servo needs to do a wide scan of the entire range. If this proves to be true, the function performs a binary check of the voltages by first -90 to 90 for azmuth and 0 to 90 for altitude and storing the maximum voltage within the range along with the degrees at which the maximum voltage was acquired. After the maximum voltage was acquired, at the end of the function it is stored in the 'prev_volt' variable. Upon reentry to the function, a check is performed to determine whether the servos need to check the entire range or perform a small shift. If 'prev_volt' is greater than the measured voltage plus 300, then another complete range check needs to be performed. Otherwise, a small shift can be performed. This check allows for the solar tracker to only do a complete range check if the solar input has changed substantially, and if it hasn't moved substantially then a small shift follows the solar input. The 'rotate_servo' function is called within the 'find_azm_alt' function with a servo number parameter and a range parameter. This allows the function to be called for a specific servo and range allowing it to be used flexibly within the code. The rotate_servo function uses the preinitalized comparators and the output of the example_angle-to_compare function to check the physical servo location vs the desired angle. The example_angle_to-compare function checks if the given angle is within the allowed minimum and maximum along with inside the needed pulsewidth. The hardware timer was set to a period of 20s which allowed for all of the functions to complete in their entirety before starting another cycle. We use connect two servo motors to GPIO pins 12 and 33 and connect the display to the I2C pins. Then we use ADC channel 6 (GPIO pin 34) to read the voltage of the solar panel.

### Sketches/Diagrams
<p align="center">
<img width="254" alt="image" src="https://user-images.githubusercontent.com/113144839/217354289-5bb91f5f-3c35-4cb3-b399-be2358f6df6b.png">
</p>
<p align="center">
The flowchart shows the algorithm design and project flow through its multiple states and the timer period of 20s.
</p>

<p align="center">
<img width="304" alt="image" src="https://user-images.githubusercontent.com/113144839/217410338-880da55b-9dca-422d-8188-46f6157a2b0d.png">
</p>
<p align="center">
Pictures from our design process and demo day!
</p>



### Supporting Artifacts
- [Link to video technical presentation](https://drive.google.com/file/d/1rsUrqxKzsXTcJ2qmQr8MCVBb2UaERr5l/view?usp=sharing).
- [Link to video demo](https://drive.google.com/file/d/1Bk7z9dyCVurmIdYeYjn3OUVca8PROR9R/view?usp=sharing).


### Modules, Tools, Source Used Including Attribution
ESP-IDF Example Code, 
ESP32, 
Visual Studio Code, 
i2c display, 
SG90 servos, 
Solar panel

### References
ESP-IDF Source Code

