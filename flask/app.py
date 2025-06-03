from flask import Flask, render_template
from flask_socketio import SocketIO
import paho.mqtt.client as mqtt
import eventlet
import json
import os

eventlet.monkey_patch()

app = Flask(__name__)
socketio = SocketIO(app)

# MQTT CONFIG
MQTT_BROKER = os.getenv("MQTT_HOST", "localhost")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
MQTT_TOPIC = os.getenv("MQTT_TOPIC", "esp32/sensors/dht11")

# MQTT CALLBACKS
def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT broker with code:", rc)
    client.subscribe(MQTT_TOPIC)

def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode())
        socketio.emit("mqtt_data", data)
    except:
        print("Received non-json data:", msg.payload)

# MQTT CLIENT INIT
mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)

# Run MQTT client in background
def mqtt_loop():
    mqtt_client.loop_forever()

@app.route("/")
def index():
    return render_template("index.html")

if __name__ == "__main__":
    socketio.start_background_task(mqtt_loop)
    socketio.run(app, host="0.0.0.0", port=5000)
