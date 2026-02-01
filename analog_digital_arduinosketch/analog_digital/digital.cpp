/**
 * digital.cpp
 *
 * Implements the digital visualization mode: a dark-red background with
 * a scrolling column of binary digits ("Matrix" rain), overlaid with
 * animated diamond-shaped eyes that open, look around, blink, and close.
 * Each blink spawns expanding black ripple rings that carve through the
 * scene. Eyes have eyelid outlines, fanning eyelashes, a soft-red iris,
 * and a dark-red pupil.
 */

#include "digital.h"
#include <elapsedMillis.h>

int8_t charOffset;
DigitChar digitChars[digitCharCount];

/** Background red intensity — slowly drifts between 15 and 50 each frame. */
static uint8_t bgRedVal = 15;

/* ------------------------------------------------------------------ */
/*  Eye system constants                                              */
/* ------------------------------------------------------------------ */
static const int MAX_EYES = 8;           // Max concurrent eyes
static const int EYE_HALF_HEIGHT = 25;   // Vertical half-span of each eye (pixels)
static const int EYE_MIN_SPACING = 55;   // Minimum Y distance between eyes
static const int EYE_OPEN_SPEED = 2;     // Pixels per frame for open/close animation

/**
 * EyeState
 *
 * State machine for eye lifecycle. Each eye progresses through:
 *   INACTIVE -> OPENING -> OPEN -> (BLINKING_CLOSE <-> BLINKING_OPEN) -> CLOSING -> INACTIVE
 * The eye blinks a random number of times while open before closing for good.
 */
enum EyeState {
  EYE_INACTIVE,       // Slot is free for reuse
  EYE_OPENING,        // openAmount increasing from 0 to maxOpen
  EYE_OPEN,           // Fully open, counting down timer between blinks
  EYE_BLINKING_CLOSE, // Mid-blink, closing
  EYE_BLINKING_OPEN,  // Mid-blink, reopening
  EYE_CLOSING         // Final close before going inactive
};

/**
 * Eye
 *
 * Holds all state for a single animated eye: position, animation phase,
 * and iris tracking. The eye shape is a diamond (two V-lines meeting at
 * the top and bottom tips) whose horizontal half-width is controlled by
 * openAmount.
 */
struct Eye {
  int x, y;                              // Center position on screen
  EyeState state;                        // Current animation state
  int openAmount;                        // Current horizontal half-width (0 = closed, maxOpen = fully open)
  int maxOpen;                           // Maximum half-width when fully open
  int halfHeight;                        // Vertical half-span (top tip to center)
  int timer;                             // Countdown timer for current state (frames)
  int blinksLeft;                        // Remaining blinks before the eye closes for good
  int irisX, irisY;                      // Current iris offset from eye center
  int irisTargetX, irisTargetY;          // Target iris offset (iris drifts toward this)
  int lookTimer;                         // Frames until a new random look target is chosen
};

static Eye eyes[MAX_EYES];

/* ------------------------------------------------------------------ */
/*  Ripple system                                                     */
/*  Black ring effects that expand outward from an eye when it blinks */
/* ------------------------------------------------------------------ */

/** A single expanding ring drawn in black to "carve" through the scene. */
struct Ripple {
  int cx, cy;    // Center of the ring (set to the blinking eye's position)
  int radius;    // Current ring radius in pixels
  int speed;     // Expansion rate (pixels per frame)
  bool active;   // false once the ring has expanded past the screen
};
static const int MAX_RIPPLES = 12;
static Ripple ripples[MAX_RIPPLES];

/**
 * spawnRipples()
 *
 * Creates 1-3 new ripple rings centered on the given eye. Called each time
 * an eye blinks. Each ripple starts at the eye's halfHeight radius (just
 * outside the lid) and expands outward at a random speed.
 *
 * @param eye  The eye that just blinked
 */
