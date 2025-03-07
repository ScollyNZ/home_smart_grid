#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <mbedtls/sha256.h>
#include <mbedtls/base64.h>

const char* ssid = "scoltock";
const char* password = "nowireshere";
const char* googleCloudHost = "timeseriesinsights.googleapis.com"; // Replace with your API host
const int googleCloudPort = 443;

// WARNING: Hardcoding keys is extremely insecure. Use SPIFFS or a secure element in production.
const char* privateKey = "-----BEGIN PRIVATE KEY-----\n"
                         "MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDWuvPS5TbgZjSv\n2GQcI1/5vbHGv4esF4ty5XIaSI1Q45hWsdccLj3ktIoGqj1MrUq8Kf5rNmShDJ0n\noeeUb+73775jqJauENxEoj1zAPnJ9JuoSLFwkEbNL/avkZKxIYIzSMkf5Lh56G7w\na41xgEB6IGkEBn1Icl9VC7x2edwuT2VkAXVAfJ2o9k11jgy5saZbCKsFCuQvLQZT\n4lu7jcUrmQ/mG3tzzI1703nku524KA4MZcEQdaA4LQBdnF5AjpJYBtlXx7BHUt6O\ni6TAhKhobHxVhT1LwqFc8exS+xUXdGeReu2l4nwi6C1GdgryJzGipQ/zKJ/5dKPR\n7IGKTXnzAgMBAAECggEAObT3ZtNz2n2NJj7MtKUrG1ZH6wY5A142ezO/ZOHfnH/1\nxkK38RpaRtbgCyeAHtWpcoUEmzJ8e62EpIyFzGYCfj4/V9AMYo4kbAt/4SK/fm9O\nz7xtdzdG18jOjmNcXV8IexjR+FDC5Pw6qED3O6wAZ/HC1zKZQ7l89kGXre1U51qv\n4p8Jw4j72i9GjV9IqJbA8ko/3aaeg5TsXs3bFFEr05YQ4xpo1S1mXhNJFALFbpG2\nuRIH4kmBXWy7z8tZwC09VtF5D+s60gphmsBUspCOVzrd0UFCkkEeZA6V3GZWxSqi\nDle/5i4aDjTfJvcvQZpyBgOTZfj7A/vYup+iHL9JxQKBgQDz05wyDmbzmJMl+DTo\n6htt45s3WPX6TyH5z144nVddUJFW/nucim9ch00/bjMangqiquf+/Ev+u1VI7hmB\niTxSs9toQCGVHH5cMFwnOZ20rzezJn/5oFGYIYIqV3OB1H2IHaBWhgPOuBVjsmIj\ncrbS0vLUFqekDEUOYzgXmcWKvQKBgQDhc3UMn1dzsz6NJW11pUEnOE5O3QlGmzwZ\nMGBTBfbbIyIcd80pxChxTXSPBivucj8fRyv0fxtA2+/zUApOoKWI4C5DaPf6oj2U\n5Bk+vJk4M5qAhVJgL/o1kayAUf8FwTXsS3EXQLCtdNbSrgZ8Mf4qfiGCQRiSNog6\n+xKPzE26bwKBgQCQ946VgFSbnmpWl+U1WgNm6X1egej1otjrguxdMsc/tuhC1dk+\nsFwXomY+QVHnEHH4Vy7KPs0cLds+GjpV9vdDoKXhrMeKtT83ppUTyUDHramrPUe3\nbic1ES8n54jId0MPi7XJ27Il1PL6rJOGyeyDGmK/0Jxpf9YLANjXx1hY7QKBgHS+\nxgEITk/ipOSQWhNOxONbW+moulHutvtQOsEjWIZ3tgVJ4FrdchfiBRa/Gma6kIdQ\n4qkUXPeELMgxTXUT6URs2mgb5jXKZ9s/FveO0ETzK/GbmGGo1oeA7PPyAf5n49V9\n540j+ZmI5Glqpn8PoE8+y3lY3jFwvbDVLPJ72FBPAoGBALxift5dGhViAwGQXI0n\nhDsb90J4ih0cYR8jYoZ9ts5RsitGSNdqsO/DvNfRTjLGz0M6Vq6sXW3neiiSItO8\nn1o1H4kDHWYNmPji1JfviVOYKASg5nLIk6zF1laMu8eTSPec4uYg02mBPbo6aPA8\nZ9bvQ4t8hNidtVCwmuqdAXu0\n"
                         "-----END PRIVATE KEY-----\n";
const char* clientEmail = "stormwater-esp@clean-room-client.iam.gserviceaccount.com"; 

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