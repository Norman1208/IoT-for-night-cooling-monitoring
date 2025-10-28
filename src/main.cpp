#include <WiFi.h>
#include <HTTPClient.h>
#include "DHTesp.h"
#include "time.h"

// ================== WiFi Config ==================
#define WIFI_SSID "Longi Kost"
#define WIFI_PASSWORD "eeee7777"
// #define WIFI_SSID "RKS ROBOTIK"
// #define WIFI_PASSWORD "RsupxRobotik2025"

// ================== Firebase Info ==================
#define DATABASE_URL "https://night-cooling-monitoring-default-rtdb.asia-southeast1.firebasedatabase.app/"

// ================== DHT Setup ==================
DHTesp dht1;
DHTesp dht2;
#define DHT_PIN1 32
#define DHT_PIN2 33

// ================== NTP Config ==================
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 8 * 3600;  // GMT+8
const int daylightOffset_sec = 0;

// ================== Helper Functions ==================
String getTimestamp(struct tm timeinfo) {
  char buffer[25];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}

void sendDataToFirebase(float t1, float h1, float t2, float h2, String timestamp) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(DATABASE_URL) + "/nightCooling/device1.json";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    // JSON payload
    String payload = "{";
    payload += "\"sensor1\":{\"temperature\":" + String(t1, 2) + ",\"humidity\":" + String(h1, 2) + "},";
    payload += "\"sensor2\":{\"temperature\":" + String(t2, 2) + ",\"humidity\":" + String(h2, 2) + "},";
    payload += "\"timestamp\":\"" + timestamp + "\"";
    payload += "}";

    int httpResponseCode = http.PUT(payload);

    if (httpResponseCode > 0) {
      Serial.print("✅ Data sent to Firebase: ");
      Serial.println(payload);
    } else {
      Serial.print("❌ Error sending: ");
      Serial.println(http.errorToString(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("⚠️ WiFi not connected!");
  }
}

// ================== Setup ==================
void setup() {
  Serial.begin(115200);

  dht1.setup(DHT_PIN1, DHTesp::DHT22);
  dht2.setup(DHT_PIN2, DHTesp::DHT22);

  // Connect WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" ✅ Connected!");

  // Init NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("NTP initialized, waiting for sync...");
}

// ================== Loop ==================
void loop() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("❌ Failed to obtain time");
    delay(1000);
    return;
  }

  int seconds = timeinfo.tm_sec;

  if (seconds == 0) {
    // Read sensors
    TempAndHumidity data1 = dht1.getTempAndHumidity();
    TempAndHumidity data2 = dht2.getTempAndHumidity();
    String timestamp = getTimestamp(timeinfo);

    // Send data
    sendDataToFirebase(data1.temperature, data1.humidity, data2.temperature, data2.humidity, timestamp);

    // Print info
    Serial.println("Sent at " + timestamp);
    Serial.println("Sensor 1: " + String(data1.temperature, 2) + "°C, " + String(data1.humidity, 2) + "%");
    Serial.println("Sensor 2: " + String(data2.temperature, 2) + "°C, " + String(data2.humidity, 2) + "%");

    delay(1100); // Wait 1.1 sec to skip the next same-second trigger
  }

  delay(200); // Regular loop check
}