static void spawnRipples(Eye &eye) {
  int count = random(1, 4);
  for (int c = 0; c < count; c++) {
    for (int i = 0; i < MAX_RIPPLES; i++) {
      if (!ripples[i].active) {
        ripples[i].cx = eye.x;
        ripples[i].cy = eye.y;
        ripples[i].radius = eye.halfHeight;
        ripples[i].speed = random(1, 4);
        ripples[i].active = true;
        break;
      }
    }
  }
}

/**
 * updateRipples()
 *
 * Advances all active ripples outward by their speed. Deactivates any
 * ripple whose radius exceeds the largest screen dimension.
 */
static void updateRipples(Adafruit_Protomatter &matrix) {
  int maxDim = max(matrix.width(), matrix.height());
  for (int i = 0; i < MAX_RIPPLES; i++) {
    if (ripples[i].active) {
      ripples[i].radius += ripples[i].speed;
      if (ripples[i].radius > maxDim) {
        ripples[i].active = false;
      }
    }
  }
}

/**
 * drawRipples()
 *
 * Renders all active ripples as two concentric black circles (the double
 * ring makes them more visible against the busy background).
 */
static void drawRipples(Adafruit_Protomatter &matrix) {
  for (int i = 0; i < MAX_RIPPLES; i++) {
    if (ripples[i].active) {
      matrix.drawCircle(ripples[i].cx, ripples[i].cy, ripples[i].radius, 0);
      if (ripples[i].radius > 0) {
        matrix.drawCircle(ripples[i].cx, ripples[i].cy, ripples[i].radius - 1, 0);
      }
    }
  }
}

/* ------------------------------------------------------------------ */
/*  Eyelash constants                                                 */
/* ------------------------------------------------------------------ */
static const int LASH_COUNT = 5;    // Number of lashes per lid side
static const int LASH_LENGTH = 5;   // Length of each lash line in pixels

/**
 * oneOrZero()
 *
 * Returns a random ASCII '0' or '1' for the scrolling binary rain.
 */
static char oneOrZero() {
  uint8_t num = random(2);
  char character = '0' + num;
  return character;
}

/**
 * initDigit()
 *
 * Creates a new DigitChar with a random '0'/'1' at the given Y offset.
 *
 * @param yOffset  Starting vertical position
 * @param color    16-bit RGB565 color
 * @return         Initialized DigitChar
 */
DigitChar initDigit(int yOffset, int color) {
  DigitChar digit;
  digit.yOffset = yOffset;
  digit.character = oneOrZero();
  digit.color = color;
  return digit;
}

/**
 * drawAlmondEye()
 *
 * Renders a single eye onto the matrix. The drawing order is:
 *   1. Black diamond fill (the eye interior)
 *   2. Lid outline (four lines forming the diamond border)
 *   3. Eyelashes (fanning outward from evenly-spaced points on each lid)
 *   4. Iris and pupil (filled circles at the iris offset position)
 *
 * The diamond shape is produced by a linear scanline fill: for each row
 * offset dy from the center, halfWidth = open * (hh - |dy|) / hh. This
 * gives straight edges tapering to points at dy = +/-hh.
 *
 * @param eye     The eye to draw
 * @param matrix  Reference to the LED matrix
 */
