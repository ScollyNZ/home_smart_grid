#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <mbedtls/sha256.h>
#include <mbedtls/base64.h>
#include <HTTPClient.h>
#include <Adafruit_SSD1306.h>
#include <appliance_state.h>

const char *ssid = "scoltock";
const char *password = "nowireshere";
const char *googleCloudHost = "pubsub.googleapis.com";
const int googleCloudPort = 443;
const char *projectId = "home-automation-453101";
//topic enum
enum PubSubTopic
{
  telemetry,
  heartbeat,
  command
};

static const char* PubSubTopicNames[] = {"telemetry", "heartbeat", "command"};

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define ULTRASONIC_TRIGGER GPIO_NUM_16 // Trigger Pin
#define ULTRASONIC_ECHO GPIO_NUM_17    // Echo Pin

// ESP Pinout - https://medesign.seas.upenn.edu/index.php/Guides/ESP32-pins

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
void initWiFi();
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

  JsonDocument toJson()
  {
    JsonDocument doc;
    doc["grid"] = grid;
    doc["load"] = load;
    doc["solar"] = solar;
    return doc;
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

PowerReading getPowerReading();
Appliance currentDevice;

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

void publishMessage(PubSubTopic topic, const JsonDocument &message);
String getAccessToken(const String &jwt);

// WARNING: Hardcoding keys is extremely insecure. Use SPIFFS or a secure element in production.
const char *privateKey = "-----BEGIN PRIVATE KEY-----\nMIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQDVtQHHCnybKopa\nKrzSnFkTwYYLMxbfZPieJeKoaE1P31X3EMpC/cNcMV1ISMwjqZIJUHHalcfBiHl4\nKWwbKEj/Eo11gOSSOOjjaZJt3+CCAc93DUVWI+0rXqDEnIbTZrc/goRtqljhPb58\nYTLQk3R8iFEASnqf5cceKW2euctrb4aSJHbTrHuwTRpmXaZ4ZDVFTRmF4Pr5o0qe\nxdJGQJphcjS1gBXs2Vf9K7Um/LwkAqP5QY6As6oVB4hkryel1BW49KR6DVUS5E+b\nhOzMoGKni9usu+JHvkpnKtlbyCC/ZV9lJZ7ip0FhD7FtWqGaYXanB5Tv8+tdKJQe\niwkSJ3a9AgMBAAECggEAEwgyNtQvwa/Zr2sD8uu6oSBIfEHaBR4caBUuqNnVHMpi\ngQTNRocL+W9qA9B74act2sd/xDw3lHw/eRyTRJ3jVlgEIhVPBdisoOgbgs5/04Qu\nwkb7yE6dxhGA1tGrzjLlGfJZMd0MWk7h4njTijC/nIGVWSXI4umXQI/m885PF+um\nkcbOLWo24V9cCVF6DJTxI0LKgNGtFeaj/qFTgT3QF1zEKPSBeqG5RqlwtXNvmM3k\nJy4wl4Iwn/EblXuGDCMQR9nGoN6j3Y+LHunsi8Gjqs1/yadC/niOv+Nqf5xoX5V3\nzTdk2pJTAHlbYPUDKVv6FQmDkwagIMAqF048wmKscQKBgQDs4wgxJaKlME2o9fd4\nAQb/ZCaHpn9kO3oDGLltveSguCLs0SJslN6G/aTiUOqg4jGSN64pnZEKdx8MJOS6\nz7GJf2OiYkZ5C921f89gQM9/AhmQiI790v46oJzgJmv9wGFjXkdHOws2jNLNegz9\nbF0/ABO4dpxSlXj9TEU9KMtSOQKBgQDm8zCJ1OIFL+ranLL3s72LI10DRdAUxtBl\n154S9+4WKfaA2+Sr3BU5rfpW7l0m743f8qRmi5ixo2cR1hQteIz7dmDkgGtlqEQh\n3NplEAHzvwK6D7Ysd5O6wG23zyTT2QQknOlZQdeU5KuatYztySDGKKuBkYXzu61F\nexUsOhc4pQKBgGxmgAstc74v2nnlBLePkMox5EfS8xzE8tKT79a+PO8nFCRWl3Ak\nt6gCTf+ak6PHnnOQs0wr5IFrfXOrNlgeTtnix485dZJS0cQKlrUvM0Sli1lOhFC2\nysu9T8xrCKP38xjrvaZk6H6v9o88uNEHU6xOtreE34gTCmivDEgucMxRAoGAPaQj\nDGa5fNDxSjAQxHRA3uYtaTY095apAMWv9zgdX+ULWhFW01gGgkKhUpqEWmQN5fwJ\nTSVtN9x5IhWVhR1r46IQ8mwkPhnPNYqQ7/B51OjifW68HNo9n9bEcg7jsXd2157Z\n2WztcVwnQT+7wauxB7LLM+X3brpk4OqdGBTZJdECgYB6y+iKrTZAcoC2b4hMkx/N\nUSw3osK834QVe7JuovEDZ+LQWtTJYAEHDMqFFLKavWr2aRVUIo4dd4Df5gmfQrfq\noRw9NlqwhJwSmcIGimx4ZDHGFTCTyjWnzXzRywfbTMm6oQEmWtv6ZRUh3/1Q02JB\nhEsDMnGoohm1Q67m/lLtxg==\n-----END PRIVATE KEY-----\n";
const char *clientEmail = "stormwater@home-automation-453101.iam.gserviceaccount.com";

WiFiClientSecure client;

String base64UrlEncode(const uint8_t *input, size_t length)
{
  size_t olen;
  unsigned char *output = (unsigned char *)malloc(length * 2); // Allocate enough memory

  if (output == NULL)
  {
    Serial.println("Memory allocation failed");
    return "";
  }

  mbedtls_base64_encode(output, length * 2, &olen, input, length);

  String encoded = String((char *)output, olen);
  free(output);

  // Base64URL encoding. remove padding and replace unsafe characters.
  String base64Url = "";
  for (int i = 0; i < encoded.length(); i++)
  {
    if (encoded[i] == '+')
    {
      base64Url += '-';
    }
    else if (encoded[i] == '/')
    {
      base64Url += '_';
    }
    else if (encoded[i] == '=')
    {
      // remove padding
    }
    else
    {
      base64Url += encoded[i];
    }
  }

  return base64Url;
}

String base64EncodeMessage(const String &message)
{
  size_t olen;
  unsigned char *output = (unsigned char *)malloc(message.length() * 2);
  if (output == NULL)
  {
    Serial.println("Memory allocation failed");
    return "";
  }
  mbedtls_base64_encode(output, message.length() * 2, &olen, (const unsigned char *)message.c_str(), message.length());
  String encoded = String((char *)output, olen);
  free(output);
  return encoded;
}

String generateJWT(const char *privateKey, const char *clientEmail)
{
  // Header
  JsonDocument headerDoc;
  headerDoc["alg"] = "RS256";
  headerDoc["typ"] = "JWT";
  String header;
  serializeJson(headerDoc, header);
  String encodedHeader = base64UrlEncode((const uint8_t *)header.c_str(), header.length());

  // Payload
  JsonDocument payloadDoc;
  payloadDoc["iss"] = clientEmail;
  payloadDoc["scope"] = "https://www.googleapis.com/auth/pubsub"; // Adjust scope
  // payloadDoc["aud"] = "https://" + String(googleCloudHost); // Adjust audience
  payloadDoc["aud"] = "https://oauth2.googleapis.com/token";
  ;

  payloadDoc["exp"] = time(nullptr) + 3600; // Expires in 1 hour
  payloadDoc["iat"] = time(nullptr);
  String payload;
  serializeJson(payloadDoc, payload);
  String encodedPayload = base64UrlEncode((const uint8_t *)payload.c_str(), payload.length());

  String unsignedToken = encodedHeader + "." + encodedPayload;

  // Sign
  mbedtls_pk_context pk;
  mbedtls_pk_init(&pk);
  mbedtls_pk_parse_key(&pk, (const unsigned char *)privateKey, strlen(privateKey) + 1, NULL, 0);

  uint8_t hash[32];
  mbedtls_sha256((const unsigned char *)unsignedToken.c_str(), unsignedToken.length(), hash, 0);

  size_t sig_len;
  uint8_t signature[256];
  mbedtls_pk_sign(&pk, MBEDTLS_MD_SHA256, hash, sizeof(hash), signature, &sig_len, NULL, NULL);
  mbedtls_pk_free(&pk);

  String encodedSignature = base64UrlEncode(signature, sig_len);

  return unsignedToken + "." + encodedSignature;
}

void setup()
{
  Serial.begin(115200);
  //logger.registerSerial(MYLOG, DEBUG, "tst"); // We want messages with DEBUG level and lower

  initWiFi();
  configTime(0, 0, "pool.ntp.org"); // Get time from NTP server
  while (time(nullptr) < 1000)
  {
    delay(500);
    Serial.print(".");
  }

  // format time to human readable string
  struct tm timeinfo;
  time_t now = time(nullptr);
  gmtime_r(&now, &timeinfo);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  pinMode(ULTRASONIC_TRIGGER, OUTPUT); // Set Trig as output
  pinMode(ULTRASONIC_ECHO, INPUT);     // Set Echo as input

}

String getAccessToken(const String &jwt)
{
  Serial.println("\n\n\n\nRequesting access token...");
  WiFiClientSecure client;
  // client.setCACert(caCert); // Use your CA certificate
  client.setInsecure(); // Remove for production!
  if (client.connect("oauth2.googleapis.com", 443))
  {
    String request = "POST /token HTTP/1.1\r\n"
                     "Host: oauth2.googleapis.com\r\n"
                     "Content-Type: application/x-www-form-urlencoded\r\n"
                     "Content-Length: ";

    String body = "grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Ajwt-bearer&assertion=" + jwt;
    request += String(body.length()) + "\r\n\r\n" + body;

    client.print(request);

    String response = "";
    String responsebody = "";
    bool headersFinished = false;

    while (client.connected())
    { //This is a rough method of parsing the response. It assumes the headers are finished when it encounters a blank line.
      if (!headersFinished)
      {
        String line = client.readStringUntil('\n');
        if (line == "\r")
        {
          headersFinished = true;
        }
        response += line + "\n";
      }
      else
      {
        String chunkSizeStr = client.readStringUntil('\n'); // discard chuck size
        responsebody = client.readString();
        responsebody = responsebody.substring(0, responsebody.length() - 4); //remove the traling '0'
        break;                 
      }
    }

    client.stop();

    // Parse the JSON response to extract the access token
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, responsebody);
    if (error)
    {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return "";
    }
    String accessToken = doc["access_token"].as<String>();

    return accessToken;
  }
  else
  {
    Serial.println("Connection to oauth2.googleapis.com failed");
    return "";
  }
}

