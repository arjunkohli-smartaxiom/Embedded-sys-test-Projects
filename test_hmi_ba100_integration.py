#!/usr/bin/env python3
"""
Test HMI-BA100 Integration
Tests the communication between HMI simulators and BA100 simulator
"""

import paho.mqtt.client as mqtt
import json
import time
import threading
from datetime import datetime

class IntegrationTester:
    def __init__(self, broker_host="192.168.29.128", broker_port=1883, 
                 username="mps-bam100", password="bam100"):
        self.broker_host = broker_host
        self.broker_port = broker_port
        self.username = username
        self.password = password
        self.client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION1)
        
        # Test results
        self.test_results = {
            "hmi_discovery": False,
            "ba100_discovery": False,
            "hmi_led_control": False,
            "hmi_shade_control": False,
            "ba100_pir_motion": False,
            "integration_working": False
        }
        
        # Setup MQTT
        self.setup_mqtt()
        
    def setup_mqtt(self):
        """Setup MQTT client"""
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_disconnect = self.on_disconnect
        
    def on_connect(self, client, userdata, flags, rc):
        """MQTT connection callback"""
        if rc == 0:
            print("✅ Integration Tester connected to MQTT broker")
            
            # Subscribe to all relevant topics
            self.client.subscribe("MPS/global/discovery")
            self.client.subscribe("MPS/global/+/status")
            self.client.subscribe("MPS/global/+/ping")
            self.client.subscribe("MPS/global/+/config")
            self.client.subscribe("MPS/global/+/control")
            
            print("📡 Subscribed to all test topics")
        else:
            print(f"❌ Failed to connect: {rc}")
            
    def on_message(self, client, userdata, message):
        """MQTT message callback"""
        try:
            topic = message.topic
            data = json.loads(message.payload.decode())
            
            # Log all messages for analysis
            timestamp = datetime.now().strftime("%H:%M:%S")
            print(f"[{timestamp}] 📩 {topic}: {data}")
            
            # Analyze messages for test results
            self.analyze_message(topic, data)
            
        except Exception as e:
            print(f"❌ Error processing message: {e}")
            
    def on_disconnect(self, client, userdata, rc):
        """MQTT disconnection callback"""
        print("🔌 Integration Tester disconnected")
        
    def analyze_message(self, topic, data):
        """Analyze messages to determine test results"""
        
        # Check for HMI discovery
        if topic == "MPS/global/discovery" and data.get("device_type") == "Modbus_HMI":
            self.test_results["hmi_discovery"] = True
            print("✅ HMI Discovery detected")
            
        # Check for BA100 discovery
        if topic == "MPS/global/discovery" and "SNO" in data and "MacAddr" in data:
            if "HMI" not in data.get("device_id", ""):
                self.test_results["ba100_discovery"] = True
                print("✅ BA100 Discovery detected")
                
        # Check for HMI LED control
        if "/status" in topic and data.get("ch_t") == "LED":
            self.test_results["hmi_led_control"] = True
            print("✅ HMI LED Control detected")
            
        # Check for HMI Shade control
        if "/status" in topic and data.get("ch_t") == "SHADE":
            self.test_results["hmi_shade_control"] = True
            print("✅ HMI Shade Control detected")
            
        # Check for BA100 PIR motion
        if "/status" in topic and data.get("ch_t") == "PIR":
            self.test_results["ba100_pir_motion"] = True
            print("✅ BA100 PIR Motion detected")
            
        # Check overall integration
        if (self.test_results["hmi_discovery"] and 
            self.test_results["ba100_discovery"] and
            (self.test_results["hmi_led_control"] or self.test_results["hmi_shade_control"]) and
            self.test_results["ba100_pir_motion"]):
            self.test_results["integration_working"] = True
            print("🎉 INTEGRATION WORKING! All components detected")
            
    def connect_to_broker(self):
        """Connect to MQTT broker"""
        try:
            print(f"🔌 Connecting to MQTT broker: {self.broker_host}:{self.broker_port}")
            
            if self.username and self.password:
                self.client.username_pw_set(self.username, self.password)
                
            self.client.connect(self.broker_host, self.broker_port, 60)
            self.client.loop_start()
            
            print("✅ Integration Tester ready")
            
        except Exception as e:
            print(f"❌ Failed to connect: {e}")
            
    def send_test_commands(self):
        """Send test commands to verify integration"""
        print("\n🧪 Sending test commands...")
        
        # Test HMI LED control
        led_command = {
            "ch_t": "LED",
            "ch_addr": "LED1",
            "cmd": 102,
            "cmd_m": "50"
        }
        self.client.publish("MPS/global/HMI_001/control", json.dumps(led_command))
        print("📤 Sent LED control command")
        
        time.sleep(2)
        
        # Test HMI Shade control
        shade_command = {
            "ch_t": "SHADE",
            "ch_addr": "SHADE1",
            "cmd": 104,
            "cmd_m": "75"
        }
        self.client.publish("MPS/global/HMI_001/control", json.dumps(shade_command))
        print("📤 Sent Shade control command")
        
        time.sleep(2)
        
        # Test BA100 PIR motion
        pir_command = {
            "ch_t": "PIR",
            "ch_addr": "Port-1_2",
            "cmd": 115,
            "cmd_m": "PIR State = 1"
        }
        self.client.publish("MPS/global/123456/status", json.dumps(pir_command))
        print("📤 Sent PIR motion command")
        
    def run_integration_test(self, duration=60):
        """Run integration test for specified duration"""
        print(f"🧪 Starting Integration Test (Duration: {duration} seconds)")
        print("=" * 60)
        
        # Connect to broker
        self.connect_to_broker()
        time.sleep(3)
        
        # Send test commands
        self.send_test_commands()
        
        # Monitor for the specified duration
        print(f"⏳ Monitoring integration for {duration} seconds...")
        print("💡 Start your HMI and BA100 simulators now!")
        print("=" * 60)
        
        start_time = time.time()
        while time.time() - start_time < duration:
            time.sleep(1)
            
            # Print progress every 10 seconds
            elapsed = int(time.time() - start_time)
            if elapsed % 10 == 0 and elapsed > 0:
                print(f"⏰ Test running... {elapsed}/{duration} seconds")
                
        # Print final results
        self.print_test_results()
        
    def print_test_results(self):
        """Print final test results"""
        print("\n" + "=" * 60)
        print("🧪 INTEGRATION TEST RESULTS")
        print("=" * 60)
        
        for test_name, result in self.test_results.items():
            status = "✅ PASS" if result else "❌ FAIL"
            print(f"{test_name.replace('_', ' ').title()}: {status}")
            
        print("=" * 60)
        
        if self.test_results["integration_working"]:
            print("🎉 INTEGRATION TEST PASSED!")
            print("   All components are working together correctly.")
        else:
            print("⚠️ INTEGRATION TEST INCOMPLETE")
            print("   Some components may not be running or communicating.")
            
        print("=" * 60)
        
    def cleanup(self):
        """Cleanup resources"""
        if self.client:
            self.client.disconnect()
        print("🧹 Integration Tester cleanup complete")

def main():
    """Main function"""
    print("🧪 HMI-BA100 Integration Tester")
    print("=" * 50)
    
    tester = IntegrationTester()
    
    try:
        # Run integration test
        duration = int(input("Enter test duration in seconds (default 60): ") or "60")
        tester.run_integration_test(duration)
        
    except KeyboardInterrupt:
        print("\n🛑 Test interrupted by user")
    except Exception as e:
        print(f"❌ Test error: {e}")
    finally:
        tester.cleanup()

if __name__ == "__main__":
    main()
