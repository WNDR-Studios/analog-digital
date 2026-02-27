# Analog Digital - Teensy MIDI Controller

Teensy sketch that reads 5 digital sensors and outputs USB MIDI for Analog Digital. Sensor triggers drive lighting cues in QLC+ and audio in a DAW.

## How It Works

Each sensor operates in two modes:

**Analog mode** (default): On startup a sustained MIDI note-on is sent on the sensor's analog channel and held until a trigger fires.

**Digital mode** (triggered): When the input pin goes HIGH and stays HIGH for 250ms, the analog note stops and a MIDI note-on plus CC message fires on the digital channel. After 5 seconds the digital note turns off and analog resumes automatically.

MIDI channels are assigned in pairs per sensor: sensor 1 = ch 1 (analog) / ch 2 (digital), sensor 2 = ch 3/4, sensor 3 = ch 5/6, sensor 4 = ch 7/8, sensor 5 = ch 9/10.

- **CC messages** are sent on channel 16 for QLC+ lighting control

## Building

1. Install [Arduino IDE](https://www.arduino.cc/en/software) with [Teensyduino](https://www.pjrc.com/teensy/teensyduino.html)
2. Open `analog_digital_teensy/analog_digital_teensy.ino`
3. Select your Teensy board and set USB Type to **MIDI**
4. Upload

No additional libraries required.
