# Medicare Bot

Medicare Bot is an ESP32-based health monitoring system. It measures heart rate, blood oxygen saturation, and body temperature, evaluates the readings against configurable thresholds, and publishes the data to an MQTT broker over a secure connection. Local feedback is provided through an LED and a buzzer, and an emergency button lets the wearer signal for help.

## Features

The system reads heart rate and SpO2 from a MAX30102 pulse oximeter and body temperature from an NTC thermistor. It checks each reading against high and low thresholds and raises an alert when any value goes out of range. Alerts are signalled locally with a buzzer and LED, and an emergency button triggers a distinct continuous alert. All readings are published as JSON to an MQTT broker every few seconds for remote monitoring.

## Hardware

The build uses an ESP32 development module, a MAX30102 sensor on the I2C bus, an NTC 10K thermistor on an analog input, an LED and buzzer for feedback, and a push button for the emergency function. Full wiring details are in the hardware setup guide.

## Getting Started

Install the required Arduino libraries, update the WiFi and MQTT settings in the sketch, wire the components as described in the hardware guide, and upload the firmware. The quick start guide walks through each step and describes what to expect in the serial monitor.

## Documentation

The quick start guide covers installation, configuration, uploading, and first-run testing. The hardware setup guide covers components and wiring. The MQTT payload guide describes the published data format and how to subscribe to it. The troubleshooting guide lists common problems and their solutions.

## Configuration

WiFi credentials, MQTT broker details, and alert thresholds are all set in the configuration section at the top of the sketch. Broker credentials should be kept private and never committed to a public repository.

## Required Libraries

The project depends on the MAX30105 library by SparkFun Electronics, the PubSubClient library by Nick O'Leary, and the ArduinoJson library by Benoit Blanchon. The Wire and WiFi libraries are part of the ESP32 core.

## License

This project is released under the MIT License.
