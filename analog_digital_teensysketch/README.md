# Analog Digital - Teensy MIDI Controller

Teensy sketch that reads 5 digital sensors and outputs USB MIDI for Analog Digital. Sensor triggers drive lighting cues in QLC+ and audio in a DAW.

## How It Works

Each sensor monitors a digital input pin for a rising edge. When triggered it sends a MIDI note-on and CC message, drives an output pin HIGH for LED displays, and automatically turns everything off after a timed duration.

- **Notes** are sent on per-sensor MIDI channels (1-5)
- **CC messages** are sent on channel 16 for QLC+ lighting control

## Building

1. Install [Arduino IDE](https://www.arduino.cc/en/software) with [Teensyduino](https://www.pjrc.com/teensy/teensyduino.html)
2. Open `analog_digital_teensy/analog_digital_teensy.ino`
3. Select your Teensy board and set USB Type to **MIDI**
4. Upload

No additional libraries required.
