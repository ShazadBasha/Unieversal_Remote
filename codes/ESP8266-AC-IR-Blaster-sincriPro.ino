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
int globalFanSpeed = 1;
String globalMode = "cool";

bool onPowerState(const String &deviceId, bool &state) {
  Serial.printf("PowerState: %s -> %s\n", deviceId.c_str(), state ? "ON" : "OFF");
  globalPowerState = state;
  if (state) {
    ac.on();
    ac.setTemp(globalTemperature);
    ac.setMode(kCoolixCool);
    ac.setFan(kCoolixFanAuto);
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
  Serial.printf("FanSpeed: %s -> %d\n", deviceId.c_str(), rangeValue);
  globalFanSpeed = rangeValue;
  if (globalPowerState) {
    ac.setFan(kCoolixFanAuto);
    ac.send();
  }
  return true;
}

bool onThermostatMode(const String &deviceId, String &mode) {
  Serial.printf("ThermostatMode: %s -> %s\n", deviceId.c_str(), mode.c_str());
  globalMode = mode;
  if (globalPowerState) {
    if (mode == "COOL") ac.setMode(kCoolixCool);
    else if (mode == "HEAT") ac.setMode(kCoolixHeat);
    else if (mode == "FAN") ac.setMode(kCoolixFan);
    else if (mode == "DRY") ac.setMode(kCoolixDry);
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
    String html = "<html><head><style>body{font-family:sans-serif;text-align:center}a{display:inline-block;padding:10px 20px;margin:5px;border:1px solid #ccc;border-radius:8px;text-decoration:none;color:#333;background:#f4f4f4}a:hover{background:#ddd}</style></head><body>";
    html += "<h1>Onida AC Control</h1>";
    html += "<a href=\"/on\">Turn ON</a> ";
    html += "<a href=\"/off\">Turn OFF</a> ";
    html += "<a href=\"/cool24\">Cool 24°C</a> ";
    html += "<a href=\"/cool26\">Cool 26°C</a> ";
    html += "<a href=\"/heat24\">Heat 24°C</a> ";
    html += "<a href=\"/dry\">Dry Mode</a> ";
    html += "<a href=\"/fan\">Fan Mode</a> ";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });
  server.on("/on", [](){ bool s=true; onPowerState(ACUNIT_ID, s); server.send(200, "text/plain", "AC turned ON"); });
  server.on("/off", [](){ bool s=false; onPowerState(ACUNIT_ID, s); server.send(200, "text/plain", "AC turned OFF"); });
  server.on("/cool24", [](){ globalTemperature=24; onTargetTemperature(ACUNIT_ID, globalTemperature); String m="COOL"; onThermostatMode(ACUNIT_ID, m); server.send(200, "text/plain", "Set to Cool 24°C"); });
  server.on("/cool26", [](){ globalTemperature=26; onTargetTemperature(ACUNIT_ID, globalTemperature); String m="COOL"; onThermostatMode(ACUNIT_ID, m); server.send(200, "text/plain", "Set to Cool 26°C"); });
  server.on("/heat24", [](){ globalTemperature=24; onTargetTemperature(ACUNIT_ID, globalTemperature); String m="HEAT"; onThermostatMode(ACUNIT_ID, m); server.send(200, "text/plain", "Set to Heat 24°C"); });
  server.on("/dry", [](){ String m="DRY"; onThermostatMode(ACUNIT_ID, m); server.send(200, "text/plain", "Set to Dry Mode"); });
  server.on("/fan", [](){ String m="FAN"; onThermostatMode(ACUNIT_ID, m); server.send(200, "text/plain", "Set to Fan Mode"); });

  server.begin();
}

void loop() {
  server.handleClient();
  MDNS.update();
  SinricPro.handle();
}
