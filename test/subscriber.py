# This script is a sample subscriber made with chatgpt to test the ESP32.
# However, since now we have the cool web app we don't use it anymore.

import paho.mqtt.client as mqtt
import argparse
import sys

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"[+] Connected to {userdata['broker']}:{userdata['port']} (code {rc})")
        client.subscribe(userdata['topic'])
        print(f"[+] Subscribed to topic: {userdata['topic']}")
    else:
        print(f"[-] Failed to connect, return code {rc}")
        sys.exit(1)

def on_message(client, userdata, msg):
    print(f"[{msg.topic}] {msg.payload.decode('utf-8', errors='replace')}")

def main():
    parser = argparse.ArgumentParser(description="MQTT Topic Subscriber")
    parser.add_argument("--broker", required=True, help="MQTT broker address (e.g. 'localhost')")
    parser.add_argument("--port", type=int, default=1883, help="MQTT broker port (default: 1883)")
    parser.add_argument("--topic", required=True, help="Topic to subscribe to")
    parser.add_argument("--client-id", default="mqtt-subscriber", help="Client ID (default: mqtt-subscriber)")

    args = parser.parse_args()

    userdata = {
        'broker': args.broker,
        'port': args.port,
        'topic': args.topic
    }

    client = mqtt.Client(client_id=args.client_id, userdata=userdata)
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(args.broker, args.port, 60)
        client.loop_forever()
    except KeyboardInterrupt:
        print("\n[!] Interrupted by user")
    except Exception as e:
        print(f"[-] Connection error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
