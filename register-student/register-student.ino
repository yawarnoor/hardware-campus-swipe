#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>

// RFID Configuration
#define SS_PIN 15 // D8
#define RST_PIN 2 // D4
MFRC522 mfrc522(SS_PIN, RST_PIN);

// WiFi Configuration
const char *ssid = "Virus";
const char *password = "03033793701";

// Backend API Endpoint
const char *apiEndpoint = "192.168.91.39:3000/cards";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");

  // Initialize RFID reader
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("RFID reader initialized");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    // Look for new RFID cards
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      Serial.println("Card detected!");

      // Read card UID
      String cardUID = "";
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        cardUID += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
        cardUID += String(mfrc522.uid.uidByte[i], HEX);
      }

      // Ask user for student ID
      Serial.print("Please enter the CMS ID of the student: ");
      while (!Serial.available()) {
        delay(10);
      }
      String cmsID = Serial.readStringUntil('\n');
      
Serial.println("Card UID: " + cardUID);
Serial.println("CMS ID: " + cmsID);

// Send POST request to backend
HTTPClient http;
WiFiClient wifiClient;
http.begin(wifiClient, "http://" + String(apiEndpoint));
http.setReuse(true); // Set connection to reuse
http.addHeader("Content-Type", "application/json");  // Set content type to JSON

// Prepare JSON data
DynamicJsonDocument jsonDocument(200);  // Adjust the size as needed
jsonDocument["card_id"] = cardUID;
jsonDocument["cms_id"] = cmsID;

String postData;
serializeJson(jsonDocument, postData);

// Send POST request
int httpCode = http.POST(postData);

if (httpCode > 0) {
  Serial.printf("HTTP POST request successful, response code: %d\n", httpCode);
  String payload = http.getString();
  Serial.println("Response payload: " + payload);
} else {
  Serial.printf("HTTP POST request failed, error code: %s\n", http.errorToString(httpCode).c_str());
}

http.end();


      // Clear the card details
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
    }
  }

  delay(500);  // Adjust delay as needed
}
