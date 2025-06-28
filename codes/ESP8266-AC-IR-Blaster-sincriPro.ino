#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Coolix.h>
#include <ESP8266mDNS.h>
#include "SinricPro.h"
#include "SinricProWindowAC.h"

#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASS     "YOUR_WIFI_PASSWORD"
#define APP_KEY       "YOUR_APP_KEY_HERE"
#define APP_SECRET    "YOUR_APP_SECRET_HERE"
#define ACUNIT_ID     "YOUR_DEVICE_ID_HERE"

const uint16_t kIrLed = 4;  // D2 (GPIO4)
IRCoolixAC ac(kIrLed);
ESP8266WebServer server(80);

float globalTemperature = 24;
bool globalPowerState = false;
int globalFanSpeed = kCoolixFanAuto;
String globalMode = "AUTO";
uint8_t timerHours = 0;

bool onPowerState(const String &deviceId, bool &state) {
  Serial.printf("PowerState: %s -> %s\n", deviceId.c_str(), state ? "ON" : "OFF");
  globalPowerState = state;
  if (state) {
    ac.on();
    ac.setTemp(globalTemperature);
    ac.setFan(globalFanSpeed);
    ac.setMode(kCoolixAuto);
    ac.send();
  } else {
    ac.off();
    ac.send();
  }
  return true;
}

bool onTargetTemperature(const String &deviceId, float &temperature) {
  Serial.printf("TargetTemperature: %s -> %.1f°C\n", deviceId.c_str(), temperature);
  globalTemperature = temperature;
  if (globalPowerState) {
    ac.setTemp(globalTemperature);
    ac.send();
  }
  return true;
}

bool onAdjustTargetTemperature(const String &deviceId, float &temperatureDelta) {
  Serial.printf("AdjustTemperature: %s -> delta %.1f\n", deviceId.c_str(), temperatureDelta);
  globalTemperature += temperatureDelta;
  return onTargetTemperature(deviceId, globalTemperature);
}

bool onRangeValue(const String &deviceId, int &rangeValue) {
  Serial.printf("RangeValue received: %d\n", rangeValue);

  if (rangeValue >= kCoolixFanMin && rangeValue <= kCoolixFanMax) {
    Serial.println("→ Mapping to Fan Speed");
    globalFanSpeed = rangeValue;
    if (globalPowerState) {
      ac.setFan(globalFanSpeed);
      ac.send();
    }
    return true;
  }

  if (rangeValue >= 1 && rangeValue <= 23) {
    Serial.println("→ Mapping to Timer Hours (not supported directly in library)");
    timerHours = rangeValue;
    // NOTE: Coolix IR library doesn't support setTimer
    // You can store the timerHours and handle it manually if needed
    return true;
  }

  Serial.println("→ Unsupported RangeValue received");
  return false;
}

bool onThermostatMode(const String &deviceId, String &mode) {
  Serial.printf("ThermostatMode: %s -> %s\n", deviceId.c_str(), mode.c_str());
  globalMode = mode;
  if (globalPowerState) {
    if (mode == "COOL") ac.setMode(kCoolixCool);
    else if (mode == "FAN") ac.setMode(kCoolixFan);
    else if (mode == "DRY") ac.setMode(kCoolixDry);
    else if (mode == "AUTO") ac.setMode(kCoolixAuto);
    else {
      Serial.println("Unsupported mode requested");
      return false;
    }
    ac.send();
  }
  return true;
}

void setupWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());
  if (MDNS.begin("accontroller")) {
    Serial.println("mDNS responder started");
  }
}

void setupSinricPro() {
  SinricProWindowAC &myAcUnit = SinricPro[ACUNIT_ID];
  myAcUnit.onPowerState(onPowerState);
  myAcUnit.onTargetTemperature(onTargetTemperature);
  myAcUnit.onAdjustTargetTemperature(onAdjustTargetTemperature);
  myAcUnit.onRangeValue(onRangeValue);
  myAcUnit.onThermostatMode(onThermostatMode);

  SinricPro.begin(APP_KEY, APP_SECRET);
  SinricPro.onConnected([]() { Serial.println("Connected to SinricPro"); });
  SinricPro.onDisconnected([]() { Serial.println("Disconnected from SinricPro"); });
}

