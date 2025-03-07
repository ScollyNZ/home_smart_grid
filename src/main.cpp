#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <mbedtls/sha256.h>

const char* ssid = "scoltock";
const char* password = "nowireshere";
const char* googleCloudHost = "timeseriesinsights.googleapis.com"; // Replace with your API host
const int googleCloudPort = 443;

// WARNING: Hardcoding keys is extremely insecure. Use SPIFFS or a secure element in production.
const char* privateKey = "-----BEGIN PRIVATE KEY-----\n"
                         "YOUR_PRIVATE_KEY_HERE\n"
                         "-----END PRIVATE KEY-----\n"; // Replace with your private key
const char* clientEmail = "your-service-account-email@your-project.iam.gserviceaccount.com"; // Replace with your service account email

WiFiClientSecure client;

String base64UrlEncode(const uint8_t* input, size_t length) {
  String encoded = "";
  static const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

  for (size_t i = 0; i < length; i += 3) {
    uint32_t val = input[i];
    val <<= 8;
    if (i + 1 < length) val |= input[i + 1];
    val <<= 8;
    if (i + 2 < length) val |= input[i + 2];

    for (int j = 0; j < 4; j++) {
      if (i * 8 + j * 6 > length * 8) break;
      encoded += base64_chars[(val >> (18 - j * 6)) & 0x3F];
    }
  }

  while (encoded.length() % 4) {
    encoded += ""; // base64url doesn't pad with '='
  }
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

  // Payload
  JsonDocument payloadDoc;
  payloadDoc["iss"] = clientEmail;
  payloadDoc["scope"] = "https://www.googleapis.com/auth/cloud-platform"; // Adjust scope
  payloadDoc["aud"] = "https://" + String(googleCloudHost); // Adjust audience
  payloadDoc["exp"] = time(nullptr) + 3600; // Expires in 1 hour
  payloadDoc["iat"] = time(nullptr);
  String payload;
  serializeJson(payloadDoc, payload);
  String encodedPayload = base64UrlEncode((const uint8_t*)payload.c_str(), payload.length());

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

  String encodedSignature = base64UrlEncode(signature, sig_len);

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

  String jwt = generateJWT(privateKey, clientEmail);

  Serial.println("JWT:");
  Serial.println(jwt);  

  client.setInsecure(); //for testing only. Remove this for production. Use client.setCACert() for proper certificate validation.
  if (client.connect(googleCloudHost, googleCloudPort)) {
    String request = "GET /v1/your-resource HTTP/1.1\r\n"
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
  }
}

void loop() {
}