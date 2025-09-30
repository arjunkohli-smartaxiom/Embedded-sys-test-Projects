#!/usr/bin/env python3
"""
Simple example to change ESP32 timer configuration via MQTT
"""

import paho.mqtt.client as mqtt
import json
import time

# Configuration
MQTT_BROKER = "192.168.29.128"  # Your MQTT broker IP
MQTT_PORT = 1883
DEVICE_ID = "123456"

def change_timer_value(timer_seconds):
    """Change timer value via MQTT"""
    
    # Create MQTT client
    client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION1)
    
    try:
        # Connect to broker
        print(f"🔌 Connecting to MQTT broker: {MQTT_BROKER}:{MQTT_PORT}")
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        
        # Create timer configuration message
        timer_config = {
            "cmd": 200,
            "timer_value": timer_seconds
        }
        
        # Publish to timer topic
        topic = f"MPS/global/{DEVICE_ID}/timer"
        payload = json.dumps(timer_config)
        
        print(f"📤 Sending timer configuration:")
        print(f"   Topic: {topic}")
        print(f"   Message: {payload}")
        
        result = client.publish(topic, payload)
        
        if result.rc == mqtt.MQTT_ERR_SUCCESS:
            print(f"✅ Timer configuration sent successfully!")
            print(f"   New timer value: {timer_seconds} seconds")
        else:
            print(f"❌ Failed to send timer configuration")
            
    except Exception as e:
        print(f"❌ Error: {e}")
    finally:
        client.disconnect()
        print("👋 Disconnected from MQTT broker")

def get_current_timer():
    """Get current timer value via MQTT"""
    
    # Create MQTT client
    client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION1)
    
    def on_message(client, userdata, message):
        try:
            data = json.loads(message.payload.decode())
            if data.get("ch_t") == "TIMER":
                print(f"📩 Timer Response: {data}")
        except json.JSONDecodeError:
            print(f"❌ Invalid JSON received")
    
    try:
        # Connect to broker
        print(f"🔌 Connecting to MQTT broker: {MQTT_BROKER}:{MQTT_PORT}")
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.on_message = on_message
        
        # Subscribe to status topic to receive response
        status_topic = f"MPS/global/UP/{DEVICE_ID}/status"
        client.subscribe(status_topic)
        print(f"📡 Subscribed to: {status_topic}")
        
        # Start loop to receive messages
        client.loop_start()
        
        # Create timer request message
        timer_request = {
            "cmd": 202
        }
        
        # Publish to timer topic
        topic = f"MPS/global/{DEVICE_ID}/timer"
        payload = json.dumps(timer_request)
        
        print(f"📤 Requesting current timer value:")
        print(f"   Topic: {topic}")
        print(f"   Message: {payload}")
        
        result = client.publish(topic, payload)
        
        if result.rc == mqtt.MQTT_ERR_SUCCESS:
            print(f"✅ Timer request sent successfully!")
            print(f"⏳ Waiting for response...")
            
            # Wait for response
            time.sleep(3)
        else:
            print(f"❌ Failed to send timer request")
            
    except Exception as e:
        print(f"❌ Error: {e}")
    finally:
        client.loop_stop()
        client.disconnect()
        print("👋 Disconnected from MQTT broker")

if __name__ == "__main__":
    print("🧪 ESP32 Timer Configuration Example")
    print("=" * 50)
    
    print("\n1. Setting timer to 90 seconds...")
    change_timer_value(90)
    
    print("\n2. Getting current timer value...")
    get_current_timer()
    
    print("\n3. Setting timer to 30 seconds...")
    change_timer_value(30)
