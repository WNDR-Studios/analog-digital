/**
 * analog.h
 *
 * Header for the analog waveform visualization mode.
 * Defines the available waveform types, the Wave struct that tracks each
 * active waveform's state, and the public API used by the main sketch.
 */

#ifndef ANALOG_H
#define ANALOG_H

#include <Adafruit_Protomatter.h>

/** Total number of distinct waveform shapes available. */
const int numWaveforms = 6;

/**
 * Waveforms
 *
 * Enum of all supported waveform shapes. Each maps to a dedicated generator
 * function in analog.cpp that converts a Y position to an X pixel coordinate.
 */
enum Waveforms {
  SIN_WAVE,    // Classic sine wave
  TRI_WAVE,    // Triangle wave (linear ramps via arcsin of sine)
  SAW_WAVE,    // Sawtooth wave (linear ramp with snap-back)
  SHARK_WAVE,  // Shark-fin wave (fast rise, slow cosine fall)
  SQR_WAVE,    // Square wave (binary high/low)
  NOISE_WAVE   // Smooth random noise (cosine-interpolated random control points)
};

/**
 * Wave
 *
 * Holds all per-waveform state: current draw position, visual properties,
 * and activity flag. Waves scroll top-to-bottom and deactivate once their
 * trailing edge passes the bottom of the screen.
 */
struct Wave {
  int curY;            // Leading edge Y position (advances each frame)
  int curClearY;       // Trailing edge Y position for erasing old pixels
  int color;           // 16-bit RGB565 color
  int length;          // Visible length in pixels between leading and trailing edges
  int speed;           // Pixels the leading edge advances per frame
  float radianOffset;  // Controls waveform frequency — higher = more cycles on screen
  Waveforms waveform;  // Which shape generator to use
  bool active;         // false once the wave has fully scrolled off-screen
};

/** Maximum number of concurrent waveforms on screen. */
const int numWaves = 5;

extern Waveforms waveformArray[numWaveforms];
extern Wave waves[numWaves];

/**
 * initWaveform()
 *
 * Creates and returns a new Wave with the given parameters and a random color.
 *
 * @param radianOffset  Frequency multiplier (multiplied by PI internally)
 * @param length        Visible tail length in pixels
 * @param speed         Scroll speed in pixels per frame
 * @param waveform      Which waveform shape to use
 * @param matrix        Reference to the LED matrix (used for color generation)
 * @return              Fully initialized Wave struct
 */
Wave initWaveform(int radianOffset, int length, int speed, Waveforms waveform, Adafruit_Protomatter &matrix);

/**
 * drawAnalog()
 *
 * Renders one frame of the analog waveform scene. Clears the screen, draws
 * all active waveforms, deactivates any that have scrolled off, and spawns
 * new ones to keep the display populated.
 *
 * @param matrix  Reference to the LED matrix
 */
void drawAnalog(Adafruit_Protomatter &matrix);

#endif
