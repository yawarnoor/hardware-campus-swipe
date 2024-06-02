#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

// RFID Configuration
#define SS_PIN 15 // D8
#define RST_PIN 2 // D4
MFRC522 mfrc522(SS_PIN, RST_PIN);

// WiFi Configuration
const char *ssid = "Virus";
const char *password = "03033793701";

// Backend API Endpoint
const char *apiEndpoint = "http://192.168.100.9:3000"; // Replace with your actual backend endpoint

// Device ID
const char *deviceID = "LAB-001";

bool wifiConnected = false;

void connectToWiFi()  {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting to WiFi...");
    }

    Serial.println("Connected to WiFi");
    wifiConnected = true;
    delay(5000);  // Introduce a delay after successful connection
  }
}

String fetchCMSID(String cardUID) {
  // Ensure WiFi is connected
  connectToWiFi();

  // Use HTTPClient for the POST request
  HTTPClient http;
  WiFiClient wifiClient;

  // Prepare the POST data
  String postData = "{\"card_id\":\"" + cardUID + "\"}";

  // Make the POST request to fetch CMS_ID
  String endpoint = String(apiEndpoint) + "/cards/fetchCMSID";
  http.begin(wifiClient, endpoint);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(postData);

  String cmsID = ""; // Variable to store the CMS_ID

  if (httpCode > 0) {
    Serial.printf("HTTP POST request successful, response code: %d\n", httpCode);
    String payload = http.getString();
    Serial.println("Response payload: " + payload);

    // Parse the JSON response to extract CMS_ID
    // For simplicity, assuming the response is {"cms_id": "your_cms_id"}
    int startIndex = payload.indexOf("cms_id") + 9;
    int endIndex = payload.indexOf("\"", startIndex);
    cmsID = payload.substring(startIndex, endIndex);
    Serial.println("CMS_ID: " + cmsID);
  } else {
    Serial.printf("HTTP POST request failed, error code: %s\n", http.errorToString(httpCode).c_str());
  }

  return cmsID;
}

void sendTransaction(String cmsID) {
  // Ensure WiFi is connected
  connectToWiFi();

  // Use HTTPClient for the POST request
  HTTPClient http;
  WiFiClient wifiClient;

  // Prepare the POST data
  String postData = "{\"cms_id\":\"" + cmsID + "\", \"device_id\":\"" + deviceID + "\"}";

  // Make the POST request to the /transaction endpoint
  String endpoint = String(apiEndpoint) + "/transactions";
  http.begin(wifiClient, endpoint);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(postData);

  if (httpCode == HTTP_CODE_OK) {
    Serial.printf("Transaction recorded successfully, response code: %d\n", httpCode);
    String payload = http.getString();
    Serial.println("Response payload: " + payload);
  } else {
    Serial.printf("HTTP POST request failed for transaction, error code: %s\n", http.errorToString(httpCode).c_str());
    Serial.println("Request body: " + postData);
  }
}

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

      // Fetch CMS_ID
      String cmsID = fetchCMSID(cardUID);

      // Print Message
      if (!cmsID.isEmpty()) {
        Serial.println("Access granted!");

        // Send Transaction
        sendTransaction(cmsID);
      } else {
        Serial.println("Card does not belong to any student.");
      }

      // Clear the card details
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
    }
  }

  delay(500); // Adjust delay as needed
}
