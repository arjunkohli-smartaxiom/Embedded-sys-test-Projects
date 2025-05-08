import React, { useState, useEffect } from 'react';
import axios from 'axios';
import { Button, TextField, Box, Typography, Select, MenuItem } from '@mui/material';

export default function App() {
  const [step, setStep] = useState(1);
  const [email, setEmail] = useState('');
  const [code, setCode] = useState('');
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [token, setToken] = useState(localStorage.getItem('iot-token') || '');
  const [devices, setDevices] = useState([]);
  const [selectedDevice, setSelectedDevice] = useState('');

  // Auto-login if token exists
  useEffect(() => {
    if (token) {
      axios.get(`http://localhost:3001/api/devices?token=${token}`)
        .then(res => {
          setDevices(res.data);
          setStep(3);
        })
        .catch(() => {
          setToken('');
          localStorage.removeItem('iot-token');
        });
    }
  }, [token]);

  // Activation flow
  const handleActivate = async () => {
    try {
      if (step === 1) {
        await axios.post('http://localhost:3001/api/activate/request', { email });
        setStep(2);
      } else if (step === 2) {
        setStep(4); // Go to set username/password
      }
    } catch (err) {
      alert(err.response?.data?.error || 'Activation failed');
    }
  };

  // Register username/password after code
  const handleRegister = async () => {
    try {
      const { data } = await axios.post('http://localhost:3001/api/activate/verify', {
        email, code, username, password
      });
      if (data.success) {
        localStorage.setItem('iot-token', data.token);
        setToken(data.token);
        setDevices(data.devices);
        setStep(3);
      }
    } catch (err) {
      alert(err.response?.data?.error || 'Registration failed');
    }
  };

  // Login flow
  const handleLogin = async () => {
    try {
      const { data } = await axios.post('http://localhost:3001/api/login', { username, password });
      localStorage.setItem('iot-token', data.token);
      setToken(data.token);
      setDevices(data.devices);
      setStep(3);
    } catch (err) {
      alert(err.response?.data?.error || 'Login failed');
    }
  };

  const controlDevice = (command) => {
    axios.post('http://localhost:3001/api/control', {
      deviceId: selectedDevice,
      command
    });
  };

  return (
    <Box sx={{ maxWidth: 400, mx: 'auto', p: 3 }}>
      {/* Activation or Login Choice */}
      {step === 1 && (
        <>
          <Typography variant="h6">Register Device</Typography>
          <TextField
            label="Email"
            fullWidth
            value={email}
            onChange={e => setEmail(e.target.value)}
            sx={{ mb: 2 }}
          />
          <Button onClick={handleActivate} variant="contained" sx={{ mr: 2 }}>
            Send Activation Code
          </Button>
          <Button onClick={() => setStep(5)} variant="outlined">
            Already have account? Login
          </Button>
        </>
      )}
  
      {/* Enter Activation Code */}
      {step === 2 && (
        <>
          <TextField
            label="Activation Code"
            fullWidth
            value={code}
            onChange={e => setCode(e.target.value)}
            sx={{ mb: 2 }}
          />
          <Button onClick={handleActivate} variant="contained">
            Next
          </Button>
        </>
      )}
  
      {/* Set Username/Password */}
      {step === 4 && (
        <>
          <TextField
            label="Choose Username"
            fullWidth
            value={username}
            onChange={e => setUsername(e.target.value)}
            sx={{ mb: 2 }}
          />
          <TextField
            label="Choose Password"
            type="password"
            fullWidth
            value={password}
            onChange={e => setPassword(e.target.value)}
            sx={{ mb: 2 }}
          />
          <Button onClick={handleRegister} variant="contained">
            Register & Finish
          </Button>
        </>
      )}
  
      {/* Login */}
      {step === 5 && (
        <>
          <Typography variant="h6">Login</Typography>
          <TextField
            label="Username"
            fullWidth
            value={username}
            onChange={e => setUsername(e.target.value)}
            sx={{ mb: 2 }}
          />
          <TextField
            label="Password"
            type="password"
            fullWidth
            value={password}
            onChange={e => setPassword(e.target.value)}
            sx={{ mb: 2 }}
          />
          <Button onClick={handleLogin} variant="contained">
            Login
          </Button>
        </>
      )}
  
      {/* Device Control with Logout */}
      {step === 3 && (
        <>
          <Box display="flex" justifyContent="space-between" alignItems="center" mb={2}>
            <Typography variant="h6" gutterBottom>
              Device Control
            </Typography>
            <Button
              variant="outlined"
              color="error"
              onClick={() => {
                localStorage.removeItem('iot-token');
                setToken('');
                setStep(1);
                setEmail('');
                setUsername('');
                setPassword('');
                setDevices([]);
                setSelectedDevice('');
              }}
            >
              Logout
            </Button>
          </Box>
          <Select
            fullWidth
            value={selectedDevice}
            onChange={e => setSelectedDevice(e.target.value)}
            displayEmpty
            sx={{ mb: 2 }}
          >
            <MenuItem value="" disabled>Select a device</MenuItem>
            {devices.map(device => (
              <MenuItem key={device.deviceId} value={device.deviceId}>
                {device.deviceId}
              </MenuItem>
            ))}
          </Select>
          <Button
            variant="contained"
            onClick={() => controlDevice('ON')}
            disabled={!selectedDevice}
            sx={{ mr: 2 }}
          >
            Turn ON
          </Button>
          <Button
            variant="contained"
            onClick={() => controlDevice('OFF')}
            disabled={!selectedDevice}
          >
            Turn OFF
          </Button>
        </>
      )}
    </Box>
  );
  
}
