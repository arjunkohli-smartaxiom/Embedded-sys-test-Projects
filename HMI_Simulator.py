#!/usr/bin/env python3
"""
HMI Simulator for BA100 System
Simulates Modbus-based HMI devices that can control LED channels and shades
Supports multiple HMI devices for realistic testing scenarios
"""

import paho.mqtt.client as mqtt
import json
import time
import threading
import random
import uuid
from datetime import datetime

class HMISimulator:
    def __init__(self, hmi_id=None, broker_host="192.168.29.128", broker_port=1883, 
                 username="mps-bam100", password="bam100"):
        # HMI Configuration
        self.hmi_id = hmi_id or f"HMI_{random.randint(1000, 9999)}"
        self.hmi_type = "Modbus_HMI"
        self.firmware_version = "v2.1.0"
        self.mac_address = f"HM:{random.randint(10,99):02X}:{random.randint(10,99):02X}:{random.randint(10,99):02X}:{random.randint(10,99):02X}:{random.randint(10,99):02X}"
        
        # MQTT Configuration
        self.broker_host = broker_host
        self.broker_port = broker_port
        self.username = username
        self.password = password
        self.client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION1)
        
        # HMI State
        self.connected = False
        self.discovery_sent = False
        self.config_received = False
        self.led_channels = {}  # LED channel states
        self.shade_channels = {}  # Shade channel states
        
        # Configure HMI with initial channels (1 LED + 1 Shade for testing)
        self.configure_hmi_channels()
        
        # MQTT Topics
        self.discovery_topic = "MPS/global/discovery"
        self.config_topic = f"MPS/global/{self.hmi_id}/config"
        self.control_topic = f"MPS/global/{self.hmi_id}/control"
        self.status_topic = f"MPS/global/UP/{self.hmi_id}/status"
        self.ping_topic = f"MPS/global/UP/{self.hmi_id}/ping"
        
        # Setup MQTT callbacks
        self.setup_mqtt_callbacks()
        
        # Threading
        self.ping_thread = None
        self.running = False
        
    def configure_hmi_channels(self):
        """Configure HMI with LED and Shade channels"""
        # LED Channels (1 channel for initial testing)
        self.led_channels = {
            "LED1": {
                "id": 1,
                "name": "Main Light",
                "state": False,
                "brightness": 0,
                "type": "LED",
                "port": 1
            }
        }
        
        # Shade Channels (1 channel for initial testing)
        self.shade_channels = {
            "SHADE1": {
                "id": 1,
                "name": "Main Shade",
                "state": False,
                "position": 0,  # 0-100%
                "type": "SHADE",
                "port": 2
            }
        }
        
        print(f"🔧 HMI {self.hmi_id} configured with:")
        print(f"   📍 LED Channels: {len(self.led_channels)}")
        print(f"   🪟 Shade Channels: {len(self.shade_channels)}")
        
    def setup_mqtt_callbacks(self):
        """Setup MQTT client callbacks"""
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_disconnect = self.on_disconnect
        
    def on_connect(self, client, userdata, flags, rc):
        """MQTT connection callback"""
        if rc == 0:
            self.connected = True
            print(f"✅ HMI {self.hmi_id} connected to MQTT broker")
            
            # Subscribe to topics
            self.client.subscribe(self.config_topic)
            self.client.subscribe(self.control_topic)
            print(f"📡 Subscribed to: {self.config_topic}")
            print(f"📡 Subscribed to: {self.control_topic}")
            
            # Send discovery message
            self.send_discovery()
            
        else:
            print(f"❌ HMI {self.hmi_id} failed to connect: {rc}")
            
    def on_message(self, client, userdata, message):
        """MQTT message callback"""
        try:
            topic = message.topic
            data = json.loads(message.payload.decode())
            
            print(f"📩 HMI {self.hmi_id} received on {topic}: {data}")
            
            if topic == self.config_topic:
                self.handle_config_message(data)
            elif topic == self.control_topic:
                self.handle_control_message(data)
                
        except Exception as e:
            print(f"❌ Error processing message: {e}")
            
    def on_disconnect(self, client, userdata, rc):
        """MQTT disconnection callback"""
        self.connected = False
        print(f"🔌 HMI {self.hmi_id} disconnected from MQTT broker")
        
    def send_discovery(self):
        """Send HMI discovery message with channel configuration"""
        discovery_data = {
            "device_id": self.hmi_id,
            "device_type": self.hmi_type,
            "SNO": f"HMI{random.randint(100000, 999999)}",
            "Firmware": self.firmware_version,
            "MacAddr": self.mac_address,
            "channels": {
                "led_channels": len(self.led_channels),
                "shade_channels": len(self.shade_channels),
                "total_channels": len(self.led_channels) + len(self.shade_channels)
            },
            "led_config": list(self.led_channels.keys()),
            "shade_config": list(self.shade_channels.keys()),
            "modbus_config": {
                "baud_rate": 9600,
                "parity": "none",
                "stop_bits": 1,
                "data_bits": 8
            }
        }
        
        self.client.publish(self.discovery_topic, json.dumps(discovery_data))
        self.discovery_sent = True
        
        print(f"📢 HMI {self.hmi_id} Discovery Sent:")
        print(f"   🔍 Device ID: {self.hmi_id}")
        print(f"   📍 LED Channels: {len(self.led_channels)}")
        print(f"   🪟 Shade Channels: {len(self.shade_channels)}")
        print(f"   📡 Published to: {self.discovery_topic}")
        
    def handle_config_message(self, data):
        """Handle configuration messages"""
        try:
            ch_type = data.get("ch_t", "")
            ch_addr = data.get("ch_addr", "")
            cmd = data.get("cmd", 0)
            
            if cmd == 106:  # Config request
                print(f"⚙️ HMI {self.hmi_id} received config request for {ch_type} {ch_addr}")
                
                # Send config response
                config_response = {
                    "ch_t": ch_type,
                    "ch_addr": ch_addr,
                    "cmd": 100,
                    "cmd_m": "config",
                    "device_id": self.hmi_id,
                    "channel_info": self.get_channel_info(ch_type, ch_addr)
                }
                
                self.client.publish(self.status_topic, json.dumps(config_response))
                self.config_received = True
                print(f"✅ HMI {self.hmi_id} sent config response")
                
        except Exception as e:
            print(f"❌ Error handling config message: {e}")
            
    def handle_control_message(self, data):
        """Handle control messages for LED and Shade channels"""
        try:
            ch_type = data.get("ch_t", "")
            ch_addr = data.get("ch_addr", "")
            cmd = data.get("cmd", 0)
            cmd_m = data.get("cmd_m", "")
            
            print(f"🎮 HMI {self.hmi_id} Control command: {data}")
            
            if ch_type == "LED":
                self.process_led_command(data)
            elif ch_type == "SHADE":
                self.process_shade_command(data)
            else:
                print(f"⚠️ Unknown channel type: {ch_type}")
                
        except Exception as e:
            print(f"❌ Error handling control message: {e}")
            
    def process_led_command(self, data):
        """Process LED control commands"""
        ch_addr = data.get("ch_addr", "")
        cmd = data.get("cmd", 0)
        cmd_m = data.get("cmd_m", "")
        
        if ch_addr in self.led_channels:
            led = self.led_channels[ch_addr]
            
            if cmd == 102:  # LED control
                if isinstance(cmd_m, (int, str)) and str(cmd_m).isdigit():
                    brightness = int(cmd_m)
                    led["brightness"] = brightness
                    led["state"] = brightness > 0
                    
                    print(f"💡 HMI {self.hmi_id} LED {ch_addr}:")
                    print(f"   🔆 Brightness: {brightness}%")
                    print(f"   🔌 State: {'ON' if led['state'] else 'OFF'}")
                    
                    # Send status update
                    self.send_led_status(ch_addr, led)
                    
            elif cmd == 103:  # LED toggle
                led["state"] = not led["state"]
                led["brightness"] = 100 if led["state"] else 0
                
                print(f"🔄 HMI {self.hmi_id} LED {ch_addr} toggled: {'ON' if led['state'] else 'OFF'}")
                self.send_led_status(ch_addr, led)
                
        else:
            print(f"⚠️ LED channel {ch_addr} not found")
            
    def process_shade_command(self, data):
        """Process Shade control commands"""
        ch_addr = data.get("ch_addr", "")
        cmd = data.get("cmd", 0)
        cmd_m = data.get("cmd_m", "")
        
        if ch_addr in self.shade_channels:
            shade = self.shade_channels[ch_addr]
            
            if cmd == 104:  # Shade position control
                if isinstance(cmd_m, (int, str)) and str(cmd_m).isdigit():
                    position = int(cmd_m)
                    shade["position"] = max(0, min(100, position))  # Clamp 0-100
                    shade["state"] = shade["position"] > 0
                    
                    print(f"🪟 HMI {self.hmi_id} Shade {ch_addr}:")
                    print(f"   📏 Position: {shade['position']}%")
                    print(f"   🔌 State: {'OPEN' if shade['state'] else 'CLOSED'}")
                    
                    # Send status update
                    self.send_shade_status(ch_addr, shade)
                    
            elif cmd == 105:  # Shade toggle
                shade["state"] = not shade["state"]
                shade["position"] = 100 if shade["state"] else 0
                
                print(f"🔄 HMI {self.hmi_id} Shade {ch_addr} toggled: {'OPEN' if shade['state'] else 'CLOSED'}")
                self.send_shade_status(ch_addr, shade)
                
        else:
            print(f"⚠️ Shade channel {ch_addr} not found")
            
    def send_led_status(self, ch_addr, led):
        """Send LED status update"""
        status_data = {
            "ch_t": "LED",
            "ch_addr": ch_addr,
            "cmd": 101,
            "cmd_m": f"LED State = {1 if led['state'] else 0}, Brightness = {led['brightness']}",
            "device_id": self.hmi_id,
            "brightness": led["brightness"],
            "state": led["state"]
        }
        
        self.client.publish(self.status_topic, json.dumps(status_data))
        print(f"📤 HMI {self.hmi_id} sent LED status: {ch_addr}")
        
    def send_shade_status(self, ch_addr, shade):
        """Send Shade status update"""
        status_data = {
            "ch_t": "SHADE",
            "ch_addr": ch_addr,
            "cmd": 101,
            "cmd_m": f"Shade State = {1 if shade['state'] else 0}, Position = {shade['position']}%",
            "device_id": self.hmi_id,
            "position": shade["position"],
            "state": shade["state"]
        }
        
        self.client.publish(self.status_topic, json.dumps(status_data))
        print(f"📤 HMI {self.hmi_id} sent Shade status: {ch_addr}")
        
    def get_channel_info(self, ch_type, ch_addr):
        """Get channel information for config response"""
        if ch_type == "LED" and ch_addr in self.led_channels:
            return self.led_channels[ch_addr]
        elif ch_type == "SHADE" and ch_addr in self.shade_channels:
            return self.shade_channels[ch_addr]
        return None
        
    def send_ping(self):
        """Send periodic ping message"""
        ping_data = {
            "device_id": self.hmi_id,
            "device_type": self.hmi_type,
            "status": "online",
            "uptime": int(time.time()),
            "rssi": random.randint(-70, -30),
            "led_channels": len(self.led_channels),
            "shade_channels": len(self.shade_channels),
            "discovery_sent": self.discovery_sent,
            "config_received": self.config_received
        }
        
        self.client.publish(self.ping_topic, json.dumps(ping_data))
        
    def ping_background_thread(self):
        """Background thread for sending ping messages"""
        while self.running:
            if self.connected:
                self.send_ping()
            time.sleep(30)  # Send ping every 30 seconds
            
    def connect_to_broker(self):
        """Connect to MQTT broker"""
        try:
            print(f"🔌 HMI {self.hmi_id} connecting to MQTT broker: {self.broker_host}:{self.broker_port}")
            
            if self.username and self.password:
                self.client.username_pw_set(self.username, self.password)
                
            self.client.connect(self.broker_host, self.broker_port, 60)
            time.sleep(3)  # Wait for connection to stabilize
            
            # Start background threads
            self.running = True
            self.ping_thread = threading.Thread(target=self.ping_background_thread, daemon=True)
            self.ping_thread.start()
            
            # Start MQTT loop
            self.client.loop_start()
            
            print(f"✅ HMI {self.hmi_id} MQTT client setup complete")
            
        except Exception as e:
            print(f"❌ Failed to connect to MQTT broker: {e}")
            
    def show_status(self):
        """Show current HMI status"""
        print(f"\n📊 HMI {self.hmi_id} Status:")
        print(f"   🔌 Connected: {self.connected}")
        print(f"   📢 Discovery Sent: {self.discovery_sent}")
        print(f"   ⚙️ Config Received: {self.config_received}")
        print(f"   💡 LED Channels: {len(self.led_channels)}")
        print(f"   🪟 Shade Channels: {len(self.shade_channels)}")
        
        print(f"\n💡 LED States:")
        for ch_addr, led in self.led_channels.items():
            print(f"   {ch_addr}: {'ON' if led['state'] else 'OFF'} ({led['brightness']}%)")
            
        print(f"\n🪟 Shade States:")
        for ch_addr, shade in self.shade_channels.items():
            print(f"   {ch_addr}: {'OPEN' if shade['state'] else 'CLOSED'} ({shade['position']}%)")
            
    def interactive_mode(self):
        """Interactive mode for testing"""
        print(f"\n🎮 HMI {self.hmi_id} Interactive Mode Started!")
        print("Commands:")
        print("  'led1 on/off' - Toggle LED1")
        print("  'led1 brightness X' - Set LED1 brightness (0-100)")
        print("  'shade1 open/close' - Toggle Shade1")
        print("  'shade1 position X' - Set Shade1 position (0-100)")
        print("  'status' or 's' - Show current status")
        print("  'discovery' or 'd' - Send discovery message")
        print("  'quit' or 'q' - Exit")
        print("=" * 60)
        
        while True:
            try:
                command = input(f"HMI_{self.hmi_id}> ").strip().lower()
                
                if command in ['quit', 'q']:
                    break
                elif command in ['status', 's']:
                    self.show_status()
                elif command in ['discovery', 'd']:
                    self.send_discovery()
                elif command.startswith('led1'):
                    self.handle_led_command(command)
                elif command.startswith('shade1'):
                    self.handle_shade_command(command)
                else:
                    print("❓ Unknown command. Type 'quit' to exit.")
                    
            except KeyboardInterrupt:
                break
            except Exception as e:
                print(f"❌ Error: {e}")
                
    def handle_led_command(self, command):
        """Handle LED commands"""
        parts = command.split()
        if len(parts) < 2:
            print("❓ Usage: led1 on/off/brightness X")
            return
            
        action = parts[1]
        
        if action == 'on':
            self.process_led_command({
                "ch_t": "LED",
                "ch_addr": "LED1",
                "cmd": 102,
                "cmd_m": "100"
            })
        elif action == 'off':
            self.process_led_command({
                "ch_t": "LED",
                "ch_addr": "LED1",
                "cmd": 102,
                "cmd_m": "0"
            })
        elif action == 'brightness' and len(parts) > 2:
            try:
                brightness = int(parts[2])
                self.process_led_command({
                    "ch_t": "LED",
                    "ch_addr": "LED1",
                    "cmd": 102,
                    "cmd_m": str(brightness)
                })
            except ValueError:
                print("❓ Invalid brightness value")
        else:
            print("❓ Usage: led1 on/off/brightness X")
            
    def handle_shade_command(self, command):
        """Handle Shade commands"""
        parts = command.split()
        if len(parts) < 2:
            print("❓ Usage: shade1 open/close/position X")
            return
            
        action = parts[1]
        
        if action == 'open':
            self.process_shade_command({
                "ch_t": "SHADE",
                "ch_addr": "SHADE1",
                "cmd": 104,
                "cmd_m": "100"
            })
        elif action == 'close':
            self.process_shade_command({
                "ch_t": "SHADE",
                "ch_addr": "SHADE1",
                "cmd": 104,
                "cmd_m": "0"
            })
        elif action == 'position' and len(parts) > 2:
            try:
                position = int(parts[2])
                self.process_shade_command({
                    "ch_t": "SHADE",
                    "ch_addr": "SHADE1",
                    "cmd": 104,
                    "cmd_m": str(position)
                })
            except ValueError:
                print("❓ Invalid position value")
        else:
            print("❓ Usage: shade1 open/close/position X")
            
    def cleanup(self):
        """Cleanup resources"""
        self.running = False
        if self.connected:
            self.client.disconnect()
        print(f"🧹 HMI {self.hmi_id} cleanup complete")

def main():
    """Main function"""
    print("🏭 HMI Simulator for BA100 System")
    print("=" * 50)
    
    # Get HMI ID from user or use default
    hmi_id = input("Enter HMI ID (or press Enter for auto-generated): ").strip()
    if not hmi_id:
        hmi_id = None
        
    # Create HMI simulator
    hmi = HMISimulator(hmi_id)
    
    try:
        # Connect to broker
        hmi.connect_to_broker()
        
        # Wait a moment for connection
        time.sleep(2)
        
        # Start interactive mode
        hmi.interactive_mode()
        
    except KeyboardInterrupt:
        print("\n🛑 Shutting down HMI simulator...")
    finally:
        hmi.cleanup()

if __name__ == "__main__":
    main()
