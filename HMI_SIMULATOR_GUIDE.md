# HMI Simulator Guide for BA100 System

## Overview
This guide explains how to use the HMI simulators to test your BA100 system without physical hardware. The HMI simulators mimic real Modbus-based HMI devices that can control LED channels and shades via MQTT.

## Files Created

### 1. `HMI_Simulator.py`
- **Purpose**: Single HMI device simulator
- **Features**: 
  - Modbus-style discovery messages
  - LED channel control (brightness, on/off)
  - Shade channel control (position, open/close)
  - Interactive command mode
  - MQTT communication

### 2. `Multi_HMI_Launcher.py`
- **Purpose**: Launch multiple HMI simulators simultaneously
- **Features**:
  - Support for 5-16 HMI devices
  - Pre-configured scenarios
  - Custom configurations
  - Threaded execution

### 3. `test_hmi_ba100_integration.py`
- **Purpose**: Test integration between HMI and BA100 simulators
- **Features**:
  - Monitors MQTT communication
  - Validates discovery messages
  - Tests control commands
  - Generates test reports

## Quick Start

### 1. Single HMI Testing
```bash
python HMI_Simulator.py
```

**Interactive Commands:**
- `led1 on/off` - Toggle LED1
- `led1 brightness 50` - Set LED1 to 50% brightness
- `shade1 open/close` - Toggle Shade1
- `shade1 position 75` - Set Shade1 to 75% position
- `status` - Show current status
- `discovery` - Send discovery message
- `quit` - Exit

### 2. Multiple HMI Testing
```bash
python Multi_HMI_Launcher.py
```

**Available Scenarios:**
1. **5 HMIs** (1 LED + 1 Shade each) - Basic testing
2. **10 HMIs** (2 LED + 2 Shade each) - Medium scale
3. **16 HMIs** (Mixed configurations) - Full scale
4. **Custom configuration** - User-defined setup
5. **Single HMI** (Interactive mode)

### 3. Integration Testing
```bash
python test_hmi_ba100_integration.py
```

## HMI Discovery Message Format

When an HMI boots up, it sends a discovery message:

```json
{
  "device_id": "HMI_001",
  "device_type": "Modbus_HMI",
  "SNO": "HMI123456",
  "Firmware": "v2.1.0",
  "MacAddr": "HM:AA:BB:CC:DD:EE:FF",
  "channels": {
    "led_channels": 1,
    "shade_channels": 1,
    "total_channels": 2
  },
  "led_config": ["LED1"],
  "shade_config": ["SHADE1"],
  "modbus_config": {
    "baud_rate": 9600,
    "parity": "none",
    "stop_bits": 1,
    "data_bits": 8
  }
}
```

## MQTT Topics

### HMI Topics
- **Discovery**: `MPS/global/discovery`
- **Config**: `MPS/global/{hmi_id}/config`
- **Control**: `MPS/global/{hmi_id}/control`
- **Status**: `MPS/global/UP/{hmi_id}/status`
- **Ping**: `MPS/global/UP/{hmi_id}/ping`

### BA100 Topics (for reference)
- **Discovery**: `MPS/global/discovery`
- **Config**: `MPS/global/{device_id}/config`
- **Control**: `MPS/global/{device_id}/control`
- **Status**: `MPS/global/UP/{device_id}/status`
- **Timer**: `MPS/global/{device_id}/timer`

## Control Commands

### LED Control
```json
{
  "ch_t": "LED",
  "ch_addr": "LED1",
  "cmd": 102,
  "cmd_m": "50"
}
```
- `cmd_m`: Brightness value (0-100)

### Shade Control
```json
{
  "ch_t": "SHADE",
  "ch_addr": "SHADE1",
  "cmd": 104,
  "cmd_m": "75"
}
```
- `cmd_m`: Position value (0-100)

## Testing Scenarios

### Scenario 1: Basic Integration
1. Start BA100_Simulator.py
2. Start HMI_Simulator.py
3. Verify discovery messages appear
4. Test LED and shade controls
5. Test PIR motion detection

### Scenario 2: Multiple HMIs
1. Start BA100_Simulator.py
2. Start Multi_HMI_Launcher.py
3. Select scenario (5, 10, or 16 HMIs)
4. Monitor MQTT broker for all devices
5. Test controls on different HMIs

### Scenario 3: Full System Test
1. Start BA100_Simulator.py
2. Start Multi_HMI_Launcher.py (16 HMIs)
3. Start test_hmi_ba100_integration.py
4. Run integration test
5. Verify all components communicate

## Real-World Usage

### Adding New HMIs
The system is designed to handle HMIs being added at any time:

1. **Boot Sequence**: HMI sends discovery message
2. **Configuration**: UI requests channel configuration
3. **Control**: HMI responds to control commands
4. **Status Updates**: HMI sends status updates

### Multiple HMI Support
- Each HMI has a unique ID (HMI_001, HMI_002, etc.)
- Each HMI can have different channel configurations
- All HMIs communicate independently via MQTT
- No conflicts between different HMI devices

### Future Expansion
The system supports:
- **5-16 HMI devices** as requested
- **Mixed configurations** (different LED/shade counts)
- **Dynamic addition** of new HMIs
- **Individual control** of each HMI

## Troubleshooting

### Common Issues

1. **HMI not appearing in discovery**
   - Check MQTT broker connection
   - Verify topic subscriptions
   - Check firewall settings

2. **Control commands not working**
   - Verify HMI ID in control topic
   - Check command format
   - Ensure HMI is connected

3. **Multiple HMIs conflicting**
   - Each HMI should have unique ID
   - Check MQTT topic paths
   - Verify channel addresses

### Debug Commands

**Check MQTT Topics:**
```bash
mosquitto_sub -h 192.168.29.128 -u mps-bam100 -P bam100 -t "MPS/global/discovery"
```

**Send Test Command:**
```bash
mosquitto_pub -h 192.168.29.128 -u mps-bam100 -P bam100 -t "MPS/global/HMI_001/control" -m '{"ch_t":"LED","ch_addr":"LED1","cmd":102,"cmd_m":"50"}'
```

## Integration with UI/API

The HMI simulators are designed to work seamlessly with your UI/API:

1. **Discovery**: UI detects new HMIs via discovery messages
2. **Configuration**: UI requests channel information
3. **Control**: UI sends control commands
4. **Status**: UI receives status updates
5. **Monitoring**: UI can monitor all HMI states

This allows you to develop and test your UI/API without physical hardware, ensuring everything works correctly before deployment.

## Next Steps

1. **Test with your UI**: Use the simulators to test your UI/API
2. **Scale testing**: Test with multiple HMIs (5-16)
3. **Integration testing**: Verify BA100-HMI communication
4. **Production deployment**: Deploy with confidence

The HMI simulators provide a complete testing environment that mirrors your production system, allowing you to develop and test with confidence.
