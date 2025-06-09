#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "SSD1306.h"        
#include <time.h>

// WiFi credentials (a mobile hotspot works well for testing)
const char* ssid = "Redmi Note 7";
const char* password = "Zt14C3gh";

// MQTT broker settings
const char* mqtt_server = "172.20.10.2";
const int mqtt_port = 1883;
const char* mqtt_topic = "esp32/sensors/dht11";

// DHT sensor settings
#define DHTPIN 27
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

// Display
#define OLED_I2C_ADDRESS 0x3c
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
SSD1306 display(OLED_I2C_ADDRESS, OLED_SDA, OLED_SCL);

void setupDisplay() {
    // Reset and initialize the screen
    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW);
    delay(200);
    digitalWrite(OLED_RST, HIGH);
    display.init();
}

// Unified log output to Serial and OLED
void log(const String& msg) {
  Serial.println(msg);

  display.clear();
  display.setFont(ArialMT_Plain_16);

  int line = 0;
  int y = 0;
  int lineHeight = 16;

  int start = 0;
  while (true) {
    int end = msg.indexOf('\n', start);
    String lineStr = (end == -1) ? msg.substring(start) : msg.substring(start, end);
    display.drawString(0, y, lineStr);
    y += lineHeight;
    if (end == -1) break;
    start = end + 1;
  }

  display.display();
}

// Wi-Fi setup
void setup_wifi() {
  WiFi.begin(ssid, password);
  log("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  log("WiFi connected\nIP: " + WiFi.localIP().toString());
}

// MQTT reconnect
void reconnect() {
  while (!client.connected()) {
    log("Connecting to MQTT...");
    if (client.connect("ESP32Client")) {
      log("MQTT connected.");
    } else {
      log("MQTT failed, rc=" + String(client.state()));
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setupDisplay();
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

String getCurrentTime() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d",
           timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  return String(buffer);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    log("Sensor error");
    delay(2000);
    return;
  }

  // Create JSON payload
  StaticJsonDocument<200> doc;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  char payload[256];
  serializeJson(doc, payload);

  client.publish(mqtt_topic, payload);

  String sampleTime = getCurrentTime();

  // Display on OLED
  String msg = "Temp: " + String(temperature, 1) + " ºC\n" +
               "Humidity: " + String(humidity, 1) + " %\n" +
               "Time: " + sampleTime;
  log(msg);

  // Sample and publish every second
  delay(1000);
}
