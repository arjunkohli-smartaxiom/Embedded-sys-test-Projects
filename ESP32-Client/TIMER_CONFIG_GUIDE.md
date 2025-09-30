# ESP32 Timer Configuration via MQTT

## 🎯 Overview
Added remote timer configuration capability to the ESP32 PIR motion detection system. Now you can configure the timer value from your UI/API via MQTT instead of just from the serial monitor.

## 🔧 What's New

### 1. **EEPROM Storage for Timer Value**
- Timer value is now saved to EEPROM and persists across reboots
- Default timer: 30 seconds (if no saved value)
- Range: 5 seconds to 1 hour (3600 seconds)

### 2. **New MQTT Topic: `/timer`**
- **Topic**: `MPS/global/{device_id}/timer`
- **Purpose**: Configure timer value remotely

### 3. **MQTT Commands**

#### Set Timer Value (Command 200)
```json
{
  "cmd": 200,
  "timer_value": 60
}
```
- Sets timer to 60 seconds
- Valid range: 5-3600 seconds
- Saves to EEPROM automatically

#### Get Current Timer Value (Command 202)
```json
{
  "cmd": 202
}
```
- Returns current timer value
- No parameters needed

### 4. **Response Messages**
All responses are published to: `MPS/global/UP/{device_id}/status`

#### Success Response
```json
{
  "device_id": "123456",
  "ch_t": "TIMER",
  "ch_addr": "TIMER_CONFIG",
  "cmd": 201,
  "timer_value": 60,
  "status": "success"
}
```

#### Error Response
```json
{
  "device_id": "123456",
  "ch_t": "TIMER",
  "ch_addr": "TIMER_CONFIG",
  "cmd": 201,
  "timer_value": 30,
  "status": "error",
  "error": "Invalid timer value. Must be 5-3600 seconds"
}
```

#### Current Timer Status
```json
{
  "device_id": "123456",
  "ch_t": "TIMER",
  "ch_addr": "TIMER_STATUS",
  "cmd": 203,
  "timer_value": 60,
  "status": "current"
}
```

## 🚀 Usage Examples

### From Python/UI:
```python
import paho.mqtt.client as mqtt
import json

# Set timer to 2 minutes
timer_config = {
    "cmd": 200,
    "timer_value": 120
}
client.publish("MPS/global/123456/timer", json.dumps(timer_config))

# Get current timer value
timer_request = {
    "cmd": 202
}
client.publish("MPS/global/123456/timer", json.dumps(timer_request))
```

### From Serial Monitor:
```
ESP32> pirtimer 90
✅ Timer value updated to: 90 seconds

ESP32> timerstatus
⏰ Current Timer Configuration:
   Timer Value: 90 seconds
   Range: 5-3600 seconds
   Status: Inactive
```

## 🧪 Testing

### 1. **ESP32 Hardware Testing**
- Flash the updated code to your ESP32
- Use serial monitor commands: `pirtimer X` and `timerstatus`
- Test MQTT commands using MQTT client

### 2. **Python Simulator Testing**
```bash
python esp32_simulator_complete.py
```
- Use `timer X` command to set timer
- Use `timerstatus` command to check current value
- Test MQTT timer configuration

### 3. **MQTT Testing**
```bash
python test_timer_config.py
```
- Interactive script to test timer configuration via MQTT
- Set different timer values
- Get current timer status

## 📁 Files Modified

### ESP32 Code (`src/main.cpp`)
- ✅ Added EEPROM storage for timer value
- ✅ Added `/timer` MQTT topic subscription
- ✅ Added `handleTimerCommand()` function
- ✅ Added `saveTimerValue()` and `loadTimerValue()` functions
- ✅ Added serial commands: `pirtimer X` and `timerstatus`
- ✅ Timer value loaded from EEPROM on startup

### Python Simulator (`esp32_simulator_complete.py`)
- ✅ Added timer topic subscription
- ✅ Added `handle_timer_command()` function
- ✅ Added `timerstatus` command
- ✅ Timer configuration via MQTT

### Test Script (`test_timer_config.py`)
- ✅ Interactive MQTT timer configuration tester
- ✅ Set/get timer values remotely
- ✅ Response monitoring

## 🎯 Benefits

1. **Remote Configuration**: Change timer from UI/API without physical access
2. **Persistent Storage**: Timer value survives reboots
3. **Validation**: Prevents invalid timer values (5-3600 seconds)
4. **Real-time Updates**: Immediate effect on motion detection logic
5. **Backward Compatible**: Serial monitor commands still work
6. **Error Handling**: Clear error messages for invalid values

## 🔄 Workflow

1. **UI sends timer configuration** → MQTT topic `/timer`
2. **ESP32 receives command** → Validates range (5-3600 seconds)
3. **ESP32 updates timer** → Saves to EEPROM, updates `SENSE_TIMEOUT`
4. **ESP32 sends confirmation** → MQTT topic `/status`
5. **New timer value takes effect** → Immediately for new motion detection

## 🎉 Ready for Production!

Your ESP32 PIR motion detection system now supports:
- ✅ Remote timer configuration via MQTT
- ✅ Persistent timer storage
- ✅ Real-time timer updates
- ✅ Comprehensive error handling
- ✅ Both hardware and simulator support

Perfect for your UI/API integration! 🚀
