#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <mbedtls/sha256.h>
#include <mbedtls/base64.h>

const char* ssid = "scoltock";
const char* password = "nowireshere";
const char* googleCloudHost = "pubsub.googleapis.com";
const int googleCloudPort = 443;
const char* projectId = "home-automation";
const char* topicId = "telemetry";

// WARNING: Hardcoding keys is extremely insecure. Use SPIFFS or a secure element in production.
const char* privateKey = "-----BEGIN PRIVATE KEY-----\n"
                         "MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCbhDvRsKy+lyqI\nVSpbwx1IikwLgMJFnaJm7+xOJDPBaC22FlwVOhpgJzqedu/6kg3Ja6rCKrfCIU+B\nn5vsbRWDpAYU7A6N8smDADYX6A/kMqDUENVa7KsRMPjLZvdOLn0sHC43dUkWbX6f\n5LvYnmxR9mwcLdEhOgUVIbOWX5/G8uEt+S6esUkOruCYdc88gMlPQdQjo033Fx+V\nqqylbrCqVIGobd20z1LWqV3S6ahZ8xyTixRt7x9jy0mhp8nXH8l01WoiSAu32mAO\nzeZYrdPSjjEwwE1QVltSENHRpl1sSZgF0Me7geVye+dJwNyBwo/pSRpWT1sEHOUd\nTin65mbPAgMBAAECggEAMFkLWdTx12zakbjcatu9YlxwaNEWVJs9rn/feKmjl3jC\nYU4RQZ4nLgZ0IGi8katPcvOwih3KtCUz+Qn8aSO+oQbgxydeHP5CbKInrwX7zsd4\nnYVANzzFsBoH9wBjWSB11L1MFvy1BK50w9PSHHWgGlkgYiBSWGCXzOqF0PWYkDpc\nkWGjmZbagXhDFnFCNYIh6vLirtuqlHs6BBWtWxG2B82Zr2DOR8PENIIwm/IVm5OA\nh1y2/6K/yQ70c7Ncywcqs6DHoX+E5OghiOM/3VpPBWxWC54rJFbqXhCnRdvQvA9g\nPuJD+Q6U6veAAoQJ/pQt6fqm57Q8mjNZOWiwI8hc/QKBgQDYjQtjon4KSbTXX7Tl\ntzprhn6JcANL+tQ5VIkeaX7bIam7q1/l4XUDOfTLh8quie9l3xOo+EwekApqMHH4\nGTtf7uGP06fek+lohJMS+NYvUtnO4zQyS+uNwDPoAFXrpl1wWqfYvsZ+ljKEU2sM\nj6F5BXxQ35xepeHjbzoDjGSeWwKBgQC32NIJpH+88Lrg8v1nNV8Z1uaQzBWLlgy6\nBc3mgSezsMp30tfudSxctrP989zie++vJ76p25pHLFaGXNa/lk0hSPHbpeeYeB5L\ngOWbyWi0M6vFcgnxvFn7hoeAGQ7GEkocMZiV4v+w/4AWX6776IcSM4J+QmJXcCEr\nC85vI0grnQKBgCcTfISkI75TgpCCsq/pGl+gy2Cdl9q0Dmux93RHcR61ul0lFY7z\n2huU77lybX6FYW+ui2uoMoQpVdfHmik81FwBTS6Y8OfXJP05PxjjUjMD7k3I7fhN\nftn7XuJ0fQyi3qWRlkwkUMky7Ta3ns+Lc4XVGZO2Zg8mO9bAkWVmhhmVAoGABTH/\nXX5kjbZP0aOMPapocZZ9FYJu6W7oFSms0+K6eH16e0BeEcMF6ejP1VFa4JuX/l8l\nKC+ogHJkT4+4EdnxfAtPqmFZ1hku4ftWgbyDVPRQ2leKqGYmNNFsatZKcJZ7uag9\nI58ykpHl0LwLedrsVjtWaLL/jHECvvCqeMGKfJECgYEAlhnd25c8aJdvvBWPO+Qu\n97bX8+zHsX7VI5lnGRY/woEBP+28cuIlHWSPci+f4DC5vDrf2H8+S/5Z+nNHtNxq\nDLchqANHXZnhEDTtLhqYIqedBg4vngfsIvX16bmWgB8ogMDAa2yFS6Upw/2dKgln\nitTO6c1Qb1nnL1rWAmzI6d8=\n"
                         "-----END PRIVATE KEY-----\n";
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
  payloadDoc["scope"] = "https://www.googleapis.com/auth/cloud-platform"; // Adjust scope
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
  Serial.println(" UTC");

  String jwt = generateJWT(privateKey, clientEmail);

  Serial.println("JWT:");
  Serial.println(jwt);  

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
  }
}

void loop() {
}