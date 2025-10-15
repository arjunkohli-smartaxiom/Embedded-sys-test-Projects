#!/usr/bin/env python3
"""
Multi-HMI Launcher for BA100 System
Launches multiple HMI simulators for realistic testing scenarios
Supports 5-16 HMI devices as requested
"""

import threading
import time
import sys
from HMI_Simulator import HMISimulator

class MultiHMILauncher:
    def __init__(self, broker_host="192.168.29.128", broker_port=1883, 
                 username="mps-bam100", password="bam100"):
        self.broker_host = broker_host
        self.broker_port = broker_port
        self.username = username
        self.password = password
        self.hmi_simulators = []
        self.running = False
        
    def create_hmi_simulator(self, hmi_id, led_channels=1, shade_channels=1):
        """Create a single HMI simulator with custom configuration"""
        hmi = HMISimulator(
            hmi_id=hmi_id,
            broker_host=self.broker_host,
            broker_port=self.broker_port,
            username=self.username,
            password=self.password
        )
        
        # Configure channels based on requirements
        hmi.configure_hmi_channels()
        
        # Override channel configuration if needed
        if led_channels != 1 or shade_channels != 1:
            hmi.led_channels = {}
            hmi.shade_channels = {}
            
            # Create LED channels
            for i in range(1, led_channels + 1):
                hmi.led_channels[f"LED{i}"] = {
                    "id": i,
                    "name": f"LED Channel {i}",
                    "state": False,
                    "brightness": 0,
                    "type": "LED",
                    "port": i
                }
            
            # Create Shade channels
            for i in range(1, shade_channels + 1):
                hmi.shade_channels[f"SHADE{i}"] = {
                    "id": i,
                    "name": f"Shade Channel {i}",
                    "state": False,
                    "position": 0,
                    "type": "SHADE",
                    "port": i + 10  # Different port range for shades
                }
        
        return hmi
        
    def launch_single_hmi(self, hmi):
        """Launch a single HMI simulator in a separate thread"""
        try:
            print(f"🚀 Starting HMI {hmi.hmi_id}...")
            hmi.connect_to_broker()
            
            # Keep the HMI running
            while self.running:
                time.sleep(1)
                
        except Exception as e:
            print(f"❌ Error in HMI {hmi.hmi_id}: {e}")
        finally:
            hmi.cleanup()
            
    def launch_multiple_hmis(self, num_hmis=5, led_channels=1, shade_channels=1):
        """Launch multiple HMI simulators"""
        print(f"🏭 Launching {num_hmis} HMI simulators...")
        print(f"   💡 LED Channels per HMI: {led_channels}")
        print(f"   🪟 Shade Channels per HMI: {shade_channels}")
        print("=" * 60)
        
        self.running = True
        threads = []
        
        for i in range(1, num_hmis + 1):
            hmi_id = f"HMI_{i:03d}"
            hmi = self.create_hmi_simulator(hmi_id, led_channels, shade_channels)
            self.hmi_simulators.append(hmi)
            
            # Create thread for this HMI
            thread = threading.Thread(target=self.launch_single_hmi, args=(hmi,), daemon=True)
            threads.append(thread)
            thread.start()
            
            # Small delay between launches
            time.sleep(0.5)
            
        print(f"✅ All {num_hmis} HMI simulators launched!")
        print("Press Ctrl+C to stop all simulators")
        
        try:
            # Keep main thread alive
            while self.running:
                time.sleep(1)
        except KeyboardInterrupt:
            print("\n🛑 Stopping all HMI simulators...")
            self.running = False
            
        # Wait for all threads to finish
        for thread in threads:
            thread.join(timeout=5)
            
        print("🧹 All HMI simulators stopped")
        
    def launch_scenario_1(self):
        """Launch Scenario 1: 5 HMIs with 1 LED + 1 Shade each"""
        print("📋 Scenario 1: 5 HMIs (1 LED + 1 Shade each)")
        self.launch_multiple_hmis(num_hmis=5, led_channels=1, shade_channels=1)
        
    def launch_scenario_2(self):
        """Launch Scenario 2: 10 HMIs with 2 LED + 2 Shade each"""
        print("📋 Scenario 2: 10 HMIs (2 LED + 2 Shade each)")
        self.launch_multiple_hmis(num_hmis=10, led_channels=2, shade_channels=2)
        
    def launch_scenario_3(self):
        """Launch Scenario 3: 16 HMIs with mixed configurations"""
        print("📋 Scenario 3: 16 HMIs (Mixed configurations)")
        
        # Launch HMIs with different configurations
        configs = [
            (1, 1),   # HMI_001: 1 LED, 1 Shade
            (2, 1),   # HMI_002: 2 LED, 1 Shade
            (1, 2),   # HMI_003: 1 LED, 2 Shade
            (3, 2),   # HMI_004: 3 LED, 2 Shade
            (2, 3),   # HMI_005: 2 LED, 3 Shade
        ]
        
        # Repeat the pattern for 16 HMIs
        for i in range(16):
            hmi_id = f"HMI_{i+1:03d}"
            led_channels, shade_channels = configs[i % len(configs)]
            
            hmi = self.create_hmi_simulator(hmi_id, led_channels, shade_channels)
            self.hmi_simulators.append(hmi)
            
            thread = threading.Thread(target=self.launch_single_hmi, args=(hmi,), daemon=True)
            thread.start()
            time.sleep(0.3)
            
        print("✅ 16 HMI simulators with mixed configurations launched!")
        
    def interactive_mode(self):
        """Interactive mode for selecting scenarios"""
        print("🏭 Multi-HMI Launcher for BA100 System")
        print("=" * 50)
        print("Select a scenario:")
        print("1. 5 HMIs (1 LED + 1 Shade each) - Basic testing")
        print("2. 10 HMIs (2 LED + 2 Shade each) - Medium scale")
        print("3. 16 HMIs (Mixed configurations) - Full scale")
        print("4. Custom configuration")
        print("5. Single HMI (Interactive mode)")
        print("=" * 50)
        
        while True:
            try:
                choice = input("Enter your choice (1-5): ").strip()
                
                if choice == '1':
                    self.launch_scenario_1()
                    break
                elif choice == '2':
                    self.launch_scenario_2()
                    break
                elif choice == '3':
                    self.launch_scenario_3()
                    break
                elif choice == '4':
                    self.custom_configuration()
                    break
                elif choice == '5':
                    self.single_hmi_mode()
                    break
                else:
                    print("❓ Invalid choice. Please enter 1-5.")
                    
            except KeyboardInterrupt:
                print("\n🛑 Exiting...")
                break
                
    def custom_configuration(self):
        """Custom configuration mode"""
        try:
            num_hmis = int(input("Number of HMIs (1-20): "))
            led_channels = int(input("LED channels per HMI (1-5): "))
            shade_channels = int(input("Shade channels per HMI (1-5): "))
            
            if 1 <= num_hmis <= 20 and 1 <= led_channels <= 5 and 1 <= shade_channels <= 5:
                self.launch_multiple_hmis(num_hmis, led_channels, shade_channels)
            else:
                print("❓ Invalid configuration. Please check your inputs.")
                
        except ValueError:
            print("❓ Invalid input. Please enter numbers only.")
            
    def single_hmi_mode(self):
        """Single HMI interactive mode"""
        hmi_id = input("Enter HMI ID (or press Enter for auto-generated): ").strip()
        if not hmi_id:
            hmi_id = None
            
        hmi = HMISimulator(hmi_id)
        
        try:
            hmi.connect_to_broker()
            time.sleep(2)
            hmi.interactive_mode()
        except KeyboardInterrupt:
            print("\n🛑 Shutting down HMI simulator...")
        finally:
            hmi.cleanup()

def main():
    """Main function"""
    launcher = MultiHMILauncher()
    
    try:
        launcher.interactive_mode()
    except KeyboardInterrupt:
        print("\n🛑 Shutting down Multi-HMI Launcher...")
    finally:
        launcher.running = False

if __name__ == "__main__":
    main()
