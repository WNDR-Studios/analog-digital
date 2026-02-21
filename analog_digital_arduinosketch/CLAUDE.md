# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Arduino sketch for an LED matrix art installation running on Adafruit MatrixPortal ESP32-S3. Drives nine chained 64-wide HUB75 panels (576x32 pixels, rotated 90° so the long axis is vertical). Two display modes selected by a hardware switch on pin A1 (LOW = analog, HIGH = digital; internal pullup enabled).

## Build Commands

Open `analog_digital/analog_digital.ino` in Arduino IDE with ESP32-S3 board support. Required libraries (install via Library Manager):
- Adafruit Protomatter
- Adafruit GFX Library
- elapsedMillis

## Architecture

**Main sketch** (`analog_digital.ino`): Initializes matrix hardware, runs 60 FPS frame loop. Reads pin A1 each loop iteration to select mode (LOW = analog, HIGH = digital, internal pullup enabled).

**Analog mode** (`analog.h`/`analog.cpp`): Scrolling colored waveforms. Six generator functions (sin, triangle, saw, shark-fin, square, noise) convert Y position to X pixel coordinate. Up to 4 concurrent waves with independent direction, speed, frequency, and color from a 12-color palette.

**Digital mode** (`digital.h`/`digital.cpp`): Matrix-style binary rain with animated diamond-shaped eyes. Eyes blink, look around with iris/pupil, have eyelashes, and emit expanding ripple rings.

Both modes follow the same pattern: `initX()` called once from setup, `drawX()` called each frame to render and advance animations.

## Git Commits

Do not include Co-Authored-By lines in commit messages.
