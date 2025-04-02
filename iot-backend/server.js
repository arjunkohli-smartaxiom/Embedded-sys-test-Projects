const express = require('express');
const mqtt = require('mqtt');
const cors = require('cors');
const app = express();

// MQTT Connection
const mqttClient = mqtt.connect('mqtt://192.168.29.98', {
  username: 'mps-bam100',
  password: 'bam100'
});

// Mock Database
let devices = [
  { id: '2265b7a0', name: 'Living Room Light', status: 'OFF' }
];

// Middleware
app.use(cors());
app.use(express.json());

// API Routes
app.get('/api/devices', (req, res) => {
  res.json(devices);
});

app.post('/api/control', (req, res) => {
  const { deviceId, command } = req.body;
  mqttClient.publish(`IoT_Device_${deviceId}/cmd`, command);
  res.sendStatus(200);
});

// MQTT Handler
// Modify the MQTT handler to ensure proper state updates
mqttClient.on('message', (topic, msg) => {
    const parts = topic.split('/');
    if (parts.length === 3 && parts[2] === 'status') {
      const deviceId = parts[1];
      const status = msg.toString();
      
      devices = devices.map(d => 
        d.id === deviceId ? { ...d, status } : d
      );
      console.log(`Updated ${deviceId} to ${status}`); // Debug log
    }
  });

// Subscribe to all status topics
mqttClient.subscribe('+/status');

// Start Server
app.listen(3001, () => {
  console.log('Server running on http://localhost:3001');
});