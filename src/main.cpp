#include <Arduino.h>
#include <WiFi.h>

struct PowerReading{
  float solar;
  float grid;
  float load;
};

void initWiFi();
PowerReading getPowerReading();

void setup() {
  Serial.begin(9600);
  initWiFi();
}

void loop() {
  // put your main code here, to run repeatedly:

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
reading.load = 300.30;

return reading;
}

