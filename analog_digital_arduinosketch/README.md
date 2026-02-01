# Analog/Digital

An LED matrix art installation for the Adafruit MatrixPortal ESP32-S3, driving a chain of nine 64-pixel-wide HUB75 panels. The display alternates between two visual modes toggled via serial input.

## Hardware

- Adafruit MatrixPortal ESP32-S3
- 9x chained 64-wide HUB75 RGB LED matrix panels (576 x 32 physical pixels, rotated 90 degrees so the long axis runs vertically)

## Display Modes

### Analog Mode

Colored waveforms scroll across the screen. Each wave randomly scrolls either downward or upward, so they frequently cross and overlap each other. Up to four waveforms can be active simultaneously, each randomly assigned one of six shapes:

| Shape | Description |
|-------|-------------|
| Sine | Classic smooth oscillation |
| Triangle | Linear zig-zag ramps (derived via arcsin of sine) |
| Sawtooth | Linear ramp with a snap-back; horizontal lines are drawn at each wrap-around |
| Shark-fin | Asymmetric shape: fast linear rise, slow cosine fall |
| Square | Binary high/low with horizontal transition lines connecting the jumps |
| Noise | Smooth random curves using cosine-interpolated control points |

Each waveform is assigned a color from a curated palette of 12 bold, saturated, and visually distinct hues (red, orange, gold, green, teal, azure, blue, purple, magenta, hot pink, cyan, and olive yellow). Frequency, tail length, and scroll speed are randomized. At least one waveform is always on screen, and additional ones spawn randomly.

### Digital Mode

A "Matrix"-style scene with a dark-red pulsing background and a column of white binary digits (`0` and `1`) scrolling downward.

Overlaid on the rain are animated diamond-shaped eyes that:

- **Open and close** with a smooth width animation
- **Blink** 1-4 times before closing for good, emitting expanding black ripple rings on each blink
- **Look around** with a soft-red iris and dark-red pupil that drift to random positions inside the eye
- **Sprout eyelashes** that fan outward from evenly-spaced points along both lids, angling up near the top and down near the bottom

At least 2 eyes are always visible, with up to 5 active at once. New eyes are placed using rejection sampling to prevent vertical overlap.

## Controls

Send a space character (`' '`) over the serial console (115200 baud) to toggle between analog and digital modes.

## Dependencies

- [Adafruit Protomatter](https://github.com/adafruit/Adafruit_Protomatter) -- HUB75 matrix driver
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library) -- Graphics primitives (dependency of Protomatter)
- [elapsedMillis](https://github.com/pfeerick/elapsedMillis) -- Non-blocking timing

## Building

Open `analog_digital/analog_digital.ino` in the Arduino IDE (or Arduino CLI) with ESP32-S3 board support installed. Install the libraries listed above via the Library Manager, then compile and upload.