void publishMessage(PubSubTopic topic, const JsonDocument &message)
{
  String jwt = generateJWT(privateKey, clientEmail);
  String accessToken = getAccessToken(jwt);

  client.setInsecure(); // Remove for production!
  if (client.connect(googleCloudHost, googleCloudPort))
  {
    String url = "/v1/projects/" + String(projectId) + "/topics/" + PubSubTopicNames[topic] + ":publish";
    String request = "POST " + url + " HTTP/1.1\r\n"
                                     "Host: " +
                     String(googleCloudHost) + "\r\n"
                                               "Authorization: Bearer " +
                     accessToken + "\r\n"
                                   "Content-Type: application/json\r\n"
                                   "Connection: close\r\n"
                                   "Content-Length: ";

    JsonDocument jsonDoc;
    //convert the message to a string
    String str_message;
    serializeJson(message, str_message);

    jsonDoc["messages"][0]["data"] = base64EncodeMessage(str_message);
    String json;
    serializeJson(jsonDoc, json);
    request += String(json.length()) + "\r\n\r\n" + json;

    client.print(request);

    while (client.connected())
    {
      String line = client.readStringUntil('\n');
      if (line == "\r")
      {
        break;
      }
    }
    while (client.available())
    {
      Serial.write(client.read());
    }
    client.stop();
    //print the message that was published
    Serial.print("Message published to ");
    Serial.println(PubSubTopicNames[topic]);
    Serial.println(str_message);
  }
  else
  {
    Serial.println("Connection failed");
  }
}

