const express = require('express');
const mqtt = require('mqtt');
const cors = require('cors');
const nodemailer = require('nodemailer');

const bcrypt = require('bcrypt');
const jwt = require('jsonwebtoken');
const mongoose = require('mongoose');

const SECRET = 'your-very-secret-key'; // Change in production!

mongoose.connect('mongodb://localhost:27017/iot_system', {
  // useNewUrlParser: true,
  // useUnifiedTopology: true
});

// MongoDB Schema
const UserSchema = new mongoose.Schema({
  email: String,
  username: String,
  password: String, // hashed
  devices: [{
    deviceId: String,
    activatedAt: { type: Date, default: Date.now }
  }]
});
const User = mongoose.model('User', UserSchema);

const app = express();
app.use(cors());
app.use(express.json());

// MQTT Setup
const mqttClient = mqtt.connect('mqtt://192.168.29.98', {
  username: 'mps-bam100',
  password: 'bam100'
});

// Nodemailer Setup
const transporter = nodemailer.createTransport({
  service: 'gmail',
  auth: {
    user: 'arjun.robozz@gmail.com',
    pass: 'oikm sltx awxt irqg'
  }
});

// Temporary Activation Storage
const pendingActivations = new Map();

mqttClient.on('connect', () => {
  console.log('Connected to MQTT broker');
  mqttClient.subscribe('devices/register', (err) => {
    if (err) {
      console.error('Failed to subscribe:', err);
    } else {
      console.log('Subscribed to devices/register');
    }
  });
});

// Device Registration (from ESP32)
mqttClient.on('message', (topic, msg) => {
  console.log(`MQTT [${new Date().toISOString()}] Topic: ${topic}`);
  console.log(`Message: ${msg.toString()}`);


  if (topic === 'devices/register') {
    console.log("MQTT registration received:", msg.toString());
    console.log("MQTT message received:", topic, msg.toString());
    const { email, deviceId } = JSON.parse(msg);
    const code = Math.floor(100000 + Math.random() * 900000).toString();
    pendingActivations.set(email, { code, deviceId, expiry: Date.now() + 900000 });
    transporter.sendMail({
      from: 'IoT Admin <arjun.robozz@gmail.com>',
      to: email,
      subject: 'Device Activation Code',
      text: `Your activation code: ${code}\nExpires in 15 minutes.`
    });
  }
});

// Request Activation Code
app.post('/api/activate/request', (req, res) => {
  const { email } = req.body;
  const code = Math.floor(100000 + Math.random() * 900000).toString();
  const existing = pendingActivations.get(email) || {};
    // Keep deviceId if present
    pendingActivations.set(email, { ...existing, code, expiry: Date.now() + 900000 });

  transporter.sendMail({
    from: 'IoT Admin <arjun.robozz@gmail.com>',
    to: email,
    subject: 'Activation Code',
    text: `Your code: ${code}`
  }, (err) => {
    if (err) return res.status(500).json({ error: 'Email failed' });
    res.sendStatus(200);
  });
});

// Verify Activation Code and Register Username/Password
app.post('/api/activate/verify', async (req, res) => {
  const { email, code, username, password } = req.body;
  const record = pendingActivations.get(email);
  console.log("Pending activation record:", record);
  console.log("Activation verify called with:", req.body);
  console.log("Pending activation record:", record);

  if (!record || record.code !== code) {
    return res.status(400).json({ error: 'Invalid code' });
  }

  if (await User.findOne({ username })) {
    return res.status(400).json({ error: 'Username already taken' });
  }

  const hashed = await bcrypt.hash(password, 10);

  let user = await User.findOne({ email });
  if (!user) {
    user = new User({
      email,
      username,
      password: hashed,
      devices: [{ deviceId: record.deviceId, activatedAt: new Date() }]
    });
    console.log("Creating new user with device:", record.deviceId);
  } else {
    user.username = username;
    user.password = hashed;
    // Only add device if not already present
    if (!user.devices.some(d => d.deviceId === record.deviceId)) {
      user.devices.push({ deviceId: record.deviceId, activatedAt: new Date() });
      console.log("Adding device to existing user:", record.deviceId);
    } else {
      console.log("Device already present for user:", record.deviceId);
    }
  }
  await user.save();
  pendingActivations.delete(email);

  // Issue JWT token for session
  const token = jwt.sign({ username: user.username, email: user.email }, SECRET, { expiresIn: '30d' });
  res.json({ success: true, token, devices: user.devices });
});

// Login with Username/Password
app.post('/api/login', async (req, res) => {
  const { username, password } = req.body;
  const user = await User.findOne({ username });
  if (!user) return res.status(400).json({ error: 'Invalid credentials' });
  const valid = await bcrypt.compare(password, user.password);
  if (!valid) return res.status(400).json({ error: 'Invalid credentials' });
  const token = jwt.sign({ username: user.username, email: user.email }, SECRET, { expiresIn: '30d' });
  res.json({ token, devices: user.devices });
});

// Get Devices (for logged-in user)
app.get('/api/devices', async (req, res) => {
  const { token } = req.query;
  try {
    const decoded = jwt.verify(token, SECRET);
    const user = await User.findOne({ username: decoded.username });
    res.json(user?.devices || []);
  } catch {
    res.status(401).json({ error: 'Invalid token' });
  }
});

// Control Device
app.post('/api/control', (req, res) => {
  const { deviceId, command } = req.body;
  //mqttClient.publish(`IoT_Device_${deviceId}/cmd`, command);
  mqttClient.publish(`${deviceId}/cmd`, command);

  res.sendStatus(200);
});

app.listen(3001, () => console.log('Server running on port 3001'));
