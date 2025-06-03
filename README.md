# IoT Connectivity: Project

*Authors: Pau de las Heras and Eric Roy*

This repository contains the code and reports for the two activities of the
"IoT Connectivity" project.

---

## Overview

The code in this repository is composed of two parts:
- A firmware for a *Heltec Wifi Lora 32v2* that collects temperature and
  humidity readings from a DHT11 and publishes them with MQTT to a broker.
  It connects to a preset WiFi network and uses a pre-defined broker IP and port.
- The software to set up an MQTT broker and a web server to look at the live
  data, passed from the server to the page using socketio.

[TODO GIF OF THE WORKING WEBPAGE]

## Assignment questions

This section contains the answers to the questions that the assignment
description proposes.

### Activity 1

**Q1: Why is MQTT suitable for IoT applications?**

In comparison to HTTP and other applications, MQTT is lightweight (its header
is much, much shorter than HTTP). This makes battery usage less of a problem,
and also makes transmission times smaller, ensuring the bandwidth isn't as
saturated.

MQTT works also with publish-subscribe model, which in its turn has also many
advantages (listed in the newt question).

**Q2: How does the publish-subscribe model improve efficiency over a request-response model?**

The great advantage of pub-sub model is that the IoT device only needs to make
one request (i.e. send one packet), even if there are multiple processes using
that data (i.e. subscribed to that topic). Moreover, the same can be done the
other way around: a process can listen to a single topic where multiple nodes
publish data, making it only having one thing (topic) to worry about.

**Q3: What are the advantages of running Mosquitto in a Docker container?**

There are a few advantages, but mainly the advantages are the docker-generic ones:
- we don't need to set up any specific software on our system,
- it's easy to export the images from different machines,
- isolating the processes also has some security advantages.

### Activity 2

**Q1: How does LoRa compare to Wi-Fi and Bluetooth in terms of range and power consumption?**

**Q2: What are the trade-offs of using lower data rates in LoRa transmission?**

**Q3: How does increasing transmission power affect battery life and interference?**

## Project Setup

In this section we document how to build the project.

### Flash the code to the ESP32

TODO PAU:
- Windows USB drivers to interface with the Heltec Wifi Lora 32 v2 board can be found [here](https://docs.keyestudio.com/projects/Arduino/en/latest/windowsCP2102.html).
- Display use [example](https://github.com/HelTecAutomation/Heltec_ESP32/blob/master/examples/OLED/UiDemo/UiDemo.ino)

- Put the WiFi configuration data and the laptop IP Address to the ArduinoIDE code.

### Software setup

You just need a recent docker version and:

- Clone the repository
- `docker compose up -d`
- Go to [localhost:5000](http://localhost:5000)
