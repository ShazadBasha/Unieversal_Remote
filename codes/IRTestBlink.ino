#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>

// WiFi credentials
const char* ssid = "WIFI Name";
const char* password = "Password";

// Web server on port 80
ESP8266WebServer server(80);

// IR Receiver and Transmitter Pins
const uint16_t recvPin = 14; // D5 GPIO14 for TSOP1738 OUT
const uint16_t irLedPin = 4; // D2 GPIO4 for IR LED

IRrecv irrecv(recvPin);
IRsend irsend(irLedPin);
decode_results results;

// Define AC IR codes
uint16_t switchOnRaw[199] = {4424, 4348,  570, 1614,  578, 518,  574, 1610,  572, 1614,  542, 552,  542, 576,  518, 1666,  518, 578,  518, 580,  518, 1664,  516, 580,  516, 578,  518, 1668,  516, 1668,  518, 578,  518, 1670,  516, 1666,  516, 580,  516, 1668,  518, 1668,  516, 1668,  516, 1668,  516, 1668,  516, 1670,  516, 582,  516, 1664,  516, 580,  516, 580,  516, 580,  516, 580,  516, 580,  516, 582,  516, 582,  516, 580,  516, 580,  516, 578,  518, 580,  516, 580,  516, 580,  516, 582,  516, 1672,  516, 1668,  516, 1668,  516, 1668,  514, 1670,  516, 1670,  516, 1668,  516, 1670,  514, 5272,  4370, 4406,  514, 1670,  514, 580,  518, 1668,  516, 1668,  516, 582,  516, 580,  516, 1670,  514, 584,  516, 582,  516, 1666,  514, 582,  514, 582,  514, 1670,  516, 1670,  514, 582,  514, 1672,  514, 1668,  514, 580,  516, 1670,  514, 1670,  516, 1668,  516, 1670,  516, 1670,  516, 1672,  516, 582,  514, 1666,  516, 580,  516, 580,  514, 582,  514, 582,  516, 580,  514, 584,  514, 584,  514, 582,  514, 582,  514, 582,  514, 582,  514, 582,  514, 580,  514, 584,  514, 1674,  514, 1670,  514, 1670,  514, 1670,  514, 1670,  514, 1670,  514, 1670,  514, 1674,  538};

uint16_t switchOffRaw[199] = {4420, 4348,  572, 1612,  548, 548,  548, 1638,  570, 1614,  544, 552,  542, 576,  520, 1664,  518, 580,  518, 582,  516, 1664,  516, 580,  516, 580,  516, 1668,  516, 1668,  516, 578,  516, 1670,  516, 582,  516, 1664,  516, 1668,  516, 1668,  514, 1668,  516, 580,  516, 1668,  514, 1672,  516, 1666,  516, 580,  516, 580,  518, 578,  518, 578,  516, 1668,  516, 580,  516, 582,  516, 1666,  516, 1668,  516, 1670,  514, 580,  516, 580,  516, 580,  516, 580,  516, 582,  516, 582,  516, 580,  516, 580,  516, 1666,  514, 1670,  514, 1670,  516, 1668,  516, 1670,  516, 5246,  4368, 4406,  514, 1670,  516, 580,  516, 1670,  514, 1670,  516, 580,  516, 580,  514, 1670,  516, 582,  514, 584,  516, 1666,  514, 582,  514, 582,  514, 1670,  516, 1668,  516, 580,  514, 1672,  514, 584,  514, 1666,  514, 1668,  514, 1670,  514, 1670,  516, 580,  514, 1670,  516, 1672,  514, 1668,  514, 582,  516, 580,  514, 582,  514, 582,  516, 1670,  514, 582,  514, 584,  514, 1668,  514, 1670,  514, 1672,  512, 582,  514, 582,  514, 582,  512, 582,  514, 586,  512, 584,  514, 584,  514, 582,  512, 1668,  514, 1670,  514, 1670,  514, 1670,  514, 1674,  536};

void handleSwitchOn() {
  Serial.println("Sending SwitchOn IR code...");
  irsend.sendRaw(switchOnRaw, 199, 38);
  server.send(200, "text/plain", "SwitchOn command sent.");
}

void handleSwitchOff() {
  Serial.println("Sending SwitchOff IR code...");
  irsend.sendRaw(switchOffRaw, 199, 38);
  server.send(200, "text/plain", "SwitchOff command sent.");
}

void setup() {
  Serial.begin(115200);
  irrecv.enableIRIn();
  irsend.begin();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/switchon", handleSwitchOn);
  server.on("/switchoff", handleSwitchOff);
  server.begin();
}

void loop() {
  server.handleClient();

  if (irrecv.decode(&results)) {
    Serial.println(F("Received IR Signal:"));
    Serial.println(resultToHumanReadableBasic(&results));
    irrecv.resume();
  }
}
