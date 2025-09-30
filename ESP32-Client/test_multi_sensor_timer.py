#!/usr/bin/env python3
"""
Test script for Multi-Sensor Timer Configuration via MQTT
This script demonstrates how to configure timer values for different sensors and ports
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

def set_sensor_timer(client, sensor_id, port, timer_seconds):
    """Set timer value for specific sensor and port"""
    if not (5 <= timer_seconds <= 3600):
        print("❌ Timer value must be between 5-3600 seconds")
        return False
    
    timer_config = {
        "cmd": 200,
        "sensor_id": sensor_id,
        "port": port,
        "timer_value": timer_seconds
    }
    
    payload = json.dumps(timer_config)
    result = client.publish(TIMER_TOPIC, payload)
    
    if result.rc == mqtt.MQTT_ERR_SUCCESS:
        print(f"📤 Sent timer configuration for Sensor ID:{sensor_id} Port:{port} - {timer_seconds} seconds")
        return True
    else:
        print(f"❌ Failed to publish timer configuration")
        return False

def get_sensor_timer(client, sensor_id, port):
    """Get current timer value for specific sensor and port"""
    timer_request = {
        "cmd": 202,
        "sensor_id": sensor_id,
        "port": port
    }
    
    payload = json.dumps(timer_request)
    result = client.publish(TIMER_TOPIC, payload)
    
    if result.rc == mqtt.MQTT_ERR_SUCCESS:
        print(f"📤 Requested timer value for Sensor ID:{sensor_id} Port:{port}")
        return True
    else:
        print("❌ Failed to request timer value")
        return False

def get_all_sensors_timer(client):
    """Get all sensors timer status"""
    timer_request = {
        "cmd": 203
    }
    
    payload = json.dumps(timer_request)
    result = client.publish(TIMER_TOPIC, payload)
    
    if result.rc == mqtt.MQTT_ERR_SUCCESS:
        print("📤 Requested all sensors timer status")
        return True
    else:
        print("❌ Failed to request all sensors timer status")
        return False

def main():
    """Main function"""
    print("🧪 Multi-Sensor Timer Configuration Test")
    print("=" * 60)
    
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
        
        print("\n📋 Multi-Sensor Timer Configuration Examples:")
        print("1. Set Sensor ID 1, Port 1 to 30 seconds")
        print("2. Set Sensor ID 2, Port 1 to 60 seconds")
        print("3. Set Sensor ID 1, Port 2 to 90 seconds")
        print("4. Get timer for Sensor ID 1, Port 1")
        print("5. Get timer for Sensor ID 2, Port 1")
        print("6. Get all sensors timer status")
        print("7. Exit")
        
        while True:
            try:
                choice = input("\n🎮 Enter your choice (1-7): ").strip()
                
                if choice == "1":
                    set_sensor_timer(client, 1, 1, 30)
                elif choice == "2":
                    set_sensor_timer(client, 2, 1, 60)
                elif choice == "3":
                    set_sensor_timer(client, 1, 2, 90)
                elif choice == "4":
                    get_sensor_timer(client, 1, 1)
                elif choice == "5":
                    get_sensor_timer(client, 2, 1)
                elif choice == "6":
                    get_all_sensors_timer(client)
                elif choice == "7":
                    break
                else:
                    print("❌ Invalid choice. Please enter 1-7.")
                
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
