# Medicare Bot - Hardware Setup

## Components

The build uses an ESP32 development module as the controller, a MAX30102 pulse oximeter for heart rate and SpO2, an NTC 10K thermistor for body temperature, an LED for status indication, a buzzer for audible alerts, and a push button for the emergency function. You will also need a 10K resistor for the thermistor voltage divider, a 10K resistor for the button if you prefer an external pull-up, and a current-limiting resistor for the LED.

## Pin Assignments

The MAX30102 connects over I2C, with SDA on GPIO 21 and SCL on GPIO 22, powered from 3.3V and ground. The thermistor connects to GPIO 34, which is an analog-capable input. The LED connects to GPIO 25 through a current-limiting resistor. The buzzer connects to GPIO 26. The emergency button connects to GPIO 27 and uses the internal pull-up, so it reads low when pressed.

## MAX30102 Wiring

Connect the sensor VIN pin to the ESP32 3.3V pin, and the sensor ground to the ESP32 ground. Connect SCL to GPIO 22 and SDA to GPIO 21. The interrupt pin is not required for this project and can be left unconnected.

## Thermistor Wiring

The thermistor is placed in a voltage divider with a 10K resistor. Connect 3.3V to one end of the 10K resistor. Connect the other end of the resistor to both GPIO 34 and one leg of the thermistor. Connect the remaining leg of the thermistor to ground. As temperature changes, the resistance of the thermistor changes, which changes the voltage at GPIO 34, and the firmware converts that voltage to a temperature.

## LED Wiring

Connect GPIO 25 to the longer leg of the LED through a current-limiting resistor, and connect the shorter leg of the LED to ground.

## Buzzer Wiring

Connect GPIO 26 to the positive terminal of the buzzer and the negative terminal to ground. An active buzzer is simplest since it produces a tone when driven high.

## Emergency Button Wiring

Connect one side of the button to GPIO 27 and the other side to ground. Because the firmware enables the internal pull-up on GPIO 27, the pin normally reads high and drops to low when the button is pressed, so no external resistor is required.

## Power

During normal operation with WiFi active and the sensors running, the device draws in the region of 150 milliamps, so power it from a stable USB source or a supply that can comfortably provide that current.

## Notes on ADC Usage

GPIO 34 is an input-only pin and is a good choice for the thermistor because it does not conflict with the pins WiFi uses internally. Keep sensor wiring reasonably short to reduce noise on the analog reading.

## Assembly Order

A practical order is to wire the MAX30102 first and confirm it is detected, then add the thermistor and confirm a sensible temperature reading, then add the LED and buzzer and confirm they respond during an alert, and finally add the emergency button. Verifying each stage before moving on makes it much easier to find a wiring mistake.
