# Custom smart-home sytem

This project is part of my very own smart-home system built in my home to control devices across my apartment and measure temperature both indoor and outdoor. This system currently consist 6 working units:

* A central unit serving several python-based microservices and running an Angular-based UI on a 7" touchscreen. 
* An indoor unit controlling two led-strips over my desk and measuring temperature in my room every 10 minutes.
* An indoor unit controlling an RGB led-strip over my bed.
* An indoor unit controlling my TV replaying the control-messages recorded from my TV-remote.
* An indoor unit to measure temperature in the Kitchen (currently out of service).
* An outdoor unit to measure temperature (currently out of service).

# Indoor controller based on ESP32

This unit is responsible for three task:

* It controls the mood light behind my laptop-stand
* It controls a led-strip above my desk
* And measures the room temperature every 10 minutes.



## Hardware

The unit is based on a NodeMCU ESP-32S module with linear-potmeters as input and two IRLZ44N MOSFET to dim the intensity of the led-strips. The unit uses 12V input to power both the LED-strips and the controller (a small DC-DC step-down converter included). I had designed a custom box for the device and printed out with our 3D printer. 

The unit uses a [2.42" display](https://www.ebay.com/itm/I2C-2-42-OLED-128x64-Graphic-OLED-White-Display-Arduino-PIC-Multi-wii/191835417966?ssPageName=STRK%3AMEBIDX%3AIT&_trksid=p2057872.m2749.l2649) to show current information and also, there is a hidden capacitive touch-button under the hood to switch on/off the display.



## Software

The firmware for the controller is developed with the [PlatformIO](https://platformio.org/) plugin in Visual Studio Code. The communication in the whole system is based on MQTT, thus the firmware uses the PubSubClient library to communicate with other modules - although I have implemented a wrapper class around the PubSubClient to use C++11 features like lambda functions etc.

The display is controlled using the Adafruit GFX library, but I have implemented a custom class to have the oportunity to have more then one frame which are could be rotated periodically.

## Usage

You can use this code as follows:

1. Install Visual Studio Code with the [PlatformIO](https://platformio.org/) plugin
2. Checkout this repository
3. Checkout the [libraries repository](https://github.com/zsoltmazlo/ESP32-libs) as well.
3. Open repository folder with VS Code
4. Install libraries with the following command: `pio lib install 13, 64, 551, 1, 89`
5. Build code and upload it.
6. Optional: all configuration is stored in the `Configuration struct` as constant expressions - you can change them there (pins, MQTT topic names, etc)
