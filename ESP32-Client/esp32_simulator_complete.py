#!/usr/bin/env python3
"""
Complete ESP32 PIR Motion Detection Simulator
This simulates the full ESP32 behavior including boot, discovery, and interactive control
"""

import paho.mqtt.client as mqtt
import json
import time
import threading
from datetime import datetime

class ESP32Simulator:
    def __init__(self, broker_host="192.168.29.128", broker_port=1883):
        self.broker_host = broker_host
        self.broker_port = broker_port
        self.device_id = "123456"
        self.sensor_id = 2
        self.port = 1
        self.sense_timeout = 30 * 1000  # 30 seconds
        self.timer_active = False
        self.first_motion_sent = False
        self.timer_start = 0
        self.motion_detected = False
        self.discovery_sent = False
        self.config_received = False
        
        # MQTT client setup
        self.client = mqtt.Client(client_id=self.device_id)
        self.client.username_pw_set("mps-bam100", "bam100")
        self.client.on_connect = self.on_connect
        self.client.on_publish = self.on_publish
        self.client.on_message = self.on_message
        
        # Topics
        self.discovery_topic = "MPS/global/discovery"
        self.config_topic = f"MPS/global/{self.device_id}/config"
        self.control_topic = f"MPS/global/{self.device_id}/control"
        self.reboot_topic = f"MPS/global/{self.device_id}/reboot"
        self.scene_topic = f"MPS/global/{self.device_id}/scene"
        self.status_topic = f"MPS/global/UP/{self.device_id}/status"
        self.ping_topic = "MPS/global/sessionPing"
        
        # LED and Shade states (simulating hardware)
        self.led_states = {}
        self.shade_states = {}
        for i in range(12):
            self.led_states[f"LED{i+1}"] = {"state": "off", "brightness": 0}
        for i in range(4):
            self.shade_states[f"SHADE{i+1}"] = {"state": "closed"}
        
    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            print("✅ Connected to MQTT broker")
            # Subscribe to topics
            self.client.subscribe(self.config_topic)
            self.client.subscribe(self.control_topic)
            self.client.subscribe(self.reboot_topic)
            self.client.subscribe(self.scene_topic)
            print(f"📡 Subscribed to: {self.config_topic}")
            print(f"📡 Subscribed to: {self.control_topic}")
            print(f"📡 Subscribed to: {self.reboot_topic}")
            print(f"📡 Subscribed to: {self.scene_topic}")
        else:
            print(f"❌ Failed to connect, return code {rc}")
    
    def on_publish(self, client, userdata, mid):
        print(f"📤 Message published (mid: {mid})")
    
    def on_message(self, client, userdata, msg):
        topic = msg.topic
        payload = msg.payload.decode()
        print(f"📩 Received on {topic}: {payload}")
        
        try:
            data = json.loads(payload)
            
            if topic.endswith("/config"):
                self.handle_config_message(data)
            elif topic.endswith("/control"):
                self.handle_control_message(data)
            elif topic.endswith("/scene"):
                print("🎨 Received scene command")
                self.process_scene_command(data)
            elif topic.endswith("/reboot"):
                print("🔄 Received reboot command")
                self.handle_reboot_command(data)
                
        except json.JSONDecodeError:
            print("❌ Invalid JSON received")
    
    def handle_config_message(self, data):
        """Handle config request from MQTT"""
        print("⚙️ Config request received")
        if data.get("cmd") == 106 and data.get("cmd_m") == "config":
            self.send_config_response()
            self.config_received = True
            print("✅ Config response sent - device ready for motion detection")
    
    def handle_control_message(self, data):
        """Handle control commands"""
        print(f"🎮 Control command: {data}")
        
        ch_type = data.get("ch_t", "")
        ch_addr = data.get("ch_addr", "")
        cmd = data.get("cmd", 0)
        cmd_m = data.get("cmd_m", "")
        
        print(f"📋 Type: {ch_type}, Address: {ch_addr}, Cmd: {cmd}")
        
        if ch_type == "LED":
            print("💡 Processing LED command...")
            self.process_led_command(data)
        elif ch_type == "SHADE":
            print("🪟 Processing Shade command...")
            self.process_shade_command(data)
    
    def process_led_command(self, command):
        """Process LED control commands"""
        led_addr = command.get("ch_addr", "")
        cmd = command.get("cmd", 0)
        cmd_m = command.get("cmd_m", "")
        
        print(f"🔍 LED Command Details:")
        print(f"   Address: {led_addr}")
        print(f"   Command: {cmd}")
        print(f"   Action: {cmd_m}")
        
        if led_addr == "LED0":
            # Built-in LED
            if cmd == 104:
                state = "on" if cmd_m == "LED_ON" else "off"
                self.led_states["LED0"] = {"state": state, "brightness": 100 if state == "on" else 0}
                self.send_status_update("LED0", state)
                print(f"💡 LED0: {state.upper()}")
            elif cmd == 102:
                brightness = int(cmd_m) if isinstance(cmd_m, (int, str)) and str(cmd_m).isdigit() else 0
                self.led_states["LED0"] = {"state": "on" if brightness > 0 else "off", "brightness": brightness}
                self.send_status_update("LED0", f"{brightness}%")
                print(f"💡 LED0: Brightness {brightness}%")
        else:
            # Regular LEDs (LED1-LED12)
            if led_addr.startswith("LED"):
                led_num = led_addr[3:]
                if led_num.isdigit():
                    led_index = int(led_num)
                    if 1 <= led_index <= 12:
                        if cmd == 104:
                            state = "on" if cmd_m == "LED_ON" else "off"
                            self.led_states[led_addr] = {"state": state, "brightness": 100 if state == "on" else 0}
                            self.send_status_update(led_addr, state)
                            print(f"💡 {led_addr}: {state.upper()}")
                        elif cmd == 102:
                            brightness = int(cmd_m) if isinstance(cmd_m, (int, str)) and str(cmd_m).isdigit() else 0
                            self.led_states[led_addr] = {"state": "on" if brightness > 0 else "off", "brightness": brightness}
                            self.send_status_update(led_addr, f"{brightness}%")
                            print(f"💡 {led_addr}: Brightness {brightness}%")
                    else:
                        print(f"❌ Invalid LED index: {led_index}")
    
    def process_shade_command(self, command):
        """Process Shade control commands"""
        shade_addr = command.get("ch_addr", "")
        cmd = command.get("cmd", 0)
        
        print(f"🔍 Shade Command Details:")
        print(f"   Address: {shade_addr}")
        print(f"   Command: {cmd}")
        
        if shade_addr.startswith("SHADE"):
            shade_num = shade_addr[5:]
            if shade_num.isdigit():
                shade_index = int(shade_num)
                if 1 <= shade_index <= 4:
                    if cmd == 113:  # Open
                        self.shade_states[shade_addr] = {"state": "open"}
                        self.send_status_update(shade_addr, "open")
                        print(f"🪟 {shade_addr}: OPENED")
                    elif cmd == 114:  # Close
                        self.shade_states[shade_addr] = {"state": "closed"}
                        self.send_status_update(shade_addr, "closed")
                        print(f"🪟 {shade_addr}: CLOSED")
                    elif cmd == 111:  # Stop
                        self.shade_states[shade_addr] = {"state": "stopped"}
                        self.send_status_update(shade_addr, "stopped")
                        print(f"🪟 {shade_addr}: STOPPED")
    
    def process_scene_command(self, command):
        """Process scene commands"""
        print("🎨 Processing Scene Command...")
        cmd_m = command.get("cmd_m")
        channels = command.get("ch_addr", [])
        
        print(f"📋 Scene Command Details:")
        print(f"   Channels: {len(channels)}")
        
        if isinstance(cmd_m, str):
            action = cmd_m
            print(f"   Action: {action}")
            if action in ["LED_ON", "LED_OFF"]:
                for ch in channels:
                    if isinstance(ch, int) and 1 <= ch <= 12:
                        led_addr = f"LED{ch}"
                        state = "on" if action == "LED_ON" else "off"
                        self.led_states[led_addr] = {"state": state, "brightness": 100 if state == "on" else 0}
                        self.send_status_update(led_addr, state)
                        print(f"💡 {led_addr}: {state.upper()}")
        elif isinstance(cmd_m, dict):
            if "LED_BRIGHTNESS" in cmd_m:
                brightness = cmd_m["LED_BRIGHTNESS"]
                print(f"   Brightness: {brightness}%")
                for ch in channels:
                    if isinstance(ch, int) and 1 <= ch <= 12:
                        led_addr = f"LED{ch}"
                        self.led_states[led_addr] = {"state": "on", "brightness": brightness}
                        self.send_status_update(led_addr, f"{brightness}%")
                        print(f"💡 {led_addr}: Brightness {brightness}%")
    
    def send_status_update(self, channel, status):
        """Send status update for LED/Shade"""
        payload = {
            "device_id": self.device_id,
            "ch_t": "LED" if channel.startswith("LED") else "SHADE",
            "ch_addr": channel,
            "status": status
        }
        
        self.client.publish(self.status_topic, json.dumps(payload))
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        print(f"{timestamp} -> 📤 Sent Status Update: {json.dumps(payload)}")
    
    def handle_reboot_command(self, data):
        """Handle reboot command"""
        if data.get("deviceId") == "reboot":
            print("🔄 Rebooting ESP32...")
            print("⚠️ Simulator will continue running (real ESP32 would restart)")
    
    def send_ping(self):
        """Send ping message (like ESP32)"""
        ping_data = {
            "device_id": self.device_id,
            "status": "online",
            "uptime": int(time.time()),
            "rssi": -50,  # Simulated RSSI
            "pir_motion": self.motion_detected
        }
        
        self.client.publish(self.ping_topic, json.dumps(ping_data))
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        print(f"{timestamp} -> 📡 Sent Ping: {json.dumps(ping_data)}")
    
    def send_device_discovery(self):
        """Send device discovery message (like ESP32 boot)"""
        discovery_data = {
            "device_id": self.device_id,
            "SNO": "234AM87697",
            "Firmware": "v1.0.0.1",
            "MacAddr": "AA:BB:CC:DD:EE:FF"
        }
        
        result = self.client.publish(self.discovery_topic, json.dumps(discovery_data), retain=True)
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        print(f"{timestamp} -> 📢 Published Discovery Data:")
        print(f"   {json.dumps(discovery_data)}")
        print(f"📤 Message published (mid: {result.mid})")
        
        # Wait for message to be sent
        time.sleep(0.5)
        print("✅ Discovery message sent to broker")
        self.discovery_sent = True
    
    def send_config_response(self):
        """Send config response"""
        config_data = {
            "ch_t": "LED",
            "ch_addr": "LED1",
            "cmd": 100,
            "cmd_m": "config"
        }
        
        self.client.publish(self.config_topic, json.dumps(config_data))
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        print(f"{timestamp} -> 📤 Sent Config Response:")
        print(f"   {json.dumps(config_data)}")
    
    def send_pir_status(self, status):
        """Send PIR status to MQTT broker"""
        payload = {
            "ch_t": "PIR",
            "ch_addr": f"Port-{self.port}_{self.sensor_id}",
            "cmd": 115,
            "cmd_m": f"PIR State = {'1' if status == 'motion_detected' else '0'}"
        }
        
        self.client.publish(self.status_topic, json.dumps(payload))
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        print(f"{timestamp} -> 📤 Sent PIR Status: {json.dumps(payload)}")
    
    def simulate_motion_detection(self, motion_state):
        """Simulate PIR motion detection (like ESP32 checkPIRMotion)"""
        current_time = time.time() * 1000
        
        if motion_state:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')[:-3]} -> 🔴 PIR MOTION DETECTED!")
            self.motion_detected = True
            
            if not self.first_motion_sent:
                print("📤 FIRST MOTION - Sending MQTT config to turn ON lights")
                self.send_pir_status("motion_detected")
                self.first_motion_sent = True
                self.timer_start = current_time
                self.timer_active = True
                print(f"⏰ Timer started ({self.sense_timeout/1000} seconds) - subsequent motion will show on serial only")
            else:
                print("⏳ Motion during timer period - Serial only (NO MQTT)")
        else:
            print(f"{datetime.now().strftime('%H:%M:%S.%f')[:-3]} -> 🟢 PIR NO MOTION")
            self.motion_detected = False
            
            if not self.timer_active:
                print("📤 Sending no motion MQTT to turn OFF lights")
                self.send_pir_status("no_motion")
                self.first_motion_sent = False
            else:
                print("⏳ No motion detected but timer still active - showing on serial only")
    
    def check_timer_timeout(self):
        """Check if timer has expired"""
        if self.timer_active and (time.time() * 1000 - self.timer_start > self.sense_timeout):
            print("⏰ Timer expired - sending no motion MQTT to turn OFF lights")
            elapsed = (time.time() * 1000 - self.timer_start) / 1000
            print(f"   Timer was active for: {elapsed:.1f} seconds")
            self.send_pir_status("no_motion")
            self.motion_detected = False
            self.first_motion_sent = False
            self.timer_active = False
    
    def connect_to_broker(self):
        """Connect to MQTT broker"""
        try:
            print(f"🔌 Connecting to MQTT broker: {self.broker_host}:{self.broker_port}")
            self.client.connect(self.broker_host, self.broker_port, 60)
            self.client.loop_start()
            time.sleep(3)  # Wait longer for connection to stabilize
            return True
        except Exception as e:
            print(f"❌ Connection failed: {e}")
            return False
    
    def boot_sequence(self):
        """Simulate ESP32 boot sequence"""
        print("🚀 ESP32 Starting...")
        print(f"📋 Device ID: {self.device_id}")
        print(f"📋 Serial Number: 234AM87695")
        print(f"📋 Firmware Version: 2.01")
        print(f"📋 MAC Address: AA:BB:CC:DD:EE:FF")
        
        if not self.connect_to_broker():
            return False
        
        print("📡 Setting up MQTT client...")
        print("✅ MQTT client setup complete")
        
        # Send discovery message
        print("📢 Sending device discovery...")
        self.send_device_discovery()
        time.sleep(1)  # Wait for discovery message to be published
        
        print("⏳ Waiting for config request from MQTT layer...")
        print("💡 Tip: Send config request from your MQTT client to continue")
        
        return True
    
    def timer_background_thread(self):
        """Background thread to check timer timeout and send pings"""
        last_ping_time = 0
        ping_interval = 30  # 30 seconds
        
        while True:
            try:
                current_time = time.time()
                
                # Check timer timeout
                if self.timer_active:
                    current_time_ms = current_time * 1000
                    elapsed = (current_time_ms - self.timer_start) / 1000
                    remaining = (self.sense_timeout / 1000) - elapsed
                    
                    # Show timer status every 5 seconds
                    if int(elapsed) % 5 == 0 and elapsed > 0 and remaining > 0:
                        print(f"⏱️ Timer status: {elapsed:.0f}s elapsed, {remaining:.0f}s remaining")
                    
                    # Check for timeout
                    if remaining <= 0:
                        print("⏰ Timer expired - sending no motion MQTT to turn OFF lights")
                        elapsed_total = (current_time_ms - self.timer_start) / 1000
                        print(f"   Timer was active for: {elapsed_total:.1f} seconds")
                        self.send_pir_status("no_motion")
                        self.motion_detected = False
                        self.first_motion_sent = False
                        self.timer_active = False
                        print("🔄 Timer reset - ready for next motion cycle")
                
                # Send ping every 30 seconds
                if current_time - last_ping_time >= ping_interval:
                    self.send_ping()
                    last_ping_time = current_time
                
                time.sleep(1)  # Check every second
            except:
                break
    
    def interactive_mode(self):
        """Interactive mode for manual motion control"""
        print("\n🎮 Interactive Mode Started!")
        print("Commands:")
        print("  'motion' or 'm' - Simulate motion detected")
        print("  'nomotion' or 'n' - Simulate no motion")
        print("  'status' or 's' - Show current status")
        print("  'timer X' - Set timer to X seconds")
        print("  'quit' or 'q' - Exit")
        print("=" * 50)
        
        # Start background timer thread
        timer_thread = threading.Thread(target=self.timer_background_thread, daemon=True)
        timer_thread.start()
        
        while True:
            try:
                command = input("\nESP32> ").strip().lower()
                
                if command in ['motion', 'm']:
                    self.simulate_motion_detection(True)
                elif command in ['nomotion', 'n']:
                    self.simulate_motion_detection(False)
                elif command in ['status', 's']:
                    self.show_status()
                elif command.startswith('timer '):
                    try:
                        new_timer = int(command.split()[1])
                        if 5 <= new_timer <= 3600:
                            self.sense_timeout = new_timer * 1000
                            print(f"✅ Timer set to {new_timer} seconds")
                        else:
                            print("❌ Timer must be between 5-3600 seconds")
                    except:
                        print("❌ Invalid timer value")
                elif command in ['quit', 'q']:
                    break
                else:
                    print("❌ Unknown command")
                
            except KeyboardInterrupt:
                break
            except EOFError:
                break
    
    def show_status(self):
        """Show current device status"""
        print("\n📋 === CURRENT STATUS ===")
        print(f"🔧 Device Information:")
        print(f"   Device ID: {self.device_id}")
        print(f"   Sensor ID: {self.sensor_id}")
        print(f"   Port: {self.port}")
        print(f"   Port Address: Port-{self.port}_{self.sensor_id}")
        print(f"   Motion Status: {'DETECTED' if self.motion_detected else 'NO MOTION'}")
        print(f"   Timer Active: {'YES' if self.timer_active else 'NO'}")
        print(f"   First Motion Sent: {'YES' if self.first_motion_sent else 'NO'}")
        print(f"   Timeout: {self.sense_timeout / 1000} seconds")
        print(f"   Discovery Sent: {'YES' if self.discovery_sent else 'NO'}")
        print(f"   Config Received: {'YES' if self.config_received else 'NO'}")
        
        if self.timer_active:
            elapsed = (time.time() * 1000 - self.timer_start) / 1000
            remaining = (self.sense_timeout / 1000) - elapsed
            print(f"   Timer Elapsed: {elapsed:.1f} seconds")
            print(f"   Timer Remaining: {remaining:.1f} seconds")
        
        print(f"\n💡 LED States:")
        for led, state in self.led_states.items():
            if state["state"] != "off":
                print(f"   {led}: {state['state'].upper()} ({state['brightness']}%)")
        
        print(f"\n🪟 Shade States:")
        for shade, state in self.shade_states.items():
            if state["state"] != "closed":
                print(f"   {shade}: {state['state'].upper()}")
        
        print("=" * 30)
    
    def run(self):
        """Main run function"""
        print("🧪 ESP32 PIR Motion Detection Simulator")
        print("=" * 50)
        
        # Boot sequence
        if not self.boot_sequence():
            print("❌ Boot sequence failed")
            return
        
        # Wait for config or start interactive mode
        print("\n⏳ Waiting for config request...")
        print("💡 You can also start interactive mode by pressing Enter")
        
        # Start interactive mode
        self.interactive_mode()
        
        # Cleanup
        self.client.disconnect()
        print("🔌 Disconnected from MQTT broker")
        print("👋 Goodbye!")

# Main execution
if __name__ == "__main__":
    simulator = ESP32Simulator()
    simulator.run()
