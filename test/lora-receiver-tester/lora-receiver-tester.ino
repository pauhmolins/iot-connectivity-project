#include <SPI.h>
#include <LoRa.h>

int packetsReceived = 0;
int packetsExpected = 0;
int lastPacketNumber = -1;
unsigned long lastPacketTime = 0;
float avgRSSI = 0;
float avgSNR = 0;

// Pins LoRa per Heltec WiFi LoRa 32 V2
#define LORA_SCK  5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_SS   18
#define LORA_RST  14
#define LORA_DIO0 26

// Configuration parameters - should match transmitter initially
struct LoRaConfig {
  long frequency = 868E6;           
  int spreadingFactor = 7;          
  long signalBandwidth = 125E3;     
  int codingRate = 5;               
  int txPower = 14;                 
  int preambleLength = 8;           
  bool crcEnabled = true;
  bool headerMode = false;          
  bool autoConfig = true;           // Auto-match transmitter config
};

LoRaConfig config;

// Auto-configuration test parameters
int testConfigs[][3] = {
  // {SF, BW_index, TX_Power}
  {7, 7, 14},   // SF7, 125kHz, 14dBm
  {8, 7, 14},   // SF8, 125kHz, 14dBm  
  {9, 7, 14},   // SF9, 125kHz, 14dBm
  {10, 7, 14},  // SF10, 125kHz, 14dBm
  {7, 6, 14},   // SF7, 62.5kHz, 14dBm
  {7, 8, 14},   // SF7, 250kHz, 14dBm
};

long bandwidthOptions[] = {7800, 10400, 15600, 20800, 31250, 41700, 62500, 125000, 250000, 500000};
int currentConfigIndex = 0;
int numTestConfigs = sizeof(testConfigs) / sizeof(testConfigs[0]);
unsigned long lastConfigChange = 0;
unsigned long configTestDuration = 30000; // Test each config for 30 seconds

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Receiver with Auto-Configuration");
  Serial.println("====================================");

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(config.frequency)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // Configure LoRa parameters
  configureLoRa();
  
  // Print current configuration
  printConfiguration();
  
  Serial.println("\nListening for packets...");
  Serial.println("Commands: 'sf[7-12]', 'bw[bandwidth]', 'auto[on/off]', 'stats', 'config'");
  
  if (config.autoConfig) {
    Serial.println("Auto-configuration enabled - will cycle through common settings");
  }
}

void loop() {
  // Check for serial commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    processCommand(command);
  }

  // Auto-configuration cycling
  if (config.autoConfig && (millis() - lastConfigChange > configTestDuration)) {
    cycleConfiguration();
  }

  // Check for received packets
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    handleReceivedPacket(packetSize);
  }

  // Periodic status update
  static unsigned long lastStatusUpdate = 0;
  if (millis() - lastStatusUpdate > 10000) { // Every 10 seconds
    printQuickStats();
    lastStatusUpdate = millis();
  }
}

void configureLoRa() {
  LoRa.setSpreadingFactor(config.spreadingFactor);
  LoRa.setSignalBandwidth(config.signalBandwidth);
  LoRa.setCodingRate4(config.codingRate);
  LoRa.setPreambleLength(config.preambleLength);
  
  if (config.crcEnabled) {
    LoRa.enableCrc();
  } else {
    LoRa.disableCrc();
  }
}

