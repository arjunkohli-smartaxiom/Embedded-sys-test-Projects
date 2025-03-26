import { Injectable, OnModuleDestroy, OnModuleInit } from '@nestjs/common';
import { InjectModel } from '@nestjs/mongoose';
import { Model } from 'mongoose';
import * as mqtt from 'mqtt';
import { Devices } from 'src/schemas/devices.schema';
import { Message } from 'src/schemas/message.schema';
import { DatabaseService } from './database.service';
import { HttpService } from '@nestjs/axios';
import { firstValueFrom } from 'rxjs';
const appRoot = require('app-root-path');
const conf = JSON.parse(require("fs").readFileSync(appRoot.path + "/env-config.json"));


@Injectable()
export class MqttService implements OnModuleInit, OnModuleDestroy {
  constructor(
    private databaseService: DatabaseService,
    private httpService: HttpService,
    @InjectModel(Message.name) private messageModel: Model<Message>,
    @InjectModel(Devices.name) private devicesModel: Model<Devices>
  ) { }

  private client: mqtt.MqttClient;
  private devices: Record<string, any> = {};
  private readonly INACTIVITY_TIMEOUT = 60000; // 60 seconds

  onModuleInit() {
    this.connectToBroker();
    setInterval(() => this.checkDeviceInactivity(), 60000); // Run every 60 seconds
  }

  private connectToBroker() {
    console.log("Mqtt connected");
    this.client = mqtt.connect(`mqtt://${conf.brokerUrl}:${conf.brokerPort}`, {
      username: conf.mqttUsername,
      password: conf.mqttPassword,
      keepalive: 120
    });

    this.client.on('connect', () => {
      console.log('Connected to MQTT Broker');
      // Subscribe to the global discovery topic
      this.client.subscribe('MPS/global/discovery', (err) => {
        if (err) console.error('Failed to subscribe:', err.message);
        else console.log('Subscribed to MPS/global/discovery');
      });

      // Subscribe to the global sessionPing topic
       
        this.client.subscribe('MPS/global/sessionPing', (err) => {
          if (err) console.error('Failed to subscribe:', err.message);
          else console.log('Subscribed to sessionPing topic');
        });
        
    });

    this.client.on('message', (topic, message) => {
      console.log(`Message received on ${topic}: ${message.toString()}`);
      
      if (topic === 'MPS/global/discovery') {
        this.handleDeviceDiscovery(message.toString());
      } else if (topic.includes('/config')) {
        this.handleDeviceConfig(message.toString(), topic);
      }  else if (topic.includes('/status')) {
        this.handleDeviceStatus(message.toString(), topic);
      }
      else if (topic.includes('/sessionPing')) {
        this.handleSessionPing(message.toString(), topic);
      }
      else if (topic.includes('/control')) {
        // this.handleDeviceControl(message.toString(), topic);
      } else {
        console.error(`Unexpected topic received: ${topic}`);
      }
    });

    this.client.on('error', (error) => {
      console.error('MQTT Error:', error.message);
    });
  }

  
// Extract device_id from topic
 private extractDeviceId (topic) {
  const topicParts = topic.split('/');
  return topicParts.length > 3 ? topicParts[3] : null;
};

  // private handleSessionPing(message, topic) {
  //   this.client.publish(`MPS/global/sessionPing`, JSON.stringify(message), { qos: 1 }, (err, msg) => {
  //     console.log('Sent Config Request', JSON.stringify(message));
  //     if (msg)
  //       console.log("Message Received:", msg);
  //     else if(err)
  //       console.log("Error:", err);
  //   });
  // }

  private handleSessionPing(payload: string, topic: string) {
    try {
      console.log('Received Ping Message', payload);
      const pingData = JSON.parse(payload);
      const { device_id, status } = pingData;

      console.log(`Ping Message: Device ID: ${device_id}, Status: ${status}`);

      if (this.devices[device_id]) {
        this.devices[device_id].lastPing = new Date();
        console.log(`Updated lastPing for Device: ${device_id}`);
      } else {
        console.log(`Unknown Device in Ping Message: ${device_id}`);
      }
    } catch (error) {
      console.error('Failed to process session ping:', error.message);
    }
  }


