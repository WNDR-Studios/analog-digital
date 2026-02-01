/**
 * analog.cpp
 *
 * Implements the analog waveform visualization mode. Multiple colored
 * waveforms scroll top-to-bottom across the LED matrix, each using one of
 * six generator functions (sine, triangle, sawtooth, shark-fin, square,
 * or smooth noise). Waveforms are drawn as a moving window of pixels;
 * once the trailing edge passes the bottom of the screen the wave is
 * deactivated and its slot can be reused.
 *
 * All waveform generators share the same interface: given a vertical pixel
 * position Y and a frequency parameter (radianOffset), they return the
 * horizontal pixel position X where the wave should be drawn on that row.
 * The Y position is mapped into radians so the waveform repeats smoothly
 * over the screen height, and the resulting -1..1 amplitude is mapped back
 * onto the screen width.
 */

#include "analog.h"
#include <math.h>

/** Lookup table so we can pick a random waveform by index. */
Waveforms waveformArray[numWaveforms] = {SIN_WAVE, TRI_WAVE, SAW_WAVE, SHARK_WAVE, SQR_WAVE, NOISE_WAVE};
Wave waves[numWaves];

/* ------------------------------------------------------------------ */
/*  Waveform generator functions                                      */
/*  Each converts a Y pixel position into an X pixel position using   */
/*  a different mathematical shape. radianOffset controls how many     */
/*  cycles fit on the screen (higher = more oscillations).            */
/* ------------------------------------------------------------------ */

/**
 * sinWave()
 *
 * Classic sine wave. Maps Y into radians, takes sin(), and scales the
 * result from -1..1 back to 0..screenWidth.
 */
static int sinWave(int y, float radianOffset, Adafruit_Protomatter &matrix) {
    float yMapped = ((float)y / (float)matrix.height()) * radianOffset;
    float sinY = sin(yMapped);
    int x = round(((sinY + 1)/2) * (float(matrix.width()) - 1));
    return x;
}

/**
 * triWave()
 *
 * Triangle wave. Uses the identity asin(sin(x)) to convert the sine
 * curve into linear ramps, producing a zig-zag pattern.
 */
static int triWave(int y, float radianOffset, Adafruit_Protomatter &matrix) {
    float yMapped = ((float)y / (float)matrix.height()) * radianOffset;
    float sinY = sin(yMapped);
    float arcY = asin(sinY);           // Folds sine into linear ramps
    float triY = (2 * arcY) / PI;      // Normalize to -1..1
    int x = round(((triY + 1)/2) * (float(matrix.width()) - 1));
    return x;
}

/**
 * sawWave()
 *
 * Sawtooth wave. Produces a linear ramp from -1 to 1 that snaps back
 * at the end of each period.
 */
static int sawWave(int y, float radianOffset, Adafruit_Protomatter &matrix) {
    float yMapped = ((float)y / (float)matrix.height()) * radianOffset;
    // Linear ramp -1 to 1 within each 2*PI period, then snaps back
    float sawY = 2.0 * (yMapped / (2.0 * PI) - floor(yMapped / (2.0 * PI) + 0.5));
    int x = round(((sawY + 1) / 2) * (float(matrix.width()) - 1));
    return x;
}

/**
 * sharkWave()
 *
 * Shark-fin wave. Produces an asymmetric shape: a fast linear rise
 * (18% of the period) followed by a slow, rounded cosine fall (82%).
 * Resembles a dorsal fin or a capacitor charge/discharge curve.
 */
static int sharkWave(int y, float radianOffset, Adafruit_Protomatter &matrix) {
    float yMapped = ((float)y / (float)matrix.height()) * radianOffset;
    float phase = fmod(yMapped, 2.0 * PI) / (2.0 * PI); // Normalize to 0..1 phase
    if (phase < 0) phase += 1.0;
    float sharkY;
    if (phase < 0.18) {
      sharkY = phase / 0.18;            // Fast linear rise from 0 to 1
    } else {
      float fallPhase = (phase - 0.18) / 0.82;
      sharkY = cos(fallPhase * PI) * 0.5 + 0.5; // Smooth cosine fall from 1 to 0
    }
    sharkY = sharkY * 2.0 - 1.0;        // Remap from 0..1 to -1..1
    int x = round(((sharkY + 1) / 2) * (float(matrix.width()) - 1));
    return x;
}

/**
 * sqrWave()
 *
 * Square wave. Outputs full-left or full-right based on the sign of
 * sin() at the current phase — producing sharp horizontal transitions.
 */
static int sqrWave(int y, float radianOffset, Adafruit_Protomatter &matrix) {
    float yMapped = ((float)y / (float)matrix.height()) * radianOffset;
    float sqrY = sin(yMapped) >= 0 ? 1.0 : -1.0;
    int x = round(((sqrY + 1) / 2) * (float(matrix.width()) - 1));
    return x;
}

/**
 * noiseHash()
 *
 * Deterministic hash for noise wave control points. Given a segment index
 * and radianOffset, returns a reproducible pseudo-random value. This lets
 * the noise wave be redrawn identically each frame without storing state.
 * Uses Knuth's multiplicative hash constants for good bit mixing.
 */
static unsigned long noiseHash(int segment, float radianOffset) {
    unsigned long seed = (unsigned long)(segment + 1) * 2654435761UL;
    seed ^= (unsigned long)(radianOffset * 100) * 2246822519UL;
    seed ^= seed >> 16;
    seed *= 0x45d9f3bUL;
    seed ^= seed >> 16;
    return seed;
}

/**
 * noiseWave()
 *
 * Smooth random waveform. Divides the screen into segments, places a
 * deterministic random X control point at each segment boundary, and
 * uses cosine interpolation between them for a smooth, organic look.
 */
