# Medicare Bot - MQTT Payload Guide

## Overview

Medicare Bot publishes health data to an MQTT broker as a JSON payload. This guide explains the topic structure, the payload format, each field, and how to subscribe to and use the data.

## Topic

The device publishes to a single topic that you configure in the sketch. Choose a clear, hierarchical name such as a device or patient identifier so multiple devices can coexist on the same broker without collisions.

## Payload Format

Data is published as a JSON object every five seconds. A typical payload looks like this:

```json
{
  "bodyTemp": 37.25,
  "spo2": 98,
  "heartRate": 72,
  "alert": false,
  "emergency": false,
  "timestamp": 125430,
  "device": "Medicare-Bot"
}
```

## Field Reference

bodyTemp is the body temperature in degrees Celsius, calculated from the thermistor using the Steinhart-Hart equation.

spo2 is the blood oxygen saturation as a percentage. A value of zero indicates the sensor did not produce a valid reading.

heartRate is the heart rate in beats per minute. A value of zero indicates no valid reading.

alert is a boolean that becomes true when any monitored value crosses its configured threshold.

emergency is a boolean that becomes true while the emergency button is pressed.

timestamp is the number of milliseconds since the device booted. It is useful for ordering messages, though it resets whenever the device restarts.

device is a string identifier for the publishing device, helpful when several devices share one broker.

## Alert Conditions

The alert field is set to true when heart rate rises above the high threshold or falls below the low threshold, when temperature rises above the high threshold or falls below the low threshold, or when SpO2 falls below its low threshold. The specific threshold values are defined in the sketch and can be adjusted.

## Sample Payloads

Normal reading with all values in range:

```json
{
  "bodyTemp": 36.80,
  "spo2": 97,
  "heartRate": 68,
  "alert": false,
  "emergency": false,
  "timestamp": 45000,
  "device": "Medicare-Bot"
}
```

Reading that triggers an alert due to elevated heart rate and temperature:

```json
{
  "bodyTemp": 39.10,
  "spo2": 96,
  "heartRate": 128,
  "alert": true,
  "emergency": false,
  "timestamp": 90000,
  "device": "Medicare-Bot"
}
```

Reading where the emergency button was pressed:

```json
{
  "bodyTemp": 37.00,
  "spo2": 98,
  "heartRate": 74,
  "alert": false,
  "emergency": true,
  "timestamp": 132000,
  "device": "Medicare-Bot"
}
```

## Subscribing with Python

The following example uses the paho-mqtt library. Replace the broker details and topic with your own values.

```python
import paho.mqtt.client as mqtt
import json
import ssl

def on_message(client, userdata, msg):
    data = json.loads(msg.payload.decode())
    print(f"Temp: {data['bodyTemp']} C | HR: {data['heartRate']} BPM | SpO2: {data['spo2']} %")
    if data['alert']:
        print("Alert condition detected")
    if data['emergency']:
        print("Emergency button pressed")

client = mqtt.Client()
client.username_pw_set("your_username", "your_password")
client.tls_set(cert_reqs=ssl.CERT_NONE, tls_version=ssl.PROTOCOL_TLSv1_2)
client.tls_insecure_set(True)
client.on_message = on_message
client.connect("your_broker_host", 8883)
client.subscribe("your/topic")
client.loop_forever()
```

## Subscribing with Node.js

The following example uses the mqtt package. Replace the connection details and topic with your own values.

```javascript
const mqtt = require('mqtt');

const client = mqtt.connect('mqtts://your_broker_host:8883', {
  username: 'your_username',
  password: 'your_password',
  rejectUnauthorized: false
});

client.on('connect', () => {
  client.subscribe('your/topic');
});

client.on('message', (topic, message) => {
  const data = JSON.parse(message.toString());
  console.log(`Temp: ${data.bodyTemp} C | HR: ${data.heartRate} BPM | SpO2: ${data.spo2} %`);
});
```

## Subscribing from the Command Line

Using the mosquitto client tools, you can subscribe to the topic to watch messages arrive. Provide your broker host, port, username, password, and topic. Because the broker uses TLS, you may also need to point to a CA certificate file depending on your setup.

## Integrating with a Dashboard

The JSON payload maps naturally onto dashboard tools. Home Assistant can read individual fields using value templates on an MQTT sensor. Node-RED can parse the JSON in a function node and route each field to gauges or charts. A time-series database such as InfluxDB can store each field for long-term trends and visualization in Grafana.

## Bandwidth Notes

Each payload is roughly 120 bytes. At a five-second publish interval, this comes to about one message every five seconds, which is modest and suitable for most broker plans. If you need to reduce traffic, increase the publish interval in the sketch.

## Security Notes

Health data is sensitive, so publishing over an encrypted TLS connection is strongly recommended. Keep broker credentials out of any shared code or public repository, and restrict topic access on the broker where possible so only authorized clients can read the data.
