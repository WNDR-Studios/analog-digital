/**
 * analog_digital.ino
 *
 * Main sketch for the Analog/Digital LED matrix art installation.
 * Drives a chain of nine 64-pixel-wide HUB75 LED panels via an
 * Adafruit MatrixPortal ESP32-S3. The display alternates between two
 * visual modes:
 *   - Analog mode: scrolling colored waveforms (sine, triangle, saw, etc.)
 *   - Digital mode: a "Matrix"-style rain of binary digits with animated
 *     blinking eyes, eyelashes, and expanding ripple effects
 *
 * Sending a space character (' ') over the serial console toggles
 * between the two modes at runtime.
 */

#include <Adafruit_Protomatter.h>
#include <elapsedMillis.h>
#include <math.h>
#include "analog.h"
#include "digital.h"

// --- HUB75 wiring for MatrixPortal ESP32-S3 ---
uint8_t rgbPins[]  = {42, 41, 40, 38, 39, 37};
uint8_t addrPins[] = {45, 36, 48, 35, 21};
uint8_t clockPin   = 2;
uint8_t latchPin   = 47;
uint8_t oePin      = 14;

// Nine 64-wide panels chained together = 576 pixels wide.
// After setRotation(1) the long axis becomes the Y axis (height).
Adafruit_Protomatter matrix(
  64 * 9,          // Width of matrix (or matrix chain) in pixels
  4,           // Bit depth, 1-6
  1, rgbPins,  // # of matrix chains, array of 6 RGB pins for each
  4, addrPins, // # of address pins (height is inferred), array of pins
  clockPin, latchPin, oePin, // Other matrix control pins
  true // Double Buffered
);

// --- Frame rate limiter ---
elapsedMicros timeSinceFrame = 0;
elapsedMicros frameTimer = 0;
const uint8_t maxFPS = 60;
const unsigned long microsPerFrame = 1000000 / maxFPS;

// true = waveform mode, false = binary rain / eyes mode
bool analogMode = true;

// Auto-switch timer
elapsedMillis timeSinceSwitch = 0;
const unsigned long switchInterval = 30000; // 30 seconds


/**
 * setup()
 *
 * Arduino entry point. Initializes serial, the LED matrix hardware,
 * pre-computes the vertical spacing for the scrolling digit characters,
 * and seeds the first waveform for analog mode.
 */
void setup(void) {
  Serial.begin(115200);

  ProtomatterStatus status = matrix.begin();
  Serial.print("Protomatter begin() status: ");
  Serial.println((int)status);
  if(status != PROTOMATTER_OK) {
    for(;;); // Halt if the matrix failed to initialize
  }

  // Rotate so the long 576 px chain becomes vertical (Y axis)
  matrix.setRotation(1);
  matrix.fillScreen(0);
  matrix.show();

  initDigital(matrix);
  initAnalog(matrix);

  Serial.println("Setup complete");
}


/**
 * loop()
 *
 * Arduino main loop. Enforces a 60 FPS cap, checks serial for a mode-toggle
 * command, then draws either the analog waveform scene or the digital
 * eyes / binary rain scene.
 */
void loop() {
  // Auto-switch between modes every 30 seconds
  if (timeSinceSwitch >= switchInterval) {
    timeSinceSwitch = 0;
    analogMode = !analogMode;
  }

  // Frame rate limiter — skip until enough time has elapsed
  if (timeSinceFrame < microsPerFrame) return;
  timeSinceFrame = 0;

  if (analogMode) {
    drawAnalog(matrix);
  } else {
    drawDigital(matrix);
  }
}