void handleReceivedPacket(int packetSize) {
  String receivedData = "";
  float rssi = LoRa.packetRssi();
  float snr = LoRa.packetSnr();
  
  // Read packet data
  while (LoRa.available()) {
    receivedData += (char)LoRa.read();
  }
  
  // Parse packet data
  int packetNumber = -1;
  unsigned long transmitTime = 0;
  
  if (receivedData.startsWith("PKT:")) {
    int commaIndex = receivedData.indexOf(',');
    if (commaIndex > 0) {
      packetNumber = receivedData.substring(4, commaIndex).toInt();
      
      int timeIndex = receivedData.indexOf("TIME:");
      if (timeIndex > 0) {
        int nextComma = receivedData.indexOf(',', timeIndex);
        if (nextComma > 0) {
          transmitTime = receivedData.substring(timeIndex + 5, nextComma).toInt();
        }
      }
    }
  }
  
  // Update statistics
  packetsReceived++;
  updateRSSI_SNR(rssi, snr);
  
  // Check for packet loss
  if (lastPacketNumber >= 0 && packetNumber > 0) {
    int expectedNext = lastPacketNumber + 1;
    if (packetNumber > expectedNext) {
      int lost = packetNumber - expectedNext;
      Serial.println("⚠️  Detected " + String(lost) + " lost packet(s)");
    }
  }
  
  lastPacketNumber = packetNumber;
  lastPacketTime = millis();
  
  // Calculate latency if timestamp available
  unsigned long latency = 0;
  if (transmitTime > 0) {
    latency = millis() - transmitTime;
  }
  
  // Print packet info
  Serial.print("📦 PKT #");
  Serial.print(packetNumber);
  Serial.print(" | Size: ");
  Serial.print(packetSize);
  Serial.print(" | RSSI: ");
  Serial.print(rssi);
  Serial.print(" dBm | SNR: ");
  Serial.print(snr);
  Serial.print(" dB");
  
  if (latency > 0) {
    Serial.print(" | Latency: ");
    Serial.print(latency);
    Serial.print(" ms");
  }
  
  Serial.println();
  
  // Send acknowledgment back to transmitter (optional)
  if (receivedData.indexOf("RSSI_REQ") > 0) {
    sendAcknowledgment(packetNumber, rssi, snr);
  }
}

void sendAcknowledgment(int packetNumber, float rssi, float snr) {
  LoRa.beginPacket();
  LoRa.print("ACK:");
  LoRa.print(packetNumber);
  LoRa.print(",RSSI:");
  LoRa.print(rssi);
  LoRa.print(",SNR:");
  LoRa.print(snr);
  LoRa.endPacket();
}

void cycleConfiguration() {
  currentConfigIndex = (currentConfigIndex + 1) % numTestConfigs;
  
  config.spreadingFactor = testConfigs[currentConfigIndex][0];
  config.signalBandwidth = bandwidthOptions[testConfigs[currentConfigIndex][1]];
  config.txPower = testConfigs[currentConfigIndex][2];
  
  configureLoRa();
  lastConfigChange = millis();
  
  Serial.println("\n🔄 Auto-config: Testing SF" + String(config.spreadingFactor) + 
                 ", BW" + String(config.signalBandwidth/1000.0, 1) + "kHz");
  
  // Reset packet counting for this configuration
  packetsReceived = 0;
  avgRSSI = 0;
  avgSNR = 0;
}

void updateRSSI_SNR(float rssi, float snr) {
  if (packetsReceived == 1) {
    avgRSSI = rssi;
    avgSNR = snr;
  } else {
    avgRSSI = (avgRSSI * (packetsReceived - 1) + rssi) / packetsReceived;
    avgSNR = (avgSNR * (packetsReceived - 1) + snr) / packetsReceived;
  }
}

void processCommand(String command) {
  command.toLowerCase();
  
  if (command.startsWith("sf")) {
    int sf = command.substring(2).toInt();
    if (sf >= 7 && sf <= 12) {
      config.spreadingFactor = sf;
      LoRa.setSpreadingFactor(sf);
      Serial.println("Spreading Factor updated to: " + String(sf));
      resetStats();
    } else {
      Serial.println("Invalid SF. Use 7-12");
    }
  }
  else if (command.startsWith("bw")) {
    long bw = command.substring(2).toFloat() * 1000;
    if (setBandwidth(bw)) {
      config.signalBandwidth = bw;
      Serial.println("Bandwidth updated to: " + String(bw/1000.0, 1) + " kHz");
      resetStats();
    }
  }
  else if (command.startsWith("auto")) {
    String mode = command.substring(4);
    if (mode == "on" || mode == "1") {
      config.autoConfig = true;
      Serial.println("Auto-configuration enabled");
      lastConfigChange = millis();
    } else if (mode == "off" || mode == "0") {
      config.autoConfig = false;
      Serial.println("Auto-configuration disabled");
    } else {
      Serial.println("Use 'autoon' or 'autooff'");
    }
  }
  else if (command == "stats") {
    printDetailedStats();
  }
  else if (command == "config") {
    printConfiguration();
  }
  else if (command == "reset") {
    resetStats();
    Serial.println("Statistics reset");
  }
  else if (command == "help") {
    printHelp();
  }
  else {
    Serial.println("Unknown command. Type 'help' for commands.");
  }
}

