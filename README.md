# Tilt Controlled Distributed LED Lights 
This is a fun project I build over a few weekends.  I would recommend this project for those who are adventurous and would like to implement a project that uses MQTT.

## Code Basics
Here you will find arduino code (written for NodeMCU) for the:

    1. light-remote: Device code for taking the X,Y,Z axis tilt, converting those values to RGB, and sending to the light-node
    
    2.light-node: Device code for receiving the RGB values from the light-remote and displaying them in an LED light strip

Note: The Light-Remote does not need code for the Light-Node and vice-versa.

## Getting Started
These files were created in VSCode with the [platformio extension](https://www.platformio.org).  I did not include most of the overhead from these files as I wanted to keep the code simple.  In the case that you use platformio, remember to install each library individually.

You can easily have this code run within the Arduino IDE, in that case remove the `#include 'Arduino.h'` at the top of the `light-remote.cpp` and `light-node.cpp`.  Remeber to download each of the libraries in the Arduino IDE.

Also make sure to address the `TODO`s in both the `light-node` and `light-remote` folder.  I added a `common.h` for both the light-remote and light-node as a safer place to list your Wifi Credentials.

Best of luck.  If you have any questions feel free to reach out.

Andrew
