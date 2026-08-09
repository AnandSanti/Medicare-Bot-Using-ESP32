# Medicare Bot - Quick Start Guide

## Overview

Medicare Bot is an ESP32-based health monitoring system that reads heart rate, blood oxygen (SpO2), and body temperature, then publishes the data securely to an MQTT broker. It includes an alert system with LED and buzzer feedback, plus an emergency button.

## Required Libraries

Install these through the Arduino IDE (Sketch → Include Library → Manage Libraries):

- MAX30105 by SparkFun Electronics
- PubSubClient by Nick O'Leary
- ArduinoJson by Benoit Blanchon

Wire.h and WiFi.h are built into the ESP32 core, so no separate installation is needed.

## Board Setup

1. Go to Tools → Board → ESP32 → ESP32 Dev Module
2. Go to Tools → Port and select your COM port
3. Set Upload Speed to 921600 for faster uploads

## Configuration

Before uploading, open the sketch and update the following in the configuration section.

WiFi credentials:

```cpp
const char* WIFI_SSID = "your_wifi_name";
const char* WIFI_PASSWORD = "your_wifi_password";
```

MQTT broker settings, fill in your own broker details:

```cpp
const char* MQTT_BROKER = "your_broker_host";
const int MQTT_PORT = 8883;
const char* MQTT_USER = "your_username";
const char* MQTT_PASSWORD = "your_password";
const char* MQTT_TOPIC = "your/topic";
```

Alert thresholds can be adjusted if needed:

```cpp
const int HEART_RATE_HIGH = 120;
const int HEART_RATE_LOW = 50;
const float TEMP_HIGH = 38.5;
const float TEMP_LOW = 35.0;
const int SPO2_LOW = 92;
```

## Upload

1. Open the sketch in Arduino IDE
2. Click Upload
3. Wait for the "Hard resetting via RTS pin" message

## Monitoring

1. Open Tools → Serial Monitor
2. Set the baud rate to 115200
3. Press the ESP32 reset button
4. Watch for the connection and sensor messages

## Expected Serial Output

On a successful boot you should see the I2C bus initialize, GPIO pins configure, the MAX30102 sensor get detected, WiFi connect with an IP address, and the MQTT broker connect. After that, sensor readings will print roughly once per second and data will publish every five seconds.

A typical reading block looks like this:

```
Temperature:  37.25 C
Heart Rate:   72 BPM
SpO2:         98 %
```

## Testing the Sensors

For heart rate and SpO2, place a fingertip gently on the MAX30102 sensor so both LEDs are covered. Keep the finger still for five to ten seconds while the algorithm gathers samples. The temperature reading comes from the thermistor and should settle near body temperature when in contact with skin.

## Testing Alerts

To test the alert system, raise your heart rate above the threshold through light exercise, or warm the thermistor above the temperature threshold. When any value crosses a threshold, the buzzer beeps twice and the LED blinks.

## Testing the Emergency Button

Press the emergency button. The buzzer sounds continuously for two seconds and the LED turns on, and the emergency flag is set in the published data.

## Troubleshooting

If the MAX30102 is not detected, check the I2C wiring on GPIO 21 (SDA) and GPIO 22 (SCL), confirm the sensor has 3.3V power, and make sure only one MAX30105 library is installed.

If WiFi fails to connect, verify the network name and password, and confirm the ESP32 is within range of the router.

If the MQTT broker will not connect, double-check the broker host, port, username, and password, and confirm the device has internet access.

If sensor readings show no valid value, keep a fingertip on the MAX30102 for a longer period and hold it still.

## Next Steps

Once the device is publishing successfully, you can subscribe to the MQTT topic from a dashboard, a mobile app, or a custom script to visualize and store the health data.
