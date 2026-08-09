# Medicare Bot - Troubleshooting

## Compilation Issues

If the compiler reports that WiFiClientSecure does not name a type, make sure the sketch includes the WiFiClientSecure header near the top, and that the ESP32 board package is selected rather than a generic Arduino board.

If the compiler reports that I2C_400kHz was not declared, use the numeric value 400000 in place of that constant when starting the sensor and the I2C bus, since that named constant is not available in the ESP32 core.

If multiple libraries are found for MAX30105.h, keep the SparkFun MAX3010x library and remove any other library that also provides that header, then restart the Arduino IDE so it picks up the change.

If a header such as PubSubClient.h, MAX30105.h, or ArduinoJson.h cannot be found, open the library manager and install the corresponding library, then compile again.

## Upload Issues

If the upload fails to start, confirm the correct COM port is selected and that the board is set to ESP32 Dev Module. On some boards you may need to hold the boot button while the upload begins. Enabling erase all flash before upload can also clear a stuck state.

If the serial output shows unreadable characters, confirm the serial monitor baud rate is set to 115200 and reset the board.

## Sensor Not Detected

If the MAX30102 is not found, check that SDA is on GPIO 21 and SCL is on GPIO 22, confirm the sensor is powered from 3.3V, and verify the ground connection. Loose jumper wires are a common cause. Running an I2C scanner sketch can confirm whether the sensor is visible on the bus.

## No Valid Heart Rate or SpO2

The MAX30102 needs a still fingertip covering both light sources for several seconds before the algorithm can produce a reading. Pressing too hard restricts blood flow, while a loose contact lets in ambient light, so aim for gentle, steady contact. Readings of zero simply mean no valid measurement was available yet.

## Temperature Reading Looks Wrong

If the temperature is far from expected, confirm the voltage divider is wired correctly with the 10K resistor between 3.3V and GPIO 34, and the thermistor between GPIO 34 and ground. If the value is consistently offset, the thermistor B coefficient or nominal resistance in the code may need to be matched to your specific thermistor.

## WiFi Will Not Connect

Check that the network name and password are correct, remembering they are case sensitive. Confirm the router is within range and broadcasting on a band the ESP32 supports. Moving the device closer to the router during initial testing helps rule out signal problems.

## MQTT Will Not Connect

A connection failure usually points to an incorrect broker host, wrong port, or bad credentials. Confirm each value matches your broker exactly, and confirm the device has working internet access. If the broker requires TLS, make sure the secure client is used and the correct secure port is set.

## LED or Buzzer Not Responding

Confirm the LED is on GPIO 25 and the buzzer is on GPIO 26, and that both share a common ground with the board. Testing the LED on its own during an alert is a quick way to confirm the output pin is working before suspecting the buzzer.

## Emergency Button Not Working

The button should connect GPIO 27 to ground. Because the internal pull-up is enabled, the pin reads high normally and low when pressed. If the button never registers, check that it is wired to the correct pin and that it actually closes the circuit when pressed.

## General Advice

The serial monitor is the most useful debugging tool, since the firmware prints clear status messages as it initializes each part of the system. Working through the output from top to bottom will usually point directly to the stage that is failing.
