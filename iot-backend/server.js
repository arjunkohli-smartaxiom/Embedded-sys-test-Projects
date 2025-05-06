const express = require('express');
const mqtt = require('mqtt');
const cors = require('cors');
const nodemailer = require('nodemailer');

const app = express();
app.use(cors());
app.use(express.json());

// MQTT setup
const mqttClient = mqtt.connect('mqtt://192.168.29.98', {
  username: 'mps-bam100',
  password: 'bam100'
});

// Nodemailer setup
const transporter = nodemailer.createTransport({
  service: 'gmail',
  auth: {
    user: 'arjun.robozz@gmail.com', // Your Gmail address
    pass: 'oikm sltx awxt irqg'      // App-specific password
  }
});

// Temporary storage for activation codes
const pendingActivations = new Map();

// MQTT device registration (optional, if used)
mqttClient.on('message', (topic, msg) => {
  if (topic === 'devices/register') {
    const { email, deviceId } = JSON.parse(msg);
    const code = Math.floor(100000 + Math.random() * 900000).toString();
    pendingActivations.set(email, { code, deviceId, expiry: Date.now() + 900000 });

    // Send code to user's email
    transporter.sendMail({
      from: 'IoT Admin <arjun.robozz@gmail.com>',
      to: email, // <-- Send to user's email
      subject: 'Your Device Activation Code',
      text: `Use this code to activate your device: ${code}\nExpires in 15 minutes.`
    }, (err, info) => {
      if (err) {
        console.error("Email failed:", err);
        pendingActivations.delete(email);
      } else {
        console.log("Email sent:", info.response);
      }
    });
  }
});

// Endpoint to request activation code
app.post('/api/activate/request', (req, res) => {
  const { email } = req.body;

  // Generate and store code
  const code = Math.floor(100000 + Math.random() * 900000).toString();
  pendingActivations.set(email, { code, expiry: Date.now() + 900000 }); // 15 min expiry

  // Send email to user's email address
  transporter.sendMail({
    from: 'IoT Admin <arjun.robozz@gmail.com>',
    to: email, // <-- Send to user's email
    subject: 'Your Device Activation Code',
    text: `Use this code to activate your device: ${code}\nExpires in 15 minutes.`
  }, (err, info) => {
    if (err) {
      console.error("Email failed:", err);
      pendingActivations.delete(email);
      return res.status(500).json({ error: "Failed to send activation email" });
    }
    console.log("Email sent:", info.response);
    res.sendStatus(200);
  });
});

// Endpoint to verify activation code
app.post('/api/activate/verify', (req, res) => {
  const { email, code } = req.body;
  const record = pendingActivations.get(email);

  if (!record) {
    return res.status(400).json({ error: "No pending activation for this email" });
  }
  if (record.code !== code) {
    return res.status(400).json({ error: "Invalid code" });
  }
  if (Date.now() > record.expiry) {
    pendingActivations.delete(email);
    return res.status(400).json({ error: "Code expired" });
  }

  // Activation successful
  pendingActivations.delete(email);
  res.json({ success: true });
});

// Endpoint to send MQTT command
app.post('/api/control', (req, res) => {
  const { deviceId, command } = req.body;
  mqttClient.publish(`IoT_Device_${deviceId}/cmd`, command);
  res.sendStatus(200);
});

app.listen(3001, () => console.log('Server running on port 3001'));
