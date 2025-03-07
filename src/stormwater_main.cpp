#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_SSD1306.h>
#include <appliance_state.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define ULTRASONIC_TRIGGER GPIO_NUM_16 // Trigger Pin
#define ULTRASONIC_ECHO GPIO_NUM_17    // Echo Pin

// ESP Pinout - https://medesign.seas.upenn.edu/index.php/Guides/ESP32-pins

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
class PowerReading
{
public:
  float solar;
  float grid;
  float load;

  // return true if power is currently being exported
  bool exporting()
  {
    return grid < 0;
  }

  float currentExportPower()
  {
    if (exporting())
    {
      return grid * -1;
    }
    return 0;
  }

  String print()
  {
    return String("Grid: " + String(grid) + "\nLoad: " + String(load) + "\nSolar: " + solar);
  }
};

/*
Appliance state machine
Disabled -enable-> Enabled -power on-> PowerOn -power off-> Power Off
                                               - disable -> Disabled (& PowerOff)
*/

/*
Describes the device current being managed
Make this downloadable one day
*/
class Appliance
{
public:
  StateMachine sm;

  // How often, minutes, can the power by cycled?
  int PowerCycleTime()
  {
    return 1;
  }

  // 1500 watts for the storm water pump.
  float PowerDraw()
  {
    return 1500.0;
  }

  // Logic for this type of applicance (e.g. the storm water pump will have a water level sensor)
  // This class needs an interface definiton
  bool CanActivate()
  {
    return true;
  }

  // Turn the device on
  void Activate()
  {
    // enable a GPIO pin
  }

  void Deactivate()
  {
    // disable a GPIO pin
  }

  void UpdatePowerStatus(PowerReading reading)
  {
  }
};

void initWiFi();
PowerReading getPowerReading();

// Update the oled display with the power statistics and the inhibit switch state
void WriteToDisplay(PowerReading, int);
/*
Return the number of seconds since the unit started.
Replace with an RTC one day
*/
long SecondsSinceStart();

/*
return the power draw of the current appliance. Will self calibrate in a later version
*/
float PowerDraw();

/*
Use the most recent power reading to determine if the power output status should be changed.
*/
void UpdatePowerOutput(PowerReading);

/*
Get reading from ultrasonic sensor, taking 5 readings and averaging them
*/
int ReadFromUltrasonic();

// Make a single reading from the ultrasonic sensor
int MakeSingleReadingFromUltrasonic();

Appliance CurrentDevice;

/*
void setup()
{
  Serial.begin(9600);
  initWiFi();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  pinMode(ULTRASONIC_TRIGGER, OUTPUT); // Set Trig as output
  pinMode(ULTRASONIC_ECHO, INPUT);     // Set Echo as input

  delay(2000);
}

void loop()
{
  String text;
  PowerReading reading = getPowerReading();
  WriteToDisplay(reading, ReadFromUltrasonic());

  sleep(1);
}
*/
void WriteToDisplay(PowerReading reading, int depth)
{

  // String inhibitState = "Yes";
  // if (inhibitSwitchState > 0) inhibitState = "No";

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 20);
  // Display static text
  // display.println(String(reading.print() + "\nInhibited: " + inhibitState));
  String output = String(reading.print() + "\nDepth: " + depth);
  display.println(output);
  display.display();

  Serial.write(output.c_str());
  Serial.write("\n\n");
}

void initWiFi()
{
  WiFi.mode(WIFI_STA);
  // yes, I know I have my WiFi details here.
  // If I see you sitting outside my house, stealing my interweb bits, there will be trouble.
  WiFi.begin("scoltock", "nowireshere");
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
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

  if (httpResponseCode > 0)
  {
    String payload = http.getString();
    deserializeJson(doc, payload);
  }

  http.end();

  PowerReading reading;
  reading.grid = doc["site"]["P_Grid"];
  reading.load = doc["site"]["P_Load"];
  reading.solar = doc["site"]["P_PV"];

  reading.load = reading.load * -1;

  return reading;
}

void UpdatePowerOutput(PowerReading reading)
{
  long currentTime = SecondsSinceStart();
  if (currentTime % (CurrentDevice.PowerCycleTime()))
  {
    if (reading.exporting())
    {

      if (PowerDraw() < (reading.currentExportPower()) && CurrentDevice.CanActivate())
      {
        // turn on power
        CurrentDevice.Activate();
      }
    }
  }
}

// Replace with an I2c RTC, when I find it, I know I bought one once upon a time....
long SecondsSinceStart()
{
  return millis() / 1000;
}

int ReadFromUltrasonic()
{
  // only make a new reading if the last reading was more than 10 seconds
  static long lastReading = 0;
  static int lastDepth = 0;

  if (SecondsSinceStart() - lastReading > 10 or lastDepth == 0)
  {
    // make 5 readings and average them
    int depth = 0;
    for (int i = 0; i < 5; i++)
    {
      depth += MakeSingleReadingFromUltrasonic();
    }
    lastDepth = depth / 5;
    lastReading = SecondsSinceStart();
  }
    return lastDepth;
}

int MakeSingleReadingFromUltrasonic()
{
  // return the depth of the water in the tank

  // trigger the ultrasonic sensor
  digitalWrite(ULTRASONIC_TRIGGER, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIGGER, LOW);

  // read the echo pin
  long duration = pulseIn(ULTRASONIC_ECHO, HIGH); // pulseIn returns the duration of the pulse in microseconds
  // calculate the distance
  int depth = duration * 0.034 / 2; // Speed of sound wave divided by 2 (goes there and back)
  return depth;
}
