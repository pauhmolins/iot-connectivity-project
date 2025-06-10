#include <DHT.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "SSD1306.h"        
#include <time.h>
#include <LoRa.h>

// LoRa pins (adjust according to your board)
#define LORA_SCK  5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_SS   18
#define LORA_RST  14
#define LORA_DIO0 26

// DHT sensor settings
#define DHTPIN 12
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Display
#define OLED_I2C_ADDRESS 0x3c
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
SSD1306 display(OLED_I2C_ADDRESS, OLED_SDA, OLED_SCL);

// Node ID for identification
const String NODE_ID = "SENSOR_1";

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

void setupLoRa() {
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // Configure LoRa parameters
  LoRa.setTxPower(20);
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.enableCrc();
  
  log("LoRa initialized");
}

void setup() {
  Serial.begin(115200);
  setupDisplay();
  dht.begin();
  setupLoRa();
  
  log("Sensor Node Ready\nNode ID: " + NODE_ID);
  delay(2000);
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
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    log("Sensor error");
    return;
  }

  // Create JSON payload
  StaticJsonDocument<200> doc;
  doc["node_id"] = NODE_ID;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  char payload[256];
  serializeJson(doc, payload);

  // Send via LoRa
  LoRa.beginPacket();
  LoRa.print(payload);
  LoRa.endPacket();

  String sampleTime = getCurrentTime();

  // Display on OLED
  String msg = "Temp: " + String(temperature, 1) + " ºC\n" +
               "Humidity: " + String(humidity, 1) + " %\n" +
               "Time: " + sampleTime + "\n" +
               "Sent via LoRa";
  log(msg);

  // Sample and send every second
  delay(1000);
}