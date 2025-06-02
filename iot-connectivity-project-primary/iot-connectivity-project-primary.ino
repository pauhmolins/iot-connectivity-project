#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Wire.h>

// WiFi credentials
const char* ssid = "Redmi Note 7";
const char* password = "Zt14C3gh";

// MQTT broker settings
const char* mqtt_server = "192.168.43.78";
const int mqtt_port = 1883;
const char* mqtt_topic = "esp32/sensors/dht11";

// DHT sensor settings
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

// Unified log output to Serial and OLED
void log(const String& msg) {
  Serial.println(msg);
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
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
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

  // Display on OLED
  String msg = "Temp: " + String(temperature, 1) + " C\n" +
               "Humidity: " + String(humidity, 1) + " %\n" +
               "MQTT sent";
  log(msg);

  delay(10000);
}
