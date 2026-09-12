#include <WiFi.h>
#include <HTTPClient.h>

const char *WIFI_SSID = "WIFI_SSID";
const char *WIFI_PASSWORD = "WIFI_PASSWORD";

const char *SERVER_URL = "http://192.168.1.100:5000";

#define RXD2 16
#define TXD2 17
HardwareSerial MegaSerial(2);

String incomingLine = "";
int plasticCount = 0;
int metalCount = 0;
int paperCount = 0;
int unknownCount = 0;

void setup() {
  Serial.begin(115200);
  MegaSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);

  connectToWiFi();
  Serial.println("[ESP32] Ready. Listening for sort events from Mega...");
}

void loop() {
  if (MegaSerial.available()) {
    incomingLine = MegaSerial.readStringUntil('\n');
    incomingLine.trim();

    if (incomingLine.startsWith("SORTED:")) {
      String label = incomingLine.substring(String("SORTED:").length());
      handleSortEvent(label);
    }
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }
}

void connectToWiFi() {
  Serial.print("[ESP32] Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[ESP32] WiFi connected. IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n[ESP32] WiFi connection failed, will retry in loop().");
  }
}

void handleSortEvent(String label) {
  Serial.println("[ESP32] Sort event received: " + label);

  if (label == "PLASTIC") plasticCount++;
  else if (label == "METAL") metalCount++;
  else if (label == "PAPER") paperCount++;
  else unknownCount++;

  sendLogToServer(label);
}

void sendLogToServer(String label) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ESP32] WiFi not connected, skipping log upload.");
    return;
  }

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"label\":\"" + label +
                    "\",\"plastic_count\":" + String(plasticCount) +
                    ",\"metal_count\":" + String(metalCount) +
                    ",\"paper_count\":" + String(paperCount) +
                    ",\"unknown_count\":" + String(unknownCount) + "}";

  int responseCode = http.POST(payload);
  Serial.println("[ESP32] Log POST response code: " + String(responseCode));

  http.end();
}
