#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <mbedtls/sha256.h>
#include <mbedtls/base64.h>

const char *ssid = "scoltock";
const char *password = "nowireshere";
const char *googleCloudHost = "pubsub.googleapis.com";
const int googleCloudPort = 443;
const char *projectId = "home-automation-453101";
const char *topicId = "telemetry";

void publishMessage(const String &message);
String getAccessToken(const String &jwt);

// WARNING: Hardcoding keys is extremely insecure. Use SPIFFS or a secure element in production.

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

  // print raw header and then encoded header
  Serial.println("Header:");
  Serial.println(header);
  Serial.println("Encoded Header:");
  Serial.println(encodedHeader);

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

  // print payload and encded payload
  Serial.println("Payload:");
  Serial.println(payload);
  Serial.println("Encoded Payload:");
  Serial.println(encodedPayload);

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
  // log that key has been signed  =
  Serial.println("Key has been signed Step 3");

  String encodedSignature = base64UrlEncode(signature, sig_len);

  // log key has been encoded
  Serial.println("Key has been encoded");

  // log unsighed token and encoded signature
  Serial.println("Unsigned Token:");
  Serial.println(unsignedToken);
  Serial.println("Encoded Signature:");
  Serial.println(encodedSignature);

  return unsignedToken + "." + encodedSignature;
}

void setup()
{
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
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

  Serial.print(asctime(&timeinfo));

  Serial.println("Private Key: ");
  Serial.println(privateKey);
  /*

   String jwt = generateJWT(privateKey, clientEmail);

   Serial.println("JWT:");
   Serial.println(jwt);
   */

  /*
  client.setInsecure(); //for testing only. Remove this for production. Use client.setCACert() for proper certificate validation.
  if (client.connect(googleCloudHost, googleCloudPort)) {
    String request = "GET /v1/projects/clean-room-client/datasets HTTP/1.1\r\n"
                     "Host: " + String(googleCloudHost) + "\r\n"
                     "Authorization: Bearer " + jwt + "\r\n"
                     "Connection: close\r\n\r\n";
    client.print(request);

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        break;
      }
      Serial.println(line);
    }
    while (client.available()) {
      Serial.write(client.read());
    }
    client.stop();
  } else {
    Serial.println("Connection failed");
  }*/
  publishMessage("Hello from ESP32!");
  Serial.println("Message published");
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

    Serial.println("Request:");
    Serial.println(request);

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

    Serial.println("\n\n\nResponse Body:");
    Serial.println(responsebody);

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
    Serial.println("Access Token:");
    Serial.println(accessToken);
    Serial.println("\n\n\n\n\n\n");
    return accessToken;
  }
  else
  {
    Serial.println("Connection to oauth2.googleapis.com failed");
    return "";
  }
}