  private handleDeviceDiscovery(payload: string) {
    try {
      const deviceInfo = JSON.parse(payload);
      const { device_id, SNO, Firmware, MacAddr } = deviceInfo;
      console.log(`Device Info------ID: ${device_id}\nSNO: ${SNO}\nMAC: ${MacAddr}\nFirmware: ${Firmware}\n`);

      
      // Subscribe to device-specific topics
      const configTopics = 'MPS/global/UP/' + device_id + '/config';
      console.log(configTopics);
      this.client.subscribe(configTopics);

      const controlTopics = 'MPS/global/UP/' + device_id + '/control';
      console.log(controlTopics);
      this.client.subscribe(controlTopics);

      const statusTopics= 'MPS/global/UP/' + device_id + '/status';
      console.log(statusTopics);
      this.client.subscribe(statusTopics);

      console.log(`Subscribed to topics for device ${device_id}`);


      // Request full config
      const requestConfig = { ch_t : "LED", ch_addr : "LED1", cmd: 106, cmd_m: "config"};
      this.client.publish(`MPS/global/${device_id}/config`, JSON.stringify(requestConfig), { qos: 1 }, (err, msg) => {
        console.log('Sent Config Request', JSON.stringify(requestConfig));
        if (msg)
          console.log("Message Received:", msg);
        else if(err)
          console.log("Error:", err);
      });

      if (!this.devices[device_id]) {
        console.log(`Discovered device: ${device_id}`);
        this.devices[device_id] = { device_id, SNO, Firmware, MacAddr };
        // this.databaseService.storeDevice(deviceInfo);
        console.log(`Subscribed to topics for device ${device_id}`);     
      } else {
        console.log(`Device ${device_id} is already registered.`);
      }

    } catch (error) {
      console.error('Failed to decode discovery payload:', error.message);
    }
  }

  // Handle device Status 
  async handleDeviceStatus(payload, topic) {
    try {
      console.log('Received Device Status', payload);
      const statusData = JSON.parse(payload);
      const deviceId = this.extractDeviceId(topic);  // Extract correct device ID from topic

      if (this.devices.hasOwnProperty(deviceId)) {
        this.devices[deviceId].status = statusData;
        console.log(`✅ Received device status`, JSON.stringify(statusData));

        if (statusData.cmd === 115) {
          await firstValueFrom(
            this.httpService.post(`http://${conf.MPS_HOST}:${conf.MPS_PORT}/events`, { payload, topic })
          );
        }

      } else {
        console.error(`❌ Device ID ${deviceId} not found in devices list!`);
      }
    } catch (error) {
      console.error(`❌ Failed to parse control command from topic ${topic}:`, error.message);
    }
  };

  // Handle device control commands
  //payload : { ch_t: 'LED', ch_addr: 'LED6', cmd: 104, cmd_m: 'LED_OFF' }
  // For scene : { ch_t: 'LED', ch_addr: [2, 7, 8, 11], cmd: 117, cmd_m: 'LED_OFF' },
  // topic: device_id
  handleDeviceControl(payload, topic) {
    try {
      console.log('Received Control Command', payload);
      console.log('Received Topic', topic);
      const controlData = payload;
      const deviceId = topic;  // Extract correct device ID from topic

      if (this.devices.hasOwnProperty(deviceId)) {
      console.log(`✅ Control Command Processed`, `Device: ${deviceId}\nCommand: ${JSON.stringify(controlData)}`);
        if (payload.cmd === 117) {
          this.client.publish(`MPS/global/${deviceId}/scene`, JSON.stringify(controlData), { qos: 1 }, (err, msg:any) => {
            if (msg) {
              console.log("Message Received:", msg);
              this.databaseService.storeMessage(deviceId,`MPS/global/${deviceId}/scene`, JSON.stringify(msg));
            }
            else if(err)
              console.log("Error:", err);
          });
        } else {
          this.client.publish(`MPS/global/${deviceId}/control`, JSON.stringify(controlData), { qos: 1 }, (err, msg:any) => {
            if (msg) {
              console.log("Message Received:", msg);
              this.databaseService.storeMessage(deviceId,`MPS/global/${deviceId}/control`, JSON.stringify(msg));              
            }
            else if(err)
              console.log("Error:", err);
          });
        }
      } else {
        console.error(`❌ Device ID ${deviceId} not found in devices list!`);
        console.error(`📌 Available devices: ${Object.keys(this.devices).join(', ')}`);
      }
    } catch (error) {
      console.error(`❌ Failed to parse control command from topic ${topic}:`, error.message);
    }
  };