static int noiseWave(int y, float radianOffset, Adafruit_Protomatter &matrix) {
    // Segment length in pixels, derived from the frequency parameter
    float period = (2.0 * PI * matrix.height()) / radianOffset;
    if (period < 2.0) period = 2.0;

    int segment = (int)floor((float)y / period);
    float t = ((float)y - segment * period) / period; // 0..1 within this segment
    float smooth = (1.0 - cos(t * PI)) / 2.0;         // Cosine interpolation (ease in/out)

    // Deterministic random X at each segment boundary
    int x0 = noiseHash(segment, radianOffset) % matrix.width();
    int x1 = noiseHash(segment + 1, radianOffset) % matrix.width();

    int x = x0 + (int)(smooth * (float)(x1 - x0));
    return x;
}

/**
 * initWaveform()
 *
 * Creates a new Wave starting at the top of the screen with a random color.
 * The radianOffset parameter is multiplied by PI so callers can pass simple
 * integers (e.g. 10 becomes ~31.4 radians across the screen height).
 */
Wave initWaveform(int radianOffset, int length, int speed, Waveforms waveform, Adafruit_Protomatter &matrix) {
  Wave wave;
  wave.curY = 0;
  wave.length = length;
  wave.speed = speed;
  wave.radianOffset = radianOffset * PI;
  wave.color = matrix.color565(random(255), random(255), random(255));
  wave.waveform = waveform;
  wave.active = true;
  return wave;
}

/**
 * drawWaveform()
 *
 * Renders a single waveform for the current frame. Draws pixels from the
 * trailing edge (curY - length) to the leading edge (curY), calling the
 * appropriate generator function for each row to determine X position.
 *
 * Special-case handling for sawtooth and square waves: when the X value
 * jumps abruptly between consecutive rows (a snap-back or high/low
 * transition), a full-width horizontal line is drawn to connect them
 * visually, mimicking how these waveforms appear on a real oscilloscope.
 */
static void drawWaveform(struct Wave &wave, Adafruit_Protomatter &matrix) {
  float yMapped;
  float sinY;
  int x;
  float matrixHeight = matrix.height();
  float matrixWidth = matrix.width();

  // Clamp the visible range to screen bounds
  int startingY = wave.curY - wave.length;
  if (startingY < 0) startingY = 0;
  int endingY = wave.curY;
  if (endingY > matrix.height()) endingY = matrix.height();

  for (int y = startingY; y <= endingY; y++) {
    // Dispatch to the correct waveform generator
    switch (wave.waveform) {
      case SIN_WAVE:
        x = sinWave(y, wave.radianOffset, matrix);
        break;
      case TRI_WAVE:
        x = triWave(y, wave.radianOffset, matrix);
        break;
      case SAW_WAVE:
        x = sawWave(y, wave.radianOffset, matrix);
        break;
      case SHARK_WAVE:
        x = sharkWave(y, wave.radianOffset, matrix);
        break;
      case SQR_WAVE:
        x = sqrWave(y, wave.radianOffset, matrix);
        break;
      case NOISE_WAVE:
        x = noiseWave(y, wave.radianOffset, matrix);
        break;
    }

    matrix.drawPixel(x, y, wave.color);

    // Sawtooth snap-back: if the next row's X jumps more than half the
    // screen width to the left, it's a wrap-around — draw a horizontal
    // line across the full width to connect the two sides.
    if (wave.waveform == SAW_WAVE && y < endingY) {
      int xNext = sawWave(y + 1, wave.radianOffset, matrix);
      if (xNext < x - (matrix.width() / 2)) {
        matrix.drawFastHLine(0, y, matrix.width(), wave.color);
      }
    }

    // Square wave transition: when the output flips between high and low,
    // draw a horizontal line to create the vertical edge of the square.
    if (wave.waveform == SQR_WAVE && y < endingY) {
      int xNext = sqrWave(y + 1, wave.radianOffset, matrix);
      if (xNext != x) {
        matrix.drawFastHLine(0, y, matrix.width(), wave.color);
      }
    }
  }

  wave.curY = wave.curY + wave.speed;
}

/**
 * spawnWave()
 *
 * Finds the first inactive wave slot and initializes it with randomized
 * parameters (frequency, length, speed, and waveform type).
 */
static void spawnWave(Adafruit_Protomatter &matrix) {
  for (int i = 0; i < numWaves; i++) {
    if (!waves[i].active) {
      waves[i] = initWaveform(random(2, 40), random(40, matrix.height()), random(1, 6), waveformArray[random(numWaveforms)], matrix);
      return;
    }
  }
}

/**
 * drawAnalog()
 *
 * Main entry point for the analog visualization mode, called once per frame.
 * Clears the screen, draws all active waveforms, retires any that have
 * scrolled off, and ensures at least one wave is always visible. Additional
 * waves spawn randomly up to a maximum of 4 concurrent.
 */
void drawAnalog(Adafruit_Protomatter &matrix) {
  matrix.fillScreen(0);

  int activeCount = 0;
  for (int i = 0; i < numWaves; i++) {
    if (!waves[i].active) continue;

    drawWaveform(waves[i], matrix);

    // A wave is "off-screen" when its trailing edge has passed the bottom
    int startingY = waves[i].curY - waves[i].length;
    if (startingY > matrix.height()) {
      waves[i].active = false;
    } else {
      activeCount++;
    }
  }

  // Always keep at least 1 wave on screen
  if (activeCount < 1) {
    spawnWave(matrix);
  }

  // ~0.8% chance each frame to spawn another wave (up to 4 concurrent)
  if (activeCount < 4 && random(120) == 0) {
    spawnWave(matrix);
  }

  matrix.show();
}