void setup() {
  Serial.begin(115200);
  ac.begin();
  setupWiFi();
  setupSinricPro();

  server.on("/", []() {
    String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'><style>"
      "body{font-family:sans-serif;text-align:center;padding:10px;background:#f0f0f0;}"
      "h1{margin-bottom:20px;}"
      ".btn{display:inline-block;padding:15px 25px;margin:10px;font-size:18px;border:none;border-radius:8px;background-color:#007BFF;color:white;text-decoration:none;width:80%;max-width:300px;}"
      ".btn:hover{background-color:#0056b3;}"
      "select, .status{font-size:18px;margin:10px;}"
      "</style></head><body>"
      "<h1>Onida AC Control</h1>"
      "<div class='status'><b>Power:</b> " + String(globalPowerState ? "ON" : "OFF") + "<br>"
      "<b>Temp:</b> " + String(globalTemperature) + "°C<br>"
      "<b>Mode:</b> " + globalMode + "<br>"
      "<b>Timer:</b> " + (timerHours > 0 ? String(timerHours) + " hr" : "OFF") + "</div>"
      "<a class='btn' href='/on'>Turn ON</a>"
      "<a class='btn' href='/off'>Turn OFF</a>"
      "<a class='btn' href='/tempup'>Temp +</a>"
      "<a class='btn' href='/tempdown'>Temp -</a>"
      "<a class='btn' href='/fanlow'>Fan: Low</a>"
      "<a class='btn' href='/fanmed'>Fan: Med</a>"
      "<a class='btn' href='/fanhigh'>Fan: High</a>"
      "<a class='btn' href='/fanturbo'>Fan: Turbo</a>"
      "<form action='/setmode' method='get'>"
      "<select name='mode'>"
      "<option value='AUTO'>Auto</option>"
      "<option value='COOL'>Cool</option>"
      "<option value='DRY'>Dry</option>"
      "<option value='FAN'>Fan</option>"
      "</select><br><button class='btn' type='submit'>Set Mode</button>"
      "</form></body></html>";
    server.send(200, "text/html", html);
  });

  server.on("/on", [](){ bool s=true; onPowerState(ACUNIT_ID, s); server.sendHeader("Location", "/", true); server.send(302, ""); });
  server.on("/off", [](){ bool s=false; onPowerState(ACUNIT_ID, s); server.sendHeader("Location", "/", true); server.send(302, ""); });
  server.on("/tempup", [](){ float d=1.0; onAdjustTargetTemperature(ACUNIT_ID, d); server.sendHeader("Location", "/", true); server.send(302, ""); });
  server.on("/tempdown", [](){ float d=-1.0; onAdjustTargetTemperature(ACUNIT_ID, d); server.sendHeader("Location", "/", true); server.send(302, ""); });
  server.on("/setmode", [](){ String m = server.arg("mode"); onThermostatMode(ACUNIT_ID, m); server.sendHeader("Location", "/", true); server.send(302, ""); });

  server.on("/fanlow", [](){ globalFanSpeed = kCoolixFanMin; ac.setFan(globalFanSpeed); ac.send(); server.sendHeader("Location", "/", true); server.send(302, ""); });
  server.on("/fanmed", [](){ globalFanSpeed = kCoolixFanMed; ac.setFan(globalFanSpeed); ac.send(); server.sendHeader("Location", "/", true); server.send(302, ""); });
  server.on("/fanhigh", [](){ globalFanSpeed = kCoolixFanMax - 1; ac.setFan(globalFanSpeed); ac.send(); server.sendHeader("Location", "/", true); server.send(302, ""); });
  server.on("/fanturbo", [](){ globalFanSpeed = kCoolixFanMax; ac.setFan(globalFanSpeed); ac.send(); server.sendHeader("Location", "/", true); server.send(302, ""); });

  server.begin();
}

void loop() {
  server.handleClient();
  MDNS.update();
  SinricPro.handle();
}