  // Handle full device config
  private handleDeviceConfig(payload, topic) {
    try {
      console.log('Received Device Config', payload);
      const configData = JSON.parse(payload);
      const device_id = this.extractDeviceId(topic);
      if (!device_id) {
          console.error(`❌ Invalid topic format: ${topic}`);
          return;
      }

      if (this.devices[device_id]) {
        this.devices[device_id].config = configData;
        console.log('✅ Device Configuration Updated', `Device: ${device_id}\nConfig: ${JSON.stringify(configData, null, 2)}`);
        this.databaseService.updateDeviceData(device_id, configData);
      } else {
        console.error(`❌ Device ID ${device_id} not found in devices list!`);
      }

      //const requestConfig = { ch_t : "LED", ch_addr : "LED1", cmd: 104, cmd_m: "LED_ON" };
      //this.client.publish(`MPS/Global/${device_id}/control`, JSON.stringify(requestConfig));
      //console.log('🔄 Sent Config Request', JSON.stringify(requestConfig));

    } catch (error) {
      console.error(`❌ Failed to parse device config from topic ${topic}:`, error.message);
    }
  };

  getDevices() {
    this.connectToBroker();
  }


  private checkDeviceInactivity() {
    const now = new Date();
    for (const deviceId in this.devices) {
        const device = this.devices[deviceId];
        const lastActive = device.lastPing ? new Date(device.lastPing).getTime() : 0;
        const timeSinceLastActive = now.getTime() - lastActive;

        if (timeSinceLastActive > this.INACTIVITY_TIMEOUT) {
            console.log(`Device ${deviceId} has been inactive for too long. Taking action...`);
            // Perform actions for inactive device, e.g., clean up resources, send notification
            this.handleInactiveDevice(deviceId);
        }
    }
}
  private handleInactiveDevice(deviceId: string) {
    console.log(`Handling inactive device ${deviceId}`);

    // Clean up resources associated with the device
    delete this.devices[deviceId];

    // Send a notification to administrators
    this.sendAdminNotification(`Device ${deviceId} is inactive.`);

    // Update the database to reflect the device's inactive state (optional)
    //this.updateDeviceStatusInDatabase(deviceId, false);
}
 private sendAdminNotification(message: string) {
  // Implement your notification logic here (e.g., send an email, log to a file, etc.)
  console.log('Admin Notification:', message);
  // Example: Send an email notification
  // this.emailService.sendAdminNotification(message);
}
//  private async updateDeviceStatusInDatabase(deviceId: string, isActive: boolean) {
//         try {
//             await this.devicesModel.updateOne(
//                 { device_id: deviceId },
//                 { $set: { isActive: isActive } }
//             );
//             console.log(`Updated device ${deviceId} status in the database to ${isActive}`);
//         } catch (error) {
//             console.error(`Failed to update device ${deviceId} status in the database:`, error.message);
//         }
//     }
    
  

  onModuleDestroy() {
    if (this.client) {
      this.client.end();
      console.log('Disconnected from MQTT Broker');
    }
  }
}
