#include <Arduino.h>
#include <WiFi.h>

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
PowerReading reading;
reading.grid = 100.10;
reading.load = 200.20;
reading.solar = 300.30;

return reading;
}

