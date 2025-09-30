# Multi-Sensor Timer Configuration Guide

## 🎯 **Overview**
Your ESP32 PIR motion detection system now supports **multiple sensors** with individual timer configurations! Each sensor can have its own timer value, making it perfect for complex installations with multiple PIR sensors.

## 🔧 **What's New**

### **1. Multi-Sensor Support**
- ✅ **Individual timers** for each sensor/port combination
- ✅ **EEPROM storage** for up to 10 sensors
- ✅ **Sensor-specific configuration** via MQTT
- ✅ **Backward compatible** with single sensor setup

### **2. Enhanced MQTT Commands**

#### **Set Timer for Specific Sensor (Command 200)**
```json
{
  "cmd": 200,
  "sensor_id": 1,
  "port": 1,
  "timer_value": 60
}
```

#### **Get Timer for Specific Sensor (Command 202)**
```json
{
  "cmd": 202,
  "sensor_id": 1,
  "port": 1
}
```

#### **Get All Sensors Status (Command 203)**
```json
{
  "cmd": 203
}
```

## 🚀 **MQTT Commands**

### **Set Timer for Sensor ID 1, Port 1:**
```bash
mosquitto_pub -h 192.168.29.128 -u mps-bam100 -P bam100 -t "MPS/global/123456/timer" -m '{"cmd": 200, "sensor_id": 1, "port": 1, "timer_value": 60}'
```

### **Set Timer for Sensor ID 2, Port 2:**
```bash
mosquitto_pub -h 192.168.29.128 -u mps-bam100 -P bam100 -t "MPS/global/123456/timer" -m '{"cmd": 200, "sensor_id": 2, "port": 2, "timer_value": 120}'
```

### **Get Timer for Sensor ID 1, Port 1:**
```bash
mosquitto_pub -h 192.168.29.128 -u mps-bam100 -P bam100 -t "MPS/global/123456/timer" -m '{"cmd": 202, "sensor_id": 1, "port": 1}'
```

### **Get All Sensors Status:**
```bash
mosquitto_pub -h 192.168.29.128 -u mps-bam100 -P bam100 -t "MPS/global/123456/timer" -m '{"cmd": 203}'
```

## 📋 **Response Examples**

### **Success Response:**
```json
{
  "device_id": "123456",
  "ch_t": "TIMER",
  "ch_addr": "TIMER_CONFIG",
  "cmd": 201,
  "sensor_id": 1,
  "port": 1,
  "timer_value": 60,
  "status": "success"
}
```

### **Current Timer Response:**
```json
{
  "device_id": "123456",
  "ch_t": "TIMER",
  "ch_addr": "TIMER_STATUS",
  "cmd": 203,
  "sensor_id": 1,
  "port": 1,
  "timer_value": 60,
  "status": "current"
}
```

### **All Sensors Response:**
```json
{
  "device_id": "123456",
  "ch_t": "TIMER",
  "ch_addr": "ALL_SENSORS",
  "cmd": 204,
  "sensors": [
    {
      "sensor_id": 2,
      "port": 1,
      "timer_value": 30,
      "active": false
    }
  ]
}
```

## 🧪 **Testing**

### **1. Test with Python Script:**
```bash
python test_multi_sensor_timer.py
```

### **2. Test with Python Simulator:**
```bash
python esp32_simulator_complete.py
```

### **3. Test with MQTT Commands:**
```bash
# Set different timers for different sensors
mosquitto_pub -h 192.168.29.128 -u mps-bam100 -P bam100 -t "MPS/global/123456/timer" -m '{"cmd": 200, "sensor_id": 1, "port": 1, "timer_value": 30}'
mosquitto_pub -h 192.168.29.128 -u mps-bam100 -P bam100 -t "MPS/global/123456/timer" -m '{"cmd": 200, "sensor_id": 2, "port": 1, "timer_value": 60}'
mosquitto_pub -h 192.168.29.128 -u mps-bam100 -P bam100 -t "MPS/global/123456/timer" -m '{"cmd": 200, "sensor_id": 1, "port": 2, "timer_value": 90}'
```

## 🎯 **Real-World Scenarios**

### **Scenario 1: Office Building**
- **Sensor 1, Port 1**: 30 seconds (high traffic area)
- **Sensor 2, Port 1**: 60 seconds (meeting room)
- **Sensor 1, Port 2**: 90 seconds (storage room)

### **Scenario 2: Home Automation**
- **Sensor 1, Port 1**: 15 seconds (bathroom)
- **Sensor 2, Port 1**: 45 seconds (living room)
- **Sensor 3, Port 1**: 120 seconds (garage)

### **Scenario 3: Warehouse**
- **Sensor 1, Port 1**: 20 seconds (loading dock)
- **Sensor 2, Port 1**: 180 seconds (storage area)
- **Sensor 1, Port 2**: 60 seconds (office area)

## 🔧 **Technical Details**

### **EEPROM Storage:**
- **Base Address**: 320
- **Per Sensor**: 4 bytes
- **Max Sensors**: 10
- **Total Storage**: 40 bytes

### **Sensor Index Calculation:**
```
sensor_index = sensor_id * 10 + port
```

### **Timer Range:**
- **Minimum**: 5 seconds
- **Maximum**: 3600 seconds (1 hour)
- **Default**: 30 seconds

## 🎉 **Benefits**

1. **Flexible Configuration**: Each sensor can have different timer values
2. **Scalable**: Support up to 10 sensors
3. **Persistent Storage**: Timer values survive reboots
4. **Real-time Updates**: Changes take effect immediately
5. **Backward Compatible**: Works with existing single sensor setup
6. **UI Ready**: Perfect for complex UI/API integration

## 🚀 **Ready for Production!**

Your ESP32 system now supports:
- ✅ **Multiple PIR sensors** with individual timers
- ✅ **Remote configuration** via MQTT
- ✅ **Persistent storage** in EEPROM
- ✅ **Real-time updates** without reboot
- ✅ **Comprehensive error handling**
- ✅ **Full simulator support** for testing

Perfect for complex installations with multiple sensors! 🎯
