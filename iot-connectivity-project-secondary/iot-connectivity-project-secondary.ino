#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "SSD1306.h"        
#include <LoRa.h>

// WiFi credentials (a mobile hotspot works well for testing)
const char* ssid = "Redmi Note 7";
const char* password = "Zt14C3gh";

// MQTT broker settings
const char* mqtt_server = "172.20.10.2";
const int mqtt_port = 1883;
const char* mqtt_topic = "esp32/sensors/dht11";

// LoRa pins (adjust according to your board)
#define LORA_SCK  5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_SS   18
#define LORA_RST  14
#define LORA_DIO0 26

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

void setupLoRa() {
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // Configure LoRa parameters (must match sender)
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.enableCrc();
  
  log("LoRa initialized");
}

void setup() {
  Serial.begin(115200);
  setupDisplay();
  setupLoRa();
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  
  log("Gateway Ready\nListening for LoRa...");
}

void handleLoRaMessage() {
  int packetSize = LoRa.parsePacket();
  if (packetSize == 0) return; // No packet received
  
  String receivedData = "";
  while (LoRa.available()) {
    receivedData += (char)LoRa.read();
  }
  
  int rssi = LoRa.packetRssi();
  Serial.println("Received: " + receivedData);
  Serial.println("RSSI: " + String(rssi));
  
  // Parse JSON and add RSSI
  StaticJsonDocument<300> doc;
  DeserializationError error = deserializeJson(doc, receivedData);
  
  if (error) {
    log("JSON parse error");
    return;
  }
  
  // Convert back to JSON string for MQTT
  String mqttPayload;
  serializeJson(doc, mqttPayload);
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Publish to MQTT
  if (client.publish(mqtt_topic, mqttPayload.c_str())) {
    // Display received data
    String temp = doc["temperature"];
    String humidity = doc["humidity"];
    String nodeId = doc["node_id"];
    
    String msg = "From: " + nodeId + "\n" +
                 "Temp: " + temp + " ºC\n" +
                 "Humidity: " + humidity + " %\n" +
                 "RSSI: " + String(rssi) + " dBm\n" +
                 "Sent to MQTT";
    log(msg);
  } else {
    log("MQTT publish failed");
  }
}

void loop() {
  handleLoRaMessage();
  
  // Keep MQTT connection alive
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  delay(100);
}