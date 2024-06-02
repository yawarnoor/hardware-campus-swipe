#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

// RFID Configuration
#define SS_PIN 15 // D8
#define RST_PIN 2 // D4
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Device ID
const char *deviceID = "BUS-002";

// WiFi Configuration
const char *ssid = "Virus";
const char *password = "03033793701";

// Backend API Endpoint
const char *apiEndpoint = "http://192.168.91.39:3000"; // Replace with your actual backend endpoint

bool wifiConnected = false;

void connectToWiFi() {
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
    // the response is {"cms_id": "your_cms_id"}
    int startIndex = payload.indexOf("cms_id") + 9;
    int endIndex = payload.indexOf("\"", startIndex);
    cmsID = payload.substring(startIndex, endIndex);
    Serial.println("CMS_ID: " + cmsID);
  } else {
    Serial.printf("HTTP POST request failed, error code: %s\n", http.errorToString(httpCode).c_str());
  }

  return cmsID;
}

bool checkSubscription(String cmsID) {
  // Ensure WiFi is connected
  connectToWiFi();

  // Use HTTPClient for the POST request
  HTTPClient http;
  WiFiClient wifiClient;

  // Prepare the POST data
  String postData = "{\"cms_id\":\"" + cmsID + "\"}";

  // Make the POST request to check subscription
  String endpoint = String(apiEndpoint) + "/checkSubscription"; // Update with your actual endpoint
  http.begin(wifiClient, endpoint);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(postData);

  bool isSubscribed = false; // Variable to store the subscription status

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Response payload: " + payload);

    // Parse the JSON response to extract subscription status
    // Assuming the response is {"is_subscribed": true}
    if (payload.indexOf("true") != -1) {
      isSubscribed = true;
      Serial.println("Subscription status: true");
    } else if (payload.indexOf("false") != -1) {
      isSubscribed = false;
      Serial.println("Subscription status: false");
    } else {
      Serial.println("Invalid response format: " + payload);
    }
  } else {
    Serial.printf("HTTP POST request failed, error code: %s\n", http.errorToString(httpCode).c_str());
  }

  return isSubscribed;
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

      // Step 2: Fetch CMS_ID
      String cmsID = fetchCMSID(cardUID);

      // Step 3: Fetch User
      bool isSubscribed = checkSubscription(cmsID);

      // Step 4: Print Message
      if (isSubscribed) {
        Serial.println("Access granted!");

        sendTransaction(cmsID);
      } else {
        Serial.println("Not Subscribed");
      }



      // Clear the card details
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
    }
  }

  delay(500); // Adjust delay as needed
}