bool setBandwidth(long bandwidth) {
  int numBW = sizeof(bandwidthOptions) / sizeof(bandwidthOptions[0]);
  
  for (int i = 0; i < numBW; i++) {
    if (abs(bandwidth - bandwidthOptions[i]) < 1000) {
      LoRa.setSignalBandwidth(bandwidthOptions[i]);
      return true;
    }
  }
  
  Serial.println("Invalid BW. Valid: 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125, 250, 500 kHz");
  return false;
}

void resetStats() {
  packetsReceived = 0;
  packetsExpected = 0;
  lastPacketNumber = -1;
  avgRSSI = 0;
  avgSNR = 0;
}

void printConfiguration() {
  Serial.println("\n=== Current LoRa Configuration ===");
  Serial.println("Frequency: " + String(config.frequency/1E6, 1) + " MHz");
  Serial.println("Spreading Factor: " + String(config.spreadingFactor));
  Serial.println("Bandwidth: " + String(config.signalBandwidth/1000.0, 1) + " kHz");
  Serial.println("Coding Rate: 4/" + String(config.codingRate));
  Serial.println("Preamble Length: " + String(config.preambleLength));
  Serial.println("CRC: " + String(config.crcEnabled ? "Enabled" : "Disabled"));
  Serial.println("Auto-Config: " + String(config.autoConfig ? "Enabled" : "Disabled"));
  Serial.println("================================\n");
}

void printQuickStats() {
  if (packetsReceived > 0) {
    Serial.println("📊 Quick Stats - Packets: " + String(packetsReceived) + 
                   " | Avg RSSI: " + String(avgRSSI, 1) + " dBm | Avg SNR: " + String(avgSNR, 1) + " dB");
  }
}

void printDetailedStats() {
  Serial.println("\n=== Reception Statistics ===");
  Serial.println("Packets Received: " + String(packetsReceived));
  
  if (packetsReceived > 0) {
    Serial.println("Average RSSI: " + String(avgRSSI, 2) + " dBm");
    Serial.println("Average SNR: " + String(avgSNR, 2) + " dB");
    
    // Signal quality assessment
    String signalQuality = "Unknown";
    if (avgRSSI > -80) signalQuality = "Excellent";
    else if (avgRSSI > -100) signalQuality = "Good";
    else if (avgRSSI > -120) signalQuality = "Fair";
    else signalQuality = "Poor";
    
    Serial.println("Signal Quality: " + signalQuality);
    
    // Estimate distance based on RSSI (very rough)
    float estimatedDistance = pow(10, (avgRSSI + 30) / -20.0);
    Serial.println("Estimated Distance: ~" + String(estimatedDistance, 1) + " km");
    
    unsigned long timeSinceLastPacket = millis() - lastPacketTime;
    Serial.println("Time since last packet: " + String(timeSinceLastPacket/1000) + " seconds");
  }
  
  Serial.println("Current Config: SF" + String(config.spreadingFactor) + 
                 ", BW" + String(config.signalBandwidth/1000.0, 1) + "kHz");
  Serial.println("Auto-config: " + String(config.autoConfig ? "ON" : "OFF"));
  Serial.println("============================\n");
}

void printHelp() {
  Serial.println("\n=== Available Commands ===");
  Serial.println("sf[7-12]     - Set Spreading Factor (e.g., 'sf10')");
  Serial.println("bw[value]    - Set Bandwidth in kHz (e.g., 'bw125')");
  Serial.println("autoon       - Enable auto-configuration cycling");
  Serial.println("autooff      - Disable auto-configuration");
  Serial.println("stats        - Show detailed reception statistics");
  Serial.println("config       - Show current configuration");
  Serial.println("reset        - Reset statistics");
  Serial.println("help         - Show this help");
  Serial.println("\nAuto-config cycles through common TX settings automatically");
  Serial.println("==========================\n");
}