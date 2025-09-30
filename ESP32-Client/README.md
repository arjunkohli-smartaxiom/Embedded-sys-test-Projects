# ESP32 PIR Sensor with MQTT Integration

A comprehensive ESP32 project that integrates PIR (Passive Infrared) motion sensors with MQTT broker communication, featuring dynamic sensor configuration, automatic timer-based sense control, and real-time motion detection.

## 🚀 Features

### Core Functionality
- **PIR Motion Detection**: Real-time motion detection using RS485 Modbus communication
- **MQTT Integration**: Seamless communication with MQTT broker for IoT applications
- **Dynamic Configuration**: Runtime sensor ID and port configuration without code changes
- **Timer-Based Control**: Automatic sense turn-off after configurable timeout periods
- **Serial Commands**: Interactive configuration via serial monitor

### Advanced Features
- **Multi-Sensor Support**: Support for multiple PIR sensors with different IDs and ports
- **Scene Command Processing**: Integration with lighting control systems
- **WiFi Configuration**: EEPROM-based credential storage and management
- **Error Handling**: Robust communication error detection and recovery
- **Real-time Status**: Live motion status reporting and monitoring

## 📋 Hardware Requirements

### ESP32 Development Board
- **Board**: ESP32-DOIT-DEVKIT-V1
- **Framework**: Arduino
- **Platform**: Espressif32

### PIR Sensor
- **Type**: ZT-IRSXD011-T485 (RS485 Modbus)
- **Communication**: RS485 Serial
- **Protocol**: Modbus RTU
- **Power**: 12V DC

### Pin Configuration
```
ESP32 Pin    →    Function
GPIO 4       →    DE (Data Enable) Pin
GPIO 16      →    RX (Receive) Pin  
GPIO 17      →    TX (Transmit) Pin
```

## 🔧 Software Dependencies

### Required Libraries
```ini
lib_deps =
    PubSubClient      # MQTT communication
    ArduinoJson       # JSON message handling
    EEPROM            # Configuration storage
    ModbusMaster      # RS485 Modbus communication
```

### PlatformIO Configuration
```ini
[env:esp32doit-devkit-v1]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 115200
```

## 🚀 Quick Start

### 1. Hardware Setup
1. Connect PIR sensor to ESP32:
   - **DE Pin**: Connect to GPIO 4
   - **RX Pin**: Connect to GPIO 16
   - **TX Pin**: Connect to GPIO 17
   - **Power**: Connect 12V DC supply

2. Connect ESP32 to computer via USB

### 2. Software Setup
1. **Clone Repository**:
   ```bash
   git clone <repository-url>
   cd ESP32-Client
   ```

2. **Install Dependencies**:
   ```bash
   pio lib install
   ```

3. **Configure WiFi**:
   - Create `src/password.h` file:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```

4. **Upload Code**:
   ```bash
   pio run --target upload
   ```

### 3. Initial Configuration
1. **Open Serial Monitor** (115200 baud)
2. **Configure Network**:
   ```
   config
   # Enter MQTT server IP
   # Enter WiFi SSID
   # Enter WiFi password
   ```
3. **Or Skip Configuration**:
   ```
   skip
   ```

## 📡 MQTT Integration

### Message Format
The device sends PIR status messages in the following format:

```json
{
  "ch_t": "PIR",
  "ch_addr": "Port-1_1",
  "cmd": 115,
  "cmd_m": "PIR State = 1"
}
```

### Topics
- **Discovery**: `MPS/global/discovery`
- **Status**: `MPS/global/UP/{device_id}/status`
- **Control**: `MPS/global/{device_id}/control`
- **Config**: `MPS/global/{device_id}/config`

### Message Types
1. **Motion Detected**: `"PIR State = 1"`
2. **No Motion**: `"PIR State = 0"`
3. **Timer Timeout**: `"SENSE_TIMEOUT"`

## ⚙️ Configuration Commands

### Serial Monitor Commands

#### Basic Commands
```
show          # Display current configuration
config        # Enter configuration mode
skip          # Use current settings
clear         # Reset to defaults
```

#### PIR Sensor Commands
```
pirconfig     # Show PIR sensor configuration
pirstatus     # Show PIR sensor status
pirid X       # Set sensor ID to X (1-255)
pirport X     # Set port to X (1-10)
pirtimer X    # Set timeout to X seconds (5-3600)
```

### Example Usage
```
# Change sensor ID to 2
pirid 2