void publishMessage(const String &message)
{
  String jwt = generateJWT(privateKey, clientEmail);
  // Access Token - Works
  // String jwt = "ya29.c.c0ASRK0GYQYnOwsMnM_4xx3lZc3F4Otw6V5iONr2lyKBEoY97I1WFf7j798A_FUDG3vHV9lFLmqEv67VEy1vDvCm95QpJedT2cNt_eRzzqJ-ndXKo-VM3pugp1Vie37-Vp1JQYEQHyE6xdhNucu9wT0lX_8BZJQSYqE6i5uMZ9i51lReHncJfSaZKOMrzzD5H3ArKvmwNWXYXzarTX0zB9gCSOx4ANBGquQIHfawJ6pYpvdtfey_TeA5-npfeEj0q5sDNhHqrfe0JE7uifAwip8kWdQpE622Bej9WdcvlEtYD1LlLfG48-bY0eL42YyM68sl0-t-_u_PRlqDFaaHQSE3kdOWbMHYAGvDkrq7W4HBMbU1r60qFG00jZGihoNnvW1OMT396Av-Rozyi9vjexOfQ0s5k5BsOSlZj0tUZ7uq2iIwW-XZSssOR-d9Mgbx29yow_crMq6pvgV3J6YzlX2d0kaegBghJSiFyBFWe6Y4yFc_hg_YdsO8ljRa167UOhMauX2Fspep9mdR_JSpociXuhIFoYemlZjeRv0hnQsxVjkrdb5udaQY9bY6j2U7F5xeQwevdWluySt3fnrXshdabfwZj97yFs6w5oSijXkUi5R3qym9z61l8XvBRuxRFJB4tiuiWdW_utveYt2OdukjJxo3BvcjuRfO14v0XURbjaviacMROn7o_Jts3YsFec3o844lOziQRgntYZvxwrg31gi0v2rcnQ3RtqFuv_9-ypqB54n0lhuWdhBIuahuJzJbdkyWYitwSQerx7XXr3USh5ysuh2tmrRpkXnZIaehew3biY-cui1UBXqkO7aSbmbxY7QJbuysxYnxRVvlW14ztUiFmM0177hot_pzh4Ywa7vwBr67Wfzx31jhayWkbJ6BykXq38tzQ3xJyVyRrS-QxBUZe5kjecrfUgUeMyOppiYrJ3y-dOxO9OQ2fO5Q3e6acI43e_XBMF1cmOQ_r9SnxB73m7akvOsX_3mO8l0i22tu72x4Jl1lo";
  // Identity Token - Doesn't work
  // String jwt = "eyJhbGciOiJSUzI1NiIsImtpZCI6IjI1ZjgyMTE3MTM3ODhiNjE0NTQ3NGI1MDI5YjAxNDFiZDViM2RlOWMiLCJ0eXAiOiJKV1QifQ.eyJhdWQiOiIzMjU1NTk0MDU1OS5hcHBzLmdvb2dsZXVzZXJjb250ZW50LmNvbSIsImF6cCI6InN0b3Jtd2F0ZXJAaG9tZS1hdXRvbWF0aW9uLTQ1MzEwMS5pYW0uZ3NlcnZpY2VhY2NvdW50LmNvbSIsImVtYWlsIjoic3Rvcm13YXRlckBob21lLWF1dG9tYXRpb24tNDUzMTAxLmlhbS5nc2VydmljZWFjY291bnQuY29tIiwiZW1haWxfdmVyaWZpZWQiOnRydWUsImV4cCI6MTc0MTQwNjA3OCwiaWF0IjoxNzQxNDAyNDc4LCJpc3MiOiJodHRwczovL2FjY291bnRzLmdvb2dsZS5jb20iLCJzdWIiOiIxMDIzMjAwOTYzMTUzODY4MTM3OTUifQ.Ak45S-b_GCnMdoJOC2FPNHiW3UrCsTvhsg5ykIN_nmwJg1hLgwNolWB15j-ahrH5L7g6F6cTjsbe1j77P_cfpClPl_6Mjlh49qMhk2JQwlr4U_quXiVOHbm0RfuWc9E1pJ5HCsM1qCGVo_BucY4MT8luZ5wh94I1avQaYQlhzm8RC9PrPo7XVKx1YcKJ7Nanl6b9LwsLyrfbjfQuvRk8pF39sNmq2XfjnoHOF7eQSFx1CHJv__myIc5CoBsIw1FcGJ-O3UFWoAsTCe4GwYU39I-Qo--i6FT2VrwJ-ARuR0iYih10zqFxYX-snq-uUUiazyiW7ueAr7TrKmKFVy65gQ";

  String accessToken = getAccessToken(jwt);
  Serial.println("Access Token:");
  Serial.println(accessToken);

  client.setInsecure(); // Remove for production!
  if (client.connect(googleCloudHost, googleCloudPort))
  {
    String url = "/v1/projects/" + String(projectId) + "/topics/" + String(topicId) + ":publish";
    String request = "POST " + url + " HTTP/1.1\r\n"
                                     "Host: " +
                     String(googleCloudHost) + "\r\n"
                                               "Authorization: Bearer " +
                     accessToken + "\r\n"
                                   "Content-Type: application/json\r\n"
                                   "Connection: close\r\n"
                                   "Content-Length: ";

    JsonDocument jsonDoc;
    jsonDoc["messages"][0]["data"] = base64EncodeMessage(message);
    String json;
    serializeJson(jsonDoc, json);
    request += String(json.length()) + "\r\n\r\n" + json;

    Serial.println("Request:");
    Serial.println(request);

    client.print(request);

    while (client.connected())
    {
      String line = client.readStringUntil('\n');
      if (line == "\r")
      {
        break;
      }
      Serial.println(line);
    }
    while (client.available())
    {
      Serial.write(client.read());
    }
    client.stop();
  }
  else
  {
    Serial.println("Connection failed");
  }
}

void loop()
{
}