static void drawAlmondEye(Eye &eye, Adafruit_Protomatter &matrix) {
  int cx = eye.x;
  int cy = eye.y;
  int hh = eye.halfHeight;
  int open = eye.openAmount;
  uint16_t lidColor = matrix.color565(180, 180, 140);

  if (open <= 0) {
    // Fully closed: draw a thin vertical slit in lid color
    matrix.drawFastVLine(cx, cy - hh, hh * 2 + 1, lidColor);
    return;
  }

  // --- 1. Scanline fill the diamond interior with black ---
  // For each row, the half-width shrinks linearly from 'open' at center
  // (dy=0) to 0 at the tips (dy=+/-hh), producing straight diamond edges.
  for (int dy = -hh; dy <= hh; dy++) {
    int halfWidth = (int)((long)open * (hh - abs(dy)) / hh);
    if (halfWidth > 0) {
      matrix.drawFastHLine(cx - halfWidth, cy + dy, halfWidth * 2 + 1, 0);
    }
  }

  // --- 2. Lid outline ---
  // Four lines connecting: top tip -> left widest -> bottom tip -> right widest
  matrix.drawLine(cx, cy - hh, cx - open, cy, lidColor);   // Top to left
  matrix.drawLine(cx - open, cy, cx, cy + hh, lidColor);    // Left to bottom
  matrix.drawLine(cx, cy - hh, cx + open, cy, lidColor);    // Top to right
  matrix.drawLine(cx + open, cy, cx, cy + hh, lidColor);    // Right to bottom

  // --- 3. Eyelashes (only when eye is open enough to show the iris) ---
  // LASH_COUNT lashes are evenly spaced along each lid from dy=-(hh-4)
  // to dy=+(hh-4), avoiding the very tips. Each lash radiates outward
  // from the lid edge; the vertical 'fan' component is proportional to
  // the lash's dy, so top lashes angle upward, middle ones go straight
  // out, and bottom ones angle downward.
  if (open > 3) {
    for (int i = 0; i < LASH_COUNT; i++) {
      // Evenly distribute lash positions from -(hh-4) to +(hh-4)
      int dy = -(hh - 4) + i * (2 * (hh - 4)) / (LASH_COUNT - 1);
      int halfWidth = (int)((long)open * (hh - abs(dy)) / hh);
      // Fan angle: lashes near top fan upward, near bottom fan downward
      int fan = dy * LASH_LENGTH / hh;
      // Left lid lash: extends leftward from the left edge
      matrix.drawLine(cx - halfWidth, cy + dy,
                      cx - halfWidth - LASH_LENGTH, cy + dy + fan, lidColor);
      // Right lid lash: extends rightward from the right edge
      matrix.drawLine(cx + halfWidth, cy + dy,
                      cx + halfWidth + LASH_LENGTH, cy + dy + fan, lidColor);
    }
  }

  // --- 4. Iris and pupil ---
  // Drawn last so they appear on top of the black fill. The iris is a
  // soft-red filled circle, and the pupil is a smaller dark-red circle
  // centered within it.
  if (open > 3) {
    int irisR = open / 3;    // Iris radius scales with eye width
    int pupilR = open / 6;   // Pupil is half the iris size
    int ix = cx + eye.irisX;
    int iy = cy + eye.irisY;
    uint16_t irisColor = matrix.color565(180, 60, 60);
    matrix.fillCircle(ix, iy, irisR, irisColor);
    uint16_t pupilColor = matrix.color565(60, 10, 10);
    matrix.fillCircle(ix, iy, pupilR, pupilColor);
  }
}

/**
 * updateIris()
 *
 * Animates the iris "looking around" within the eye. Periodically picks a
 * new random target offset, then drifts toward it at 1 pixel per frame.
 * Horizontal range is wider (openAmount/3) than vertical (halfHeight/5)
 * to keep the iris within the diamond shape.
 *
 * @param eye  The eye whose iris to update
 */
static void updateIris(Eye &eye) {
  if (eye.openAmount <= 3) return;
  eye.lookTimer--;
  if (eye.lookTimer <= 0) {
    int maxH = eye.openAmount / 3;
    int maxV = eye.halfHeight / 5;
    eye.irisTargetX = random(-maxH, maxH + 1);
    eye.irisTargetY = random(-maxV, maxV + 1);
    eye.lookTimer = random(30, 120);
  }
  // Drift toward target 1 pixel per frame on each axis
  if (eye.irisX < eye.irisTargetX) eye.irisX++;
  else if (eye.irisX > eye.irisTargetX) eye.irisX--;
  if (eye.irisY < eye.irisTargetY) eye.irisY++;
  else if (eye.irisY > eye.irisTargetY) eye.irisY--;
}

