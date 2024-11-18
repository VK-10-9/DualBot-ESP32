# DualBot ESP32  
A versatile, dual-mode robot built using the ESP32 microcontroller. This project combines two functionalities:  
1. **IR-based Line Following**  
2. **Web-Based Remote Control**  

## Features  
- Switch between autonomous line-following mode and manual remote-controlled mode.  
- Wireless communication using a web interface.  
- Speed control with a rotary encoder.  

## Hardware Requirements  
- ESP32-S3-MINI-1U-N4R2  
- IR Sensors  
- Motor Driver (e.g., L298N)  
- DC Motors  
- Power Supply (e.g., Li-ion battery)  
- Rotary Encoder  

## Software Requirements  
- Arduino IDE  
- Required Libraries:  
  - `WiFi.h`  
  - `ESPAsyncWebServer.h`  
  - `Encoder.h`  

## Getting Started  
Follow these steps to build and run the bot:  
1. Assemble the hardware as shown in the circuit diagram.  
2. Flash the firmware provided in the `/software/` folder using Arduino IDE.  
3. Connect to the web interface to control the bot wirelessly.  

