# MAIN README

HAN reader for ESP8266 12E
==========================

This program for Arduino has the following functions:
* Read and decode data from the HAN-port on a Kaifa (MA304H3E 3-phase) smart electricity meter. It also reads data from Kaifa 1-phase meter, but this is not tested.
* Present data on a simple webpage, where the ESP8266 contains the webserver.
* Control two output relays.
* Send notification to mobile phone via the IFTTT service when the active power exceeds a user defined limit.

Hardware
--------
* LoLin NodeMCU V3 breakout board (Ebay)
* TSS721 Module Board M-BUS To TTL converter (AliExpress)
* FTDI basic 3.3V USB/Serial converter (Sparkfun)
* Two 3.3V relay boards that will operate each relay by draining less than 12mA from NodeMCU
* Some connecting wire
* Two suitable USB cables

Required software
-----------------
The sketch "Han_Receive_Web_Relay_Output.ino", the COM/serial driver for the USB to serial chip, and the necessary Arduino libraries (see below)

Get started (Windows 10)
------------------------
Install the latest Arduino for Windows from here: https://www.arduino.cc/en/Main/Software (should be 1.6.7 or newer).

Configure the Arduino application
---------------------------------
Start the Arduino application.

In File->Preferences->Settings, enter this additional Boards Manager URL: http://arduino.esp8266.com/stable/package_esp8266com_index.json .

In Tools->Board:->Boards Manager, find and install the library "esp8266 by ESP8266 Community".

In Tools, choose Board: "NodeMCU 1.0 (ESP8266-12E Module)", Flash Size: "4M (3M SPIFFS)", CPU Frequency: "80 MHz", Upload Speed: "115200".

Connect NodeMCU V3 to the computer
----------------------------------
Download COM/Serial port file CH341SER_WINDOWS.zip from https://github.com/nodemcu/nodemcu-devkit/tree/master/Drivers . Choose "Download ZIP", extract and install the driver. Note this is for the CH340 serial chip. Check the board you purchased. Your board may have the CP2102 chip, which needs another driver (not covered here). You should be able to find that driver online, and continue with this project.

Get data from the smart electricity meter
-----------------------------------------
 1. Update the Arduino sketch with the ssid and WiFi password for your local network
 2. Connect all equipment, including PC and meter
 3. On Arduino IDE, select the serial port for NodeMCU V3
 4. Upload the sketch via USB cable (serial upload)
 5. When upload is finished, select serial port for the monitor USB/Serial converter. Start the monitor
 6. Check that ESP8266 connects to your local WiFi network and note the provided IP address
 7. Type the address into the address field of your browser on a PC or mobile phone (device must be connected to the same network)
 8. The webpage with data from the electricity meter should show on your screen
10. To make the arrangement stand-alone, you may remove the USB cable used for monitoring,
    and power the NodeMCU via USB cable from a mobile phone charger or other 5V adapter.

During initial upload, the OTA (Over The Air) update code was installed. In subsequent updates you shold find and choose the WiFi port under Tools->Port:->Network ports. This makes it easy to update the code, even when the board is placed in a hard to reach location. Note, when doing the OTA update the NodeMCU board and PC with Arduino IDE must be connencted to the same network (LAN).

About the Arduino sketch
------------------------
To see the messages in HEX code format on the serial monitor, set the debug variable to true. Due to limitations (in the processor or in my code), about 1% of the messages are lost. I believe this to be a "time battle" between the TCP protocol and the Arduino code, and to be of little importance to this application.

The webpage uses only one variable per data field, the fields on the webpage show the most recent value available. The last message type that was read is shown on the page in red typing. To get the latest data, refresh the page by clicking one of the relay control Update / Send buttons.

To connect from the outside world, you need to forward port 80 in your router to the IP address of ESP8266 / NodeMCU. The sketch allows NodeMCU to be assigned an IP address from the DHCP pool of the houme router. Given that the board is mostly running continously, and the lease time is set to one or two days (recommended), it will rarely get a new IP address. So port forwarding to one address should not be a problem. 

I found that using the serial port (UART0) on ESP8266 is the best solution for collecting data from the meter. Data format is 8E1, which is not supported by SoftwareSerial.
UART0 is connected physically to the USB/serial chip on the board, and also to the pins marked TX/RX on the board. By connecting the TTL signal directly to the RX pin on NodeMCU, there will be a port collision. So, in the setup() function, Serial.swap() is called to move TX from GPIO1/TX/D10 to GPIO15/D8 and RX from GPIO3/RX/D9 to GPIO13/D7. Now TTL from the meter can be connected to GPIO13, and there will be no collision.

Explaining how to configure IFTTT (IF This Then That) on the IFTTT site is outside the scope of this Readme, but it is quite easy to get working if you spend a little time looking into it.