/**
 * updateEye()
 *
 * Advances the eye's state machine by one frame. Handles opening/closing
 * animation, blink cycling, and delegates iris movement to updateIris().
 * Each state transition uses openAmount as the animation parameter:
 *   - OPENING / BLINKING_OPEN: openAmount increases by EYE_OPEN_SPEED
 *   - BLINKING_CLOSE / CLOSING: openAmount decreases by EYE_OPEN_SPEED
 *   - OPEN: counts down a random timer, then either blinks or closes
 *
 * @param eye  The eye to update
 */
static void updateEye(Eye &eye) {
  switch (eye.state) {
    case EYE_OPENING:
      eye.openAmount += EYE_OPEN_SPEED;
      if (eye.openAmount >= eye.maxOpen) {
        eye.openAmount = eye.maxOpen;
        eye.state = EYE_OPEN;
        eye.timer = random(60, 180); // Hold open for 1-3 seconds at 60fps
      }
      updateIris(eye);
      break;
    case EYE_OPEN:
      eye.timer--;
      if (eye.timer <= 0) {
        if (eye.blinksLeft > 0) {
          eye.state = EYE_BLINKING_CLOSE;
          eye.blinksLeft--;
          spawnRipples(eye);  // Each blink sends out ripple rings
        } else {
          eye.state = EYE_CLOSING;  // No blinks left — close for good
        }
      }
      updateIris(eye);
      break;
    case EYE_BLINKING_CLOSE:
      eye.openAmount -= EYE_OPEN_SPEED;
      if (eye.openAmount <= 0) {
        eye.openAmount = 0;
        eye.state = EYE_BLINKING_OPEN; // Immediately reopen
      }
      break;
    case EYE_BLINKING_OPEN:
      eye.openAmount += EYE_OPEN_SPEED;
      if (eye.openAmount >= eye.maxOpen) {
        eye.openAmount = eye.maxOpen;
        eye.state = EYE_OPEN;
        eye.timer = random(60, 180);
      }
      updateIris(eye);
      break;
    case EYE_CLOSING:
      eye.openAmount -= EYE_OPEN_SPEED;
      if (eye.openAmount <= 0) {
        eye.openAmount = 0;
        eye.state = EYE_INACTIVE; // Slot is now free
      }
      break;
    case EYE_INACTIVE:
      break;
  }
}

/**
 * spawnEye()
 *
 * Finds the first inactive eye slot and places a new eye at a random Y
 * position. Uses rejection sampling (up to 20 attempts) to ensure the
 * new eye is at least EYE_MIN_SPACING pixels away from all other active
 * eyes, preventing overlap. The eye is centered horizontally on the screen.
 *
 * @param matrix  Reference to the LED matrix (used for screen dimensions)
 */
static void spawnEye(Adafruit_Protomatter &matrix) {
  for (int i = 0; i < MAX_EYES; i++) {
    if (eyes[i].state == EYE_INACTIVE) {
      int newY = 0;
      bool valid = false;
      // Rejection sampling: try random Y positions until one is far enough
      // from all existing eyes, or give up after 20 attempts.
      for (int attempt = 0; attempt < 20; attempt++) {
        newY = random(EYE_HALF_HEIGHT + 2, matrix.height() - EYE_HALF_HEIGHT - 2);
        valid = true;
        for (int j = 0; j < MAX_EYES; j++) {
          if (j != i && eyes[j].state != EYE_INACTIVE) {
            if (abs(newY - eyes[j].y) < EYE_MIN_SPACING) {
              valid = false;
              break;
            }
          }
        }
        if (valid) break;
      }
      if (!valid) return;  // Screen too packed, skip spawning
      eyes[i].x = matrix.width() / 2;
      eyes[i].y = newY;
      eyes[i].halfHeight = EYE_HALF_HEIGHT;
      eyes[i].maxOpen = matrix.width() / 2 - 2;  // Nearly full screen width
      eyes[i].state = EYE_OPENING;
      eyes[i].openAmount = 0;
      eyes[i].blinksLeft = random(1, 5);
      eyes[i].timer = 0;
      eyes[i].irisX = 0;
      eyes[i].irisY = 0;
      eyes[i].irisTargetX = 0;
      eyes[i].irisTargetY = 0;
      eyes[i].lookTimer = random(20, 60);
      return;
    }
  }
}

