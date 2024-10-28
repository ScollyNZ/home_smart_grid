#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
class PowerReading{
  public: 
    float solar;
    float grid;
    float load;

  String print() {
    return String("Grid: " + String(grid) + "\nLoad: " + String(load) + "\nSolar: " + solar);
  }
};

void initWiFi();
PowerReading getPowerReading();
void WriteToDisplay(String text);

void setup() {
  Serial.begin(9600);
  initWiFi();
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
}

void loop() {
  String text;
  text = getPowerReading().print();
  Serial.println(text);
  WriteToDisplay(text);
  
  sleep(1);
}

void WriteToDisplay(String text)
{
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  // Display static text
  display.println(text);
  display.display();
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

