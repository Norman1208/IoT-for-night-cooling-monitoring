#include <WiFi.h>
#include <HTTPClient.h>
#include "DHTesp.h"
#include "time.h"

// WiFi credentials
#define WIFI_SSID "Longi Kost"
#define WIFI_PASSWORD "eeee7777"

// Firebase info
#define DATABASE_URL "https://night-cooling-monitoring-default-rtdb.asia-southeast1.firebasedatabase.app/"

DHTesp dht1;
#define DHT_PIN1 32

// NTP config
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 8 * 3600; // GMT+8
const int daylightOffset_sec = 0;

String getTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "1970-01-01 00:00:00";
  }
  char buffer[25];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}

void setup() {
  Serial.begin(115200);

  // Init DHT
  dht1.setup(DHT_PIN1, DHTesp::DHT22);

  // Connect WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");

  // Init NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {
  TempAndHumidity data1 = dht1.getTempAndHumidity();
  String timestamp = getTimestamp();

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Push to Firebase under /nightCooling.json (no auth)
    String url = String(DATABASE_URL) + "/nightCooling.json";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    // JSON payload
    String payload = "{";
    payload += "\"sensor1\":{\"temperature\":" + String(data1.temperature) +
               ",\"humidity\":" + String(data1.humidity) + "},";
    payload += "\"timestamp\":\"" + timestamp + "\"";
    payload += "}";

    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      Serial.print("Data pushed, response: ");
      Serial.println(httpResponseCode);
      Serial.println(http.getString());
    } else {
      Serial.print("Error in sending POST: ");
      Serial.println(http.errorToString(httpResponseCode));
    }

    http.end();
  }

  Serial.println("Sensor 1 - Temp: " + String(data1.temperature, 2) + "°C Hum: " + String(data1.humidity, 2) + "%");
  delay(10000); // every 10 sec
}
