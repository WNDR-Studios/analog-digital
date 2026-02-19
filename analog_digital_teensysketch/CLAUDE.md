# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Teensy microcontroller sketch for Analog Digital. Reads 5 digital sensor inputs and outputs USB MIDI notes and CC messages to trigger lighting cues in QLC+ and audio in a DAW.

## Build Commands

Open `analog_digital_teensy/analog_digital_teensy.ino` in Arduino IDE (or Teensyduino) with Teensy board support selected. Uses the built-in `elapsedMillis` type from Teensyduino — no additional library installs required.

## Architecture

**Main sketch** (`analog_digital_teensy.ino`): Declares 5 Sensor instances (input pins 0-4, output pins 6-10, MIDI channels 1-5). Polls all sensors each loop iteration.

**Sensor class** (`Sensor.h`/`Sensor.cpp`): Each sensor monitors one digital input pin. When the pin transitions from LOW to HIGH, it sends a USB MIDI note-on (note 60, velocity 100) on its assigned channel plus a CC message on channel 16 (for QLC+ lighting). After a 2-second duration the note-off and corresponding CC are sent automatically. Notes are ignored while one is already active.

Key conventions:
- MIDI notes go on per-sensor channels (1-5); CC messages all go on channel 16
- CC numbers are derived from the channel: on = channel * 2, off = channel * 2 + 1
- `usbMIDI` is the Teensy USB MIDI interface (no external MIDI library needed)

## Git Commits

Do not include Co-Authored-By lines in commit messages.
