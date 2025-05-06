import React, { useState, useEffect } from 'react';
import axios from 'axios';
import { Button, TextField, Box, Typography } from '@mui/material';

export default function App() {
  const [email, setEmail] = useState('');
  const [code, setCode] = useState('');
  const [step, setStep] = useState(1); // 1=email, 2=code, 3=control

  const handleActivate = async () => {
    try {
      if (step === 1) {
        // Step 1: Request activation code
        await axios.post('http://localhost:3001/api/activate/request', { 
          email 
        });
        setStep(2);
      } else {
        // Step 2: Verify code
        const { data } = await axios.post('http://localhost:3001/api/activate/verify', { 
          email, 
          code 
        });
        if (data.success) setStep(3);
      }
    } catch (err) {
      console.error("Activation error:", err.response?.data || err.message);
      alert(`Activation failed: ${err.response?.data?.error || err.message}`);
    }
  };

  const controlDevice = (command) => {
    axios.post('http://localhost:3001/api/control', {
      deviceId: 'e438bcd8', // Replace with dynamic value
      command
    });
  };

  return (
    <Box sx={{ maxWidth: 400, mx: 'auto', p: 3 }}>
      {step === 1 ? (
        <>
          <TextField 
            label="Email" 
            fullWidth 
            value={email} 
            onChange={(e) => setEmail(e.target.value)} 
          />
          <Button onClick={handleActivate} variant="contained">
            Send Code
          </Button>
        </>
      ) : step === 2 ? (
        <>
          <TextField 
            label="Activation Code" 
            fullWidth 
            value={code} 
            onChange={(e) => setCode(e.target.value)} 
          />
          <Button onClick={handleActivate} variant="contained">
            Activate
          </Button>
        </>
      ) : (
        <>
          <Typography>Device Control</Typography>
          <Button onClick={() => controlDevice('ON')}>Turn ON</Button>
          <Button onClick={() => controlDevice('OFF')}>Turn OFF</Button>
        </>
      )}
    </Box>
  );
}