# Change port to 3
pirport 3

# Set timeout to 60 seconds
pirtimer 60

# Show current configuration
pirconfig
```

## 🔄 Dynamic Configuration

### Sensor ID Management
- **Default**: Sensor ID = 1
- **Range**: 1-255
- **Change**: Use `pirid X` command
- **Effect**: Updates Modbus communication target

### Port Configuration
- **Default**: Port = 1
- **Range**: 1-10
- **Change**: Use `pirport X` command
- **Effect**: Updates MQTT message address format

### Timer Settings
- **Default**: 30 seconds (testing)
- **Production**: 15 minutes (900 seconds)
- **Change**: Use `pirtimer X` command
- **Effect**: Controls automatic sense turn-off timing

## 🎯 Timer Logic

### Motion Detection Flow
1. **Motion Detected** → Start timer → Send "PIR State = 1"
2. **No Motion** → Send "PIR State = 0"
3. **Timer Timeout** → Send "SENSE_TIMEOUT" → Reset motion state

### Timer Callback
```cpp
void pir_motion_timer_callback() {
  if (pirSensor.motion_detected == 1) {
    pirSensor.motion_detected = 0;
    pirSensor.last_send_status = 0;
    sendPIRStatusUpdate("no_motion");
    pirSensor.timer_active = false;
  }
}
```

## 📊 Status Monitoring

### Serial Output
```
🔍 PIR Sensor Configuration:
   Sensor ID: 1
   Port: 1
   Port Address: Port-1_1
   Motion Status: NO MOTION
   Timer Active: NO
   Timeout: 30 seconds
   DE Pin: 4
   RX Pin: 16
   TX Pin: 17
```

### MQTT Messages
```
📤 Sent PIR Status: {"ch_t":"PIR","ch_addr":"Port-1_1","cmd":115,"cmd_m":"PIR State = 1"}
📤 Sent Sense Timeout: {"ch_t":"PIR","ch_addr":"Port-1_1","cmd":115,"cmd_m":"SENSE_TIMEOUT"}
```

## 🔧 Troubleshooting

### Common Issues

#### 1. PIR Sensor Not Detected
- **Check**: RS485 wiring (DE, RX, TX pins)
- **Check**: Power supply (12V DC)
- **Check**: Sensor ID configuration
- **Solution**: Use `pirid X` to match sensor ID

#### 2. MQTT Connection Failed
- **Check**: WiFi credentials
- **Check**: MQTT broker IP address
- **Check**: Network connectivity
- **Solution**: Use `config` command to update settings

#### 3. No Motion Detection
- **Check**: Sensor ID matches PIR sensor
- **Check**: RS485 communication
- **Check**: Serial monitor for error messages
- **Solution**: Verify sensor ID with `pirconfig`

### Debug Commands
```
show          # Check overall status
pirconfig     # Check PIR sensor status
pirstatus     # Detailed PIR information
```

## 📁 Project Structure

```
ESP32-Client/
├── src/
│   ├── main.cpp              # Main application code
│   └── password.h            # WiFi credentials
├── include/                  # Header files
├── lib/                      # Library files
├── test/                     # Test files
├── platformio.ini           # PlatformIO configuration
└── README.md                # This file
```

## 🚀 Deployment

### Production Settings
1. **Change Timer**: Set to 15 minutes (900 seconds)
2. **Update Credentials**: Configure WiFi and MQTT
3. **Test Communication**: Verify MQTT message reception
4. **Monitor Status**: Check serial output for errors

### Testing Settings
1. **Short Timer**: Use 30 seconds for quick testing
2. **Multiple Sensors**: Test different sensor IDs
3. **Port Changes**: Verify dynamic port configuration

## 📝 License

This project is open source and available under the MIT License.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📞 Support

For issues and questions:
- Check the troubleshooting section
- Review serial monitor output
- Verify hardware connections
- Test with different sensor configurations

---

**Note**: This project is designed for IoT applications requiring reliable PIR motion detection with MQTT integration. The dynamic configuration system allows for easy adaptation to different sensor setups and network requirements.

