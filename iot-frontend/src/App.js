import React, { useState, useEffect, useCallback } from 'react';
import axios from 'axios';
import { Button, List, ListItem, Typography, Box } from '@mui/material';

function App() {
  const [devices, setDevices] = useState([]);
  const [loading, setLoading] = useState(true);

  // Fetch devices
  const fetchDevices = useCallback(async () => {
    try {
      const res = await axios.get('http://localhost:3001/api/devices');
      setDevices(res.data);
    } catch (err) {
      console.error("Fetch error:", err);
    } finally {
      setLoading(false);
    }
  }, []);

  // Control device
  const controlDevice = async (deviceId, command) => {
    try {
      await axios.post('http://localhost:3001/api/control', {
        deviceId,
        command
      });
      fetchDevices(); // Refresh state immediately
    } catch (err) {
      console.error("Control error:", err);
    }
  };

  // Initial load + polling
  useEffect(() => {
    fetchDevices();
    const interval = setInterval(fetchDevices, 1000);
    return () => clearInterval(interval);
  }, [fetchDevices]);

  return (
    <Box sx={{ maxWidth: 600, mx: 'auto', p: 3 }}>
      <Typography variant="h6" gutterBottom>
        Device Control
      </Typography>

      {loading ? (
        <Typography>Loading...</Typography>
      ) : (
        <List>
          {devices.map(device => (
            <ListItem key={device.id}>
              <Typography sx={{ width: 150 }}>
                {device.name}: {device.status}
              </Typography>
              <Button 
                variant="contained" 
                sx={{ mx: 1 }}
                onClick={() => controlDevice(device.id, 'ON')}
              >
                ON
              </Button>
              <Button 
                variant="outlined"
                onClick={() => controlDevice(device.id, 'OFF')}
              >
                OFF
              </Button>
            </ListItem>
          ))}
        </List>
      )}
    </Box>
  );
}

export default App;