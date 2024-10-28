#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <OLED_I2C.h>

OLED myOLED(SDA, SCL);

extern uint8_t SmallFont[];

class PowerReading{
  public: 
    float solar;
    float grid;
    float load;

  String print() {
    return String("Grid: " + String(grid) + " Load: " + String(load) + " Solar: " + solar);
  }
};

void initWiFi();
PowerReading getPowerReading();

void setup() {
  Serial.begin(9600);
  initWiFi();

  myOLED.begin(SSD1306_128X32);
  myOLED.setFont(SmallFont);
  myOLED.clrScr();
  myOLED.print("Hello, world!", CENTER, 0);
  myOLED.update();
}

void loop() {
  Serial.println(getPowerReading().print());
  sleep(1);
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin("scoltock", "nowireshere");
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

PowerReading getPowerReading()
{
HTTPClient http;
JsonDocument doc;

String serverPath = "http://192.168.1.85/status/powerflow";
http.begin(serverPath.c_str());
int httpResponseCode = http.GET();
      
if (httpResponseCode>0) {
        String payload = http.getString();
        deserializeJson(doc, payload);
}

http.end();

PowerReading reading;
reading.grid = doc["site"]["P_Grid"];
reading.load = doc["site"]["P_Load"];
reading.solar = doc["site"]["P_PV"];

return reading;
}

