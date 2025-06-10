#include <SPI.h>
#include <LoRa.h>

int counter = 0;
int packetsLost = 0;
int totalPacketsSent = 0;
unsigned long lastTransmissionTime = 0;

// Pins LoRa per Heltec WiFi LoRa 32 V2
#define LORA_SCK  5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_SS   18
#define LORA_RST  14
#define LORA_DIO0 26

// Configuration parameters for optimization
struct LoRaConfig {
  long frequency = 868E6;           // 915 MHz
  int spreadingFactor = 7;          // SF7 to SF12 (higher = longer range, slower speed)
  long signalBandwidth = 125E3;     // 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, 500E3
  int codingRate = 5;               // 5 to 8 (4/5 to 4/8)
  int txPower = 14;                 // 2 to 20 dBm
  int preambleLength = 8;           // Default 8
  bool crcEnabled = true;
  bool headerMode = false;          // Explicit header mode
};

LoRaConfig config;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Sender with Optimization Features");
  Serial.println("=====================================");

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
  
  Serial.println("\nStarting transmission...");
  Serial.println("Commands: 'sf[7-12]', 'bw[bandwidth]', 'tp[2-20]', 'stats', 'config'");
}

void loop() {
  // Check for serial commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    processCommand(command);
  }

  // Send packet every 5 seconds
  if (millis() - lastTransmissionTime >= 5000) {
    sendPacket();
    lastTransmissionTime = millis();
  }
}

void configureLoRa() {
  LoRa.setSpreadingFactor(config.spreadingFactor);
  LoRa.setSignalBandwidth(config.signalBandwidth);
  LoRa.setCodingRate4(config.codingRate);
  LoRa.setTxPower(config.txPower);
  LoRa.setPreambleLength(config.preambleLength);
  
  if (config.crcEnabled) {
    LoRa.enableCrc();
  } else {
    LoRa.disableCrc();
  }
}

void sendPacket() {
  unsigned long startTime = millis();
  
  Serial.print("Sending packet: ");
  Serial.print(counter);
  
  // Calculate theoretical data rate
  float dataRate = calculateDataRate();
  
  // send packet with timestamp and sequence number for QoS measurement
  LoRa.beginPacket();
  LoRa.print("PKT:");
  LoRa.print(counter);
  LoRa.print(",TIME:");
  LoRa.print(millis());
  LoRa.print(",RSSI_REQ"); // Request RSSI from receiver
  LoRa.endPacket();
  
  unsigned long transmissionTime = millis() - startTime;
  totalPacketsSent++;
  
  Serial.print(" | TX Time: ");
  Serial.print(transmissionTime);
  Serial.print("ms | Data Rate: ");
  Serial.print(dataRate, 2);
  Serial.println(" bps");
  
  counter++;
}

void processCommand(String command) {
  command.toLowerCase();
  
  if (command.startsWith("sf")) {
    int sf = command.substring(2).toInt();
    if (sf >= 7 && sf <= 12) {
      config.spreadingFactor = sf;
      LoRa.setSpreadingFactor(sf);
      Serial.println("Spreading Factor updated to: " + String(sf));
      printDataRateInfo();
    } else {
      Serial.println("Invalid SF. Use 7-12");
    }
  }
  else if (command.startsWith("bw")) {
    long bw = command.substring(2).toFloat() * 1000; // Convert kHz to Hz
    if (setBandwidth(bw)) {
      config.signalBandwidth = bw;
      Serial.println("Bandwidth updated to: " + String(bw/1000.0, 1) + " kHz");
      printDataRateInfo();
    }
  }
  else if (command.startsWith("tp")) {
    int power = command.substring(2).toInt();
    if (power >= 2 && power <= 20) {
      config.txPower = power;
      LoRa.setTxPower(power);
      Serial.println("TX Power updated to: " + String(power) + " dBm");
      printPowerInfo();
    } else {
      Serial.println("Invalid TX Power. Use 2-20 dBm");
    }
  }
  else if (command == "stats") {
    printQoSStats();
  }
  else if (command == "config") {
    printConfiguration();
  }
  else if (command == "help") {
    printHelp();
  }
  else {
    Serial.println("Unknown command. Type 'help' for commands.");
  }
}

bool setBandwidth(long bandwidth) {
  // Valid LoRa bandwidths
  long validBW[] = {7800, 10400, 15600, 20800, 31250, 41700, 62500, 125000, 250000, 500000};
  int numBW = sizeof(validBW) / sizeof(validBW[0]);
  
  for (int i = 0; i < numBW; i++) {
    if (abs(bandwidth - validBW[i]) < 1000) { // Allow some tolerance
      LoRa.setSignalBandwidth(validBW[i]);
      return true;
    }
  }
  
  Serial.println("Invalid BW. Valid: 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125, 250, 500 kHz");
  return false;
}

