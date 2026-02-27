# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Teensy microcontroller sketch for Analog Digital. Reads 5 digital sensor inputs and outputs USB MIDI notes and CC messages to trigger lighting cues in QLC+ and audio in a DAW.

## Build Commands

Open `analog_digital_teensy/analog_digital_teensy.ino` in Arduino IDE (or Teensyduino) with Teensy board support selected. Uses the built-in `elapsedMillis` type from Teensyduino — no additional library installs required.

## Architecture

**Main sketch** (`analog_digital_teensy.ino`): Declares 5 Sensor instances (input pins 0-4, output pins 33-37). MIDI channels are assigned in pairs per sensor: sensor 1 = ch 1/2, sensor 2 = ch 3/4, ..., sensor 5 = ch 9/10. Polls all sensors each loop iteration.

**Sensor class** (`Sensor.h`/`Sensor.cpp`): Each sensor operates in two modes:
- **Analog mode** (default): A sustained MIDI note-on is sent on `midiChannelAnalog` immediately after `init()` and held indefinitely.
- **Digital mode** (triggered): When the input pin transitions LOW-to-HIGH (after 250ms debounce), the analog note stops and a note-on is sent on `midiChannelDigital` plus a CC message on channel 16. After 5 seconds the digital note-off fires, the CC off is sent, and analog mode resumes.

Public methods: `playAnalog()`, `stopAnalog()`, `playDigital()`, `stopDigital()`, `init()`, `check()`.

Key conventions:
- Each sensor has two MIDI channels: odd for analog, even for digital (ch 1/2, 3/4, 5/6, 7/8, 9/10)
- CC messages go on channel 16; CC numbers derived from the analog channel: on = analogChannel * 2, off = analogChannel * 2 + 1
- `usbMIDI` is the Teensy USB MIDI interface (no external MIDI library needed)

## Git Commits

Do not include Co-Authored-By lines in commit messages.
