#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <mbedtls/sha256.h>
#include <mbedtls/base64.h>

const char* ssid = "scoltock";
const char* password = "nowireshere";
const char* googleCloudHost = "pubsub.googleapis.com";
const int googleCloudPort = 443;
const char* projectId = "home-automation-453101";
const char* topicId = "telemetry";

void publishMessage(const String& message);


// WARNING: Hardcoding keys is extremely insecure. Use SPIFFS or a secure element in production.
const char* privateKey = "-----BEGIN PRIVATE KEY-----\nMIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDkML8ELU9Uc+CW\nLw6Yw6bseMjHDOgVaYHFbTRZws9ey6LqEphPm+W9WXQpZfPD75yAdDWplHYplNNV\nL+VEdlSj2IVvy0FQ/CPHnQI77awfWx+Glo8YzwvglFti8AhFcitpydiAsO4bhcad\nsGp2UPv6s/PasUHjul3eEL7xqesLW/4y4ZfbP1MPVWHvOfTPIzVs7tlZck3GTMz1\nF7jIkGFiRXtIQ3NjRuJtkWjKOJDGgPbIHTT/TJLQJANjk+GyFffDqKmspj1MYLDo\np8CJypkifGWK/Cr7x1xoPEJZmR0/HW4QJR0soowbpsSezF6C83Ey9KyC2kmlRqOt\nox9ek8BFAgMBAAECggEABSZQeUCohKhkzh7bvnjT4IxNQ6i36BvMcO8o+kmaMnJ4\nB75Jr6o+Agrtwy1o1egnaQK2X1BZZlDUgBynmObuvA/b2w0EOVanXA7gCQgxFTgS\nOxBUxGOPyg6vDUmLdj3AWw/SEf1sMjTPqalcjbUAKsU64JmVehUvnx5ToNKL2mK7\nLiBv7EfcOJEbcT6x5NAateLvoix+S+OqZmEJfePnpea1E/sNlC58BYOejuP3hfO4\n/nT3U2nZe2JatyUJRG7T4nL+SSumU9FstjRsMdgrbNs8pn/aqPobXK9KxKFKWnLn\nfs0cvbBg4CNO5BQkJZufmz2faz8o8CJHibvTNpp84QKBgQDylp52efz72Hw6RIqp\nBb3rdyIkw3qS1yFK+yLJczW2+hdYnJ5o9IfCOe5YCmiQTzo6AVhmNxKNAbeIXrRC\n8TrJk71hwwhj2AUhDmZ7xhKTLkT94iRa8AuhG+q0nJ52cxgsvzkuRbzrIDQqMHQo\nCdSPa8a7gAC1uejJNeKVMZdZ2QKBgQDwzln4O7x0kMxYPnrTZrwW5nSvjBRhugm0\ngsuWuVTcSljkaYBsA8QcOIuvjry3m+3HTcksL1uexwJb8xaXZwGNKma9aYqM3iiU\n77Hhiauv7HxhTQSdiGX+UerVPOHnDvY0u0RJzv1q44eKyTYTNMo0MT5xo7ktkoip\n08XqKp1KTQKBgQCxIkYHIMAXbVYYfd/511V8T/tAePRRsICA7avTCSsZtrfBXmtV\nG+jda8ubwc3kQdW0cTMJQAFtsOlAzFY340kX06cfcXf4382u+4Ldsh3yFoEOGUiO\nlmUHeQkCZLJYr2Xhlqe8H/P4hAVTIRpP+g2ZxArS8n6SM3PFKHGQmyt20QKBgC8l\nR2oZT/FGdCuGuAdxorudDRVdiJQFl2bXAMMWr+2dRMAbg8AiEKAx1e9eM8aC6c/O\ng+d5sido8SZNdovX5+7acVj+M9kWmb1nHF161blQxJ1MNe4dyVI8eYycKeJSBJQY\nYetNckIeH+hlMogaGsiTJ5WaJ2Qrv0P4qVQKWdc1AoGBANhMwiJz8lITG0f1RXxA\nnxFQy+gHVEyFkd5CXSkP0b59T5gZXkCwX63CR8nNzxqK+sLBZsE33efO7dVySXix\n+xMmxZPmlMHnaRhp/9IPG1BMDDO0jXkCNhowZGw/rxR9QwsD3/KEM3yqC1abWV/r\nz+9uDXc2YWKuo24u7OHwDLOP\n-----END PRIVATE KEY-----\n";
const char* clientEmail = "stormwater@home-automation-453101.iam.gserviceaccount.com"; 

