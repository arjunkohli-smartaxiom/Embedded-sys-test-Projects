#!/usr/bin/env python3
"""
Test script for ESP32 Timer Configuration via MQTT
This script demonstrates how to configure the timer value remotely via MQTT
"""

import paho.mqtt.client as mqtt
import json
import time
import sys

# MQTT Configuration
MQTT_BROKER = "192.168.29.128"  # Your MQTT broker IP
MQTT_PORT = 1883
DEVICE_ID = "123456"

# Topics
TIMER_TOPIC = f"MPS/global/{DEVICE_ID}/timer"
STATUS_TOPIC = f"MPS/global/UP/{DEVICE_ID}/status"

def on_connect(client, userdata, flags, rc):
    """Callback for when the client connects to the broker"""
    if rc == 0:
        print("✅ Connected to MQTT broker")
        # Subscribe to status topic to see responses
        client.subscribe(STATUS_TOPIC)
        print(f"📡 Subscribed to: {STATUS_TOPIC}")
    else:
        print(f"❌ Failed to connect, return code {rc}")

def on_message(client, userdata, message):
    """Callback for when a message is received"""
    try:
        data = json.loads(message.payload.decode())
        print(f"📩 Received: {data}")
    except json.JSONDecodeError:
        print(f"❌ Invalid JSON received: {message.payload.decode()}")

def set_timer_value(client, timer_seconds):
    """Set timer value via MQTT"""
    if not (5 <= timer_seconds <= 3600):
        print("❌ Timer value must be between 5-3600 seconds")
        return False
    
    timer_config = {
        "cmd": 200,
        "timer_value": timer_seconds
    }
    
    payload = json.dumps(timer_config)
    result = client.publish(TIMER_TOPIC, payload)
    
    if result.rc == mqtt.MQTT_ERR_SUCCESS:
        print(f"📤 Sent timer configuration: {timer_seconds} seconds")
        return True
    else:
        print(f"❌ Failed to publish timer configuration")
        return False

def get_timer_value(client):
    """Get current timer value via MQTT"""
    timer_request = {
        "cmd": 202
    }
    
    payload = json.dumps(timer_request)
    result = client.publish(TIMER_TOPIC, payload)
    
    if result.rc == mqtt.MQTT_ERR_SUCCESS:
        print("📤 Requested current timer value")
        return True
    else:
        print("❌ Failed to request timer value")
        return False

def main():
    """Main function"""
    print("🧪 ESP32 Timer Configuration Test")
    print("=" * 50)
    
    # Create MQTT client
    client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION1)
    client.on_connect = on_connect
    client.on_message = on_message
    
    try:
        # Connect to broker
        print(f"🔌 Connecting to MQTT broker: {MQTT_BROKER}:{MQTT_PORT}")
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.loop_start()
        
        # Wait for connection
        time.sleep(2)
        
        print("\n📋 Available Commands:")
        print("1. Set timer to 30 seconds")
        print("2. Set timer to 60 seconds")
        print("3. Set timer to 120 seconds")
        print("4. Get current timer value")
        print("5. Exit")
        
        while True:
            try:
                choice = input("\n🎮 Enter your choice (1-5): ").strip()
                
                if choice == "1":
                    set_timer_value(client, 30)
                elif choice == "2":
                    set_timer_value(client, 60)
                elif choice == "3":
                    set_timer_value(client, 120)
                elif choice == "4":
                    get_timer_value(client)
                elif choice == "5":
                    break
                else:
                    print("❌ Invalid choice. Please enter 1-5.")
                
                # Wait for response
                time.sleep(1)
                
            except KeyboardInterrupt:
                break
                
    except Exception as e:
        print(f"❌ Error: {e}")
    finally:
        client.loop_stop()
        client.disconnect()
        print("\n👋 Disconnected from MQTT broker")

if __name__ == "__main__":
    main()
