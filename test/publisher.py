# This file sends test data to test the app when we don't have the ESP available

import paho.mqtt.client as mqtt
import time, random

client = mqtt.Client()
client.connect("localhost", 1883, 60)

for i in range(50):
    message = '{"temperature":' + str(random.randint(250, 350)/10) + ',"humidity":' + str(random.randint(0,100)) + '}'
    client.publish("esp32/sensors/dht11", message)
    print(f"Sent: {message}")
    time.sleep(1)

client.disconnect()