WiFiClientSecure client;

String base64UrlEncode(const uint8_t* input, size_t length) {
  size_t olen;
  unsigned char* output = (unsigned char*)malloc(length * 2); // Allocate enough memory

  if (output == NULL) {
    Serial.println("Memory allocation failed");
    return "";
  }

  mbedtls_base64_encode(output, length * 2, &olen, input, length);

  String encoded = String((char*)output, olen);
  free(output);

  //Base64URL encoding. remove padding and replace unsafe characters.
  String base64Url = "";
  for(int i = 0; i < encoded.length(); i++){
    if(encoded[i] == '+'){
      base64Url += '-';
    }else if(encoded[i] == '/'){
      base64Url += '_';
    }else if(encoded[i] == '='){
      //remove padding
    }else{
      base64Url += encoded[i];
    }
  }

  return base64Url;
}

String base64EncodeMessage(const String& message){
  size_t olen;
  unsigned char* output = (unsigned char*)malloc(message.length() * 2);
  if (output == NULL) {
    Serial.println("Memory allocation failed");
    return "";
  }
  mbedtls_base64_encode(output, message.length() * 2, &olen, (const unsigned char*)message.c_str(), message.length());
  String encoded = String((char*)output, olen);
  free(output);
  return encoded;
}

String generateJWT(const char* privateKey, const char* clientEmail) {
  // Header
  JsonDocument headerDoc;
  headerDoc["alg"] = "RS256";
  headerDoc["typ"] = "JWT";
  String header;
  serializeJson(headerDoc, header);
  String encodedHeader = base64UrlEncode((const uint8_t*)header.c_str(), header.length());

//print raw header and then encoded header
  Serial.println("Header:");
  Serial.println(header);
  Serial.println("Encoded Header:");
  Serial.println(encodedHeader);  

  // Payload
  JsonDocument payloadDoc;
  payloadDoc["iss"] = clientEmail;
  payloadDoc["scope"] = "https://www.googleapis.com/auth/pubsub"; // Adjust scope
  payloadDoc["aud"] = "https://" + String(googleCloudHost); // Adjust audience
  payloadDoc["exp"] = time(nullptr) + 3600; // Expires in 1 hour
  payloadDoc["iat"] = time(nullptr);
  String payload;
  serializeJson(payloadDoc, payload);
  String encodedPayload = base64UrlEncode((const uint8_t*)payload.c_str(), payload.length());

//print payload and encded payload
  Serial.println("Payload:");
  Serial.println(payload);
  Serial.println("Encoded Payload:");
  Serial.println(encodedPayload); 

  String unsignedToken = encodedHeader + "." + encodedPayload;

  // Sign
  mbedtls_pk_context pk;
  mbedtls_pk_init(&pk);
  mbedtls_pk_parse_key(&pk, (const unsigned char*)privateKey, strlen(privateKey) + 1, NULL, 0);



  uint8_t hash[32];
  mbedtls_sha256((const unsigned char*)unsignedToken.c_str(), unsignedToken.length(), hash, 0);

  size_t sig_len;
  uint8_t signature[256];
  mbedtls_pk_sign(&pk, MBEDTLS_MD_SHA256, hash, sizeof(hash), signature, &sig_len, NULL, NULL);
  mbedtls_pk_free(&pk);
 //log that key has been signed  =
 Serial.println("Key has been signed Step 3");

  String encodedSignature = base64UrlEncode(signature, sig_len);

//log key has been encoded
  Serial.println("Key has been encoded");

  // log unsighed token and encoded signature
  Serial.println("Unsigned Token:");
  Serial.println(unsignedToken);
  Serial.println("Encoded Signature:");
  Serial.println(encodedSignature);

  return unsignedToken + "." + encodedSignature;
}

void setup() {
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  configTime(0, 0, "pool.ntp.org"); // Get time from NTP server
  while (time(nullptr) < 1000) {
    delay(500);
    Serial.print(".");
  }

  //format time to human readable string
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

void publishMessage(const String& message) {
  String jwt = generateJWT(privateKey, clientEmail);
  client.setInsecure(); // Remove for production!
  if (client.connect(googleCloudHost, googleCloudPort)) {
    String url = "/v1/projects/" + String(projectId) + "/topics/" + String(topicId) + ":publish";
    String request = "POST " + url + " HTTP/1.1\r\n"
                     "Host: " + String(googleCloudHost) + "\r\n"
                     "Authorization: Bearer " + jwt + "\r\n"
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
  }
}

void loop() {
}