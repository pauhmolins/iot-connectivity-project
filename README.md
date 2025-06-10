# IoT Connectivity: Project

*Authors: Pau de las Heras and Eric Roy*

This repository contains the code and reports for the first activitiy of the
"IoT Connectivity" course.

Section [Overview](#overview) describes the project and its components.
Section [Project Setup](#project-setup) describes how to set up the project by yourself.
Section [Guiding questions](#guiding-questions) contains the answers to the questions that the assignment description proposes.

---

## Overview

The code in this repository is composed of two parts:
- The firmware for the *Heltec Wifi Lora 32v2* board that collects temperature and
  humidity readings from a DHT11 sensor and publishes them using the MQTT protocol to a broker.
  It connects to a preset WiFi network and uses a pre-defined broker IP and port.
  The corresponding Arduino IDE sketch can be found at [`iot-connectivity-project-primary.ino`](iot-connectivity-project-primary/iot-connectivity-project-primary.ino).
- The software to set up an MQTT broker and a web server (located in the [`flask`](flask/) directory) that ingests live data,
  passed from the server to the page using socketio. Directory [`test/`](test/) contains handy Python scripts to test the MQTT broker,
  which can be used to run the dashboard without an actual weather station.

The simple dashboard visualizes the data in real-time, showing the latest temperature and humidity readings from the weather station:

![webdemo](/docs/webdemo.gif)

The weather station shows some basic status information using a small OLED built-in display:

![displaydemo](/docs/screen.jpg)

## Project Setup

In this section we document how to set up the project by yourself.

### Monitoring server

In our toy weather station scenario, a laptop can act as the monitoring server that ingests and visualizes incoming data from the weather stations. Essentially, it runs a MQTT broker and a web server that displays the data in real-time.

We use the [Eclipse Mosquitto](https://mosquitto.org/) MQTT broker and a custom web server built with Flask and SocketIO to visualize the data, all neatly packaged into a Docker container.

You just need a recent Docker version and:

1. Clone this repository locally
2. Run from the root directory: `docker compose up -d`
3. Go to [localhost:5000](http://localhost:5000)
4. You should see a web page that displays new data incoming from the weather station

### Weather station

Our setup for the weather station (built around an ESP32-based board with sensors) consists of the following components:
- Hardware
  - A Heltec Wifi Lora 32 v2 board
    - In Windows, you may have to install USB drivers for the board available [here](https://docs.keyestudio.com/projects/Arduino/en/latest/windowsCP2102.html).
  - A DHT11 temperature and humidity sensor
- Software
  - Arduino IDE version 2.3.6
    - Board ``Heltec WiFi LoRa 32(V2)`` included in version ``3.1.3`` of the `esp32` package by Espressif Systems
  - Additional libraries:
    - [`PubSubClient`](http://pubsubclient.knolleary.net/) (version ``2.8``) by Nick O'Leary for MQTT communication
    - [`DHT sensor library`](https://github.com/adafruit/DHT-sensor-library) (version ``1.4.6``) by Adafruit for interfacing with the DHT11 sensor
    - [`ArduinoJson`](https://arduinojson.org/?utm_source=meta&utm_medium=library.properties) (version ``7.4.1``) by Benoit Blanchon for JSON serialization
    - [`ESP8266 and ESP32 OLED driver for SSD1306 displays`](https://github.com/ThingPulse/esp8266-oled-ssd1306) (version ``4.6.1``) by ThingPulse for controlling the OLED display

The setup steps are as follows:
1. Install the Arduino IDE.
2. Connect the DHT11 sensor to the Heltec Wifi Lora 32 v2 board:
   - ``VCC`` pin to the board's ``3.3V`` pin
   - ``GND`` pin to the board's ``GND`` pin
   - ``DATA`` pin to ``GPIO27`` on the board (or any other GPIO, but make sure to adjust the code accordingly)
3. Connect the Heltec Wifi Lora 32 v2 board to your computer via USB.
4. Open the Arduino IDE and select the correct board and port.
5. Open the sketch at [`iot-connectivity-project-primary.ino`](iot-connectivity-project-primary/iot-connectivity-project-primary.ino).
6. Adjust the WiFi credentials and MQTT broker settings in the code as described below.
7. Ensure the WiFi access point is available and the MQTT broker is running (see the previous section for details on setting up the broker and web dashboard).
8. Install the necessary libraries (if not already installed).
9. Upload the code to the ESP32 board.

> IMPORTANT: There's a [reported issue](https://github.com/arduino/arduino-ide/issues/2685) with the currently latest ``esp32`` board by Espressif Systems (version 3.2.0) that causes a compilation error.
> To fix it, you need to make sure to downgrade the board to version 3.1.3 in the Arduino IDE.

#### WiFi and MQTT configuration

The WiFi credentials and MQTT broker IP and port are defined in code, adjust appropriately in file [`iot-connectivity-project-primary.ino`](iot-connectivity-project-primary/iot-connectivity-project-primary.ino):

```cpp
...
// WiFi credentials
const char* ssid = "XXXXXX"; // Your WiFi SSID
const char* password = "1234567890"; // Your WiFi password

// MQTT broker settings
const char* mqtt_server = "172.20.10.2"; // Your MQTT broker IP address
const int mqtt_port = 1883; // Your MQTT broker port
const char* mqtt_topic = "esp32/sensors/dht11"; // The topic to publish the sensor data to
...
```

## Guiding questions

This section contains the answers to the questions that the assignment
description proposes.

### Activity 1

**Q1: Why is MQTT suitable for IoT applications?**

In comparison to HTTP and other applications, MQTT is lightweight
(its header is much, much shorter than HTTP, for instance, among other factors).
This makes battery usage less of a problem (a common issue in IoT devices),
and also makes transmission times smaller,
ensuring the bandwidth isn't as saturated
(which can be relevant in deployments with many devices, such as smart homes or industrial IoT).

MQTT also maintains persistent sessions and supports Quality of Service (QoS) levels,
which help ensure message delivery even in unreliable network conditions (again, a common scenario in many IoT deployments).

MQTT works also with publish-subscribe model, which in its turn has also many
advantages (listed in the next question).

**Q2: How does the publish-subscribe model improve efficiency over a request-response model?**

The great advantage of pub-sub model is that the IoT device only needs to make
one request (i.e. send one packet), even if there are multiple processes using
that data (i.e. subscribed to that topic). Moreover, the same can be done the
other way around: a process can listen to a single topic where multiple nodes
publish data, making it only having one thing (topic) to worry about.

This model also decouples producers and consumers in time, space, and synchronization:
publishers and subscribers don’t need to be connected at the same time,
know each other's network locations, or wait for responses — increasing scalability and reliability.

**Q3: What are the advantages of running Mosquitto in a Docker container?**

There are a few advantages to using Docker in our case:
- We don't need to set up any specific software on our system besides Docker
- It's easy to export the images between different machines
- Process isolation offers security advantages by limiting the scope of the broker
- It allows us to run multiple instances of the broker, if needed, without conflicts
- Configuration and dependencies are bundled, making deployments more reproducible
- It simplifies updates and rollbacks, as different versions of Mosquitto can be tested and swapped easily using container tags

### Activity 2

For the activity two, we decided to study deeply the first experiment: *"Adjusting the bandwidth and data rate to balance transmission speed and range."*.

We set up an easily tweakable transmitter and receiver that send packets using different:
- Spreading factors: we used values of 7, 10 and 12.
- Bandwidths: we used 20.8kHz, 41.7kHz, 125kHz and 500kHz.

For each combination of the above, we measured the following data from the receiver
and transmitter (depending on the kind of data): transmission time (in seconds),
transmission rate (in bits per second), RSSI (in dBm) and SNR (in dB).

Below are the plots of the measured 'outputs' for all LoRa configuration combinations:

![Transmission time plot](/docs/tx_time_vs_sf.png)

We can see that higher frequencies enable faster transmission times, as well as small
spreading factors, which make the message to spread less accross time, making it last
less.

![Data rate plot](/docs/data_rate_vs_sf.png)

This is the same as the previous plot, but with the data rate metric. This plot is
maybe 'rendundant', since a long-lasting message already implies a slower data rate.
However, we included it here too just for showcasing the results.

![RSSI plot](/docs/rssi_vs_sf.png)

This plot demonstrates that the received signal is higher with lower spreading factors.
This maybe not that intuitive, since a message with high spreading factor should be
received with greater signal, but keep in mind that received signal depends deeply on
the transmission power, as well as the environment itself.

Here it can be that the antenna has a better reception with certain frequencies, as well
as the on the air time can affect the RSSI calculation (longer transmission times
makes it more probable for an interference to happen).

The same applies with this last plot.

![SNR plot](/docs/snr_vs_sf.png)

Finally, we answer the assignment questions:

**Q1: How does LoRa compare to Wi-Fi and Bluetooth in terms of range and power consumption?**

LoRa is by far the protocol with the highest range, lasting kilometers. Wifi, on the other hand,
has a range of a few meters (~20m in urban environments), and Bluetooth has the shortest range,
lasting only a few meters.

This usually relates to power consumption, making protocols that send to higher distances
use more energy, but in this comparison there are other factors that have also a high impact
on power consumption, such as data rates and frequency bands. LoRa is designed to use very
low energy, but sends packets very slowly. The same applies to Bluetooth (which, compared to
WiFi, sends very small packets).

**Q2: What are the trade-offs of using lower data rates in LoRa transmission?**

Using lower data rates makes transmission times much longer, but makes the signal more reliable
to noise, especially in long distances such as the ones LoRa signals usually need to face.

**Q3: How does increasing transmission power affect battery life and interference?**

We did not try increasing transmission power since we went deeply into a previous analysis,
but we suspect that increasing transmission power will make the battery to last less (i.e.
reduce battery life) but reduce interferences, since our singal will stand out more among
the noise.