/**
 * initDigital()
 *
 * Initializes the digital scene. Computes the vertical spacing so that
 * digitCharCount characters are evenly distributed across the screen
 * height with seamless wrapping, then populates the scrolling column
 * with random '0'/'1' characters.
 */
void initDigital(Adafruit_Protomatter &matrix) {
  // The default GFX font is 8 px tall; charScale multiplies that.
  // charOffset is the negative Y distance between characters, calculated
  // so they tile evenly and wrap from bottom back to top without a gap.
  int charHeight = 8 * charScale;
  charOffset = (((matrix.height() - (charHeight * (digitCharCount - 1))) / (digitCharCount - 1)) + charHeight) * -1;

  for (int i = 0; i < digitCharCount; i++) {
    digitChars[i] = initDigit(charOffset * i, matrix.color565(255, 255, 255));
  }
}

/**
 * drawDigital()
 *
 * Main entry point for the digital scene, called once per frame.
 *
 * Rendering order:
 *   1. Fill screen with a slowly drifting dark-red background
 *   2. Scroll and draw the column of binary digit characters
 *   3. Update and draw all active eyes (includes lids, lashes, iris)
 *   4. Spawn new eyes to maintain at least 2 on screen
 *   5. Update and draw expanding ripple rings
 *   6. Push the frame buffer to the display
 *
 * @param matrix  Reference to the LED matrix
 */
void drawDigital(Adafruit_Protomatter &matrix) {
  // --- Background color drift ---
  // Randomly nudge the red intensity up or down each frame, clamped to 15-50.
  // This creates a subtle breathing/pulsing effect on the background.
  bool addOrSub = random(2);
  if (bgRedVal > 50) addOrSub = false;
  if (bgRedVal < 15) addOrSub = true;
  if(addOrSub) {
    bgRedVal += random(2);
  } else {
    bgRedVal -= random(2);
  }

  uint16_t bgRedColor = matrix.color565(bgRedVal, 0, 0);
  matrix.fillScreen(bgRedColor);

  // --- Scrolling binary digits ---
  // Each character advances downward by 2 pixels per frame. When it scrolls
  // past the bottom, it wraps back to the top with a new random '0' or '1'.
  for (int i = 0; i < digitCharCount; i++) {
    digitChars[i].yOffset = digitChars[i].yOffset + 2;

    if (digitChars[i].yOffset > matrix.height()) {
      digitChars[i] = initDigit(charOffset, matrix.color565(255, 255, 255));
    }

    // Skip drawing characters that are still above the visible area
    if (digitChars[i].yOffset > charOffset) {
      matrix.drawChar(charXPos, digitChars[i].yOffset, digitChars[i].character, digitChars[i].color, bgRedColor, charScale);
    }
  }

  // --- Eyes ---
  int activeEyes = 0;
  for (int i = 0; i < MAX_EYES; i++) {
    if (eyes[i].state != EYE_INACTIVE) {
      updateEye(eyes[i]);
      drawAlmondEye(eyes[i], matrix);
      activeEyes++;
    }
  }

  // Guarantee at least 2 eyes are always visible
  while (activeEyes < 2) {
    spawnEye(matrix);
    activeEyes++;
  }
  // ~1.1% chance each frame to add another eye (up to 5 concurrent)
  if (activeEyes < 5 && random(90) == 0) {
    spawnEye(matrix);
  }

  // --- Ripples ---
  updateRipples(matrix);
  drawRipples(matrix);

  matrix.show();
}