void sendHeartbeat();

void loop()
{
  String text;
  PowerReading reading = getPowerReading();
  int ultrasonicReading = ReadFromUltrasonic();
  WriteToDisplay(reading, ultrasonicReading);

  // if 10 seconds have past since the last reading, publish the message
  static long lastPublish = 0;
  if (SecondsSinceStart() - lastPublish > 10)
  {
    publishMessage(telemetry, reading.toJson());
    lastPublish = SecondsSinceStart();
  }

  sendHeartbeat();
  delay(1000);
  sleep(1);

}
//method that sends a json message containing the device id and the time to the heartbeat topic using the publishmessage method
void sendHeartbeat()
{
  //wait 5 minutes
  static long lastHeartbeat = 0;
  if (SecondsSinceStart() - lastHeartbeat > 300 or lastHeartbeat == 0)  
  {
    //create a json object
    JsonDocument doc;
    //add esp32 device id to json object and the current time
    int chip_id = ESP.getEfuseMac();
    doc["device_id"] = chip_id;
    doc["device_label"] = "stormwater";
    doc["time"] = time(nullptr);
    if (lastHeartbeat == 0)
    {
      doc["status"] = "restart";
    }
    else
    {
      doc["status"] = "heartbeat";
    }
    publishMessage(heartbeat, doc);
    lastHeartbeat = SecondsSinceStart();
    //logger.log(MYLOG, DEBUG, "Heartbeat Sent at: %d", lastHeartbeat);
    Serial.println("Heartbeat Sent at " + lastHeartbeat); 
  }
}

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
  if (currentTime % (currentDevice.PowerCycleTime()))
  {
    if (reading.exporting())
    {

      if (PowerDraw() < (reading.currentExportPower()) && currentDevice.CanActivate())
      {
        // turn on power
        currentDevice.Activate();
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
