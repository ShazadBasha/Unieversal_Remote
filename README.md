# ESP8266 AC IR Blaster

A DIY project to control your Air Conditioner remotely using ESP8266, an IR LED, and a BC547 transistor.

## 📦 What’s in this Project
- 📱 Control your AC via a simple web server
- 📡 Send IR commands via ESP8266
- 🔌 Transistor-based IR LED driver
- 🔄 Test visible LED blinks for verification

## 📑 Hardware Required
- ESP8266 board
- IR LED
- 1x BC547 NPN transistor
- 220Ω resistor
- Breadboard & jumper wires

## 🔌 Circuit Diagram
See `schematics/circuit_diagram.jpg`

## 📜 How to Use
1. Flash `IRTestBlink.ino` to test visible LED blinks.
2. Confirm IR LED circuit works.
3. Flash `ACRemoteWebServer.ino` to enable web control.
4. Access ESP8266 IP in browser → control your AC.

## 🔐 IR Codes
Find raw IR codes in `ir_codes/raw_codes.txt`

## 📷 Demo
See `images/working_demo.jpg`

## 📄 License
This project is open-source under the MIT License.