float calculateDataRate() {
  // LoRa data rate calculation: DR = SF * (BW / 2^SF) * CR
  float cr = 4.0 / (4.0 + config.codingRate);
  float dataRate = config.spreadingFactor * (config.signalBandwidth / pow(2, config.spreadingFactor)) * cr;
  return dataRate;
}

void printConfiguration() {
  Serial.println("\n=== Current LoRa Configuration ===");
  Serial.println("Frequency: " + String(config.frequency/1E6, 1) + " MHz");
  Serial.println("Spreading Factor: " + String(config.spreadingFactor));
  Serial.println("Bandwidth: " + String(config.signalBandwidth/1000.0, 1) + " kHz");
  Serial.println("Coding Rate: 4/" + String(config.codingRate));
  Serial.println("TX Power: " + String(config.txPower) + " dBm");
  Serial.println("Preamble Length: " + String(config.preambleLength));
  Serial.println("CRC: " + String(config.crcEnabled ? "Enabled" : "Disabled"));
  
  printDataRateInfo();
  printRangeEstimate();
  printPowerInfo();
  Serial.println("================================\n");
}

void printDataRateInfo() {
  float dataRate = calculateDataRate();
  float timeOnAir = calculateTimeOnAir(20); // Assuming 20 byte payload
  
  Serial.println("\n--- Data Rate Information ---");
  Serial.println("Theoretical Data Rate: " + String(dataRate, 2) + " bps");
  Serial.println("Time on Air (20 bytes): " + String(timeOnAir, 2) + " ms");
}

void printRangeEstimate() {
  Serial.println("\n--- Range Estimate ---");
  // Rough range estimates based on SF and environment
  int rangeUrban, rangeRural;
  
  switch(config.spreadingFactor) {
    case 7:  rangeUrban = 2;   rangeRural = 5;   break;
    case 8:  rangeUrban = 3;   rangeRural = 7;   break;
    case 9:  rangeUrban = 4;   rangeRural = 10;  break;
    case 10: rangeUrban = 6;   rangeRural = 15;  break;
    case 11: rangeUrban = 8;   rangeRural = 20;  break;
    case 12: rangeUrban = 10;  rangeRural = 25;  break;
    default: rangeUrban = 5;   rangeRural = 10;  break;
  }
  
  Serial.println("Estimated Range - Urban: ~" + String(rangeUrban) + " km");
  Serial.println("Estimated Range - Rural: ~" + String(rangeRural) + " km");
}

void printPowerInfo() {
  Serial.println("\n--- Power Information ---");
  Serial.println("TX Power: " + String(config.txPower) + " dBm");
  
  // Rough current consumption estimates (depends on specific module)
  int currentMA;
  if (config.txPower <= 10) currentMA = 80;
  else if (config.txPower <= 15) currentMA = 100;
  else currentMA = 120;
  
  Serial.println("Estimated TX Current: ~" + String(currentMA) + " mA");
}

float calculateTimeOnAir(int payloadBytes) {
  // Simplified Time on Air calculation for LoRa
  float tSym = (pow(2, config.spreadingFactor) / config.signalBandwidth) * 1000; // Symbol time in ms
  float tPreamble = (config.preambleLength + 4.25) * tSym;
  
  // Payload calculation (simplified)
  float payloadSymb = 8 + max(ceil((8.0 * payloadBytes - 4 * config.spreadingFactor + 28 + 16) / 
                                  (4 * (config.spreadingFactor - 2))) * (4.0 / (4 + config.codingRate)), 0.0);
  
  float tPayload = payloadSymb * tSym;
  return tPreamble + tPayload;
}

void printQoSStats() {
  Serial.println("\n=== Quality of Service Statistics ===");
  Serial.println("Total Packets Sent: " + String(totalPacketsSent));
  Serial.println("Packets Lost: " + String(packetsLost));
  
  if (totalPacketsSent > 0) {
    float packetLossRate = (float)packetsLost / totalPacketsSent * 100;
    float packetSuccessRate = 100 - packetLossRate;
    Serial.println("Packet Success Rate: " + String(packetSuccessRate, 2) + "%");
    Serial.println("Packet Loss Rate: " + String(packetLossRate, 2) + "%");
  }
  
  Serial.println("Current Data Rate: " + String(calculateDataRate(), 2) + " bps");
  Serial.println("====================================\n");
}

void printHelp() {
  Serial.println("\n=== Available Commands ===");
  Serial.println("sf[7-12]     - Set Spreading Factor (e.g., 'sf10')");
  Serial.println("bw[value]    - Set Bandwidth in kHz (e.g., 'bw125')");
  Serial.println("tp[2-20]     - Set TX Power in dBm (e.g., 'tp14')");
  Serial.println("stats        - Show QoS statistics");
  Serial.println("config       - Show current configuration");
  Serial.println("help         - Show this help");
  Serial.println("\nValid Bandwidths: 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125, 250, 500 kHz");
  Serial.println("==========================\n");
}