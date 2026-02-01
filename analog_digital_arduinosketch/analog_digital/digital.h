/**
 * digital.h
 *
 * Header for the digital visualization mode. This mode displays a
 * "Matrix"-style column of scrolling binary digits overlaid with animated
 * diamond-shaped eyes that blink, look around, and emit ripple effects.
 *
 * Defines the DigitChar struct for individual scrolling characters, layout
 * constants, and the public API used by the main sketch.
 */

#ifndef DIGITAL_H
#define DIGITAL_H

#include <Adafruit_Protomatter.h>

/**
 * DigitChar
 *
 * Represents a single scrolling character in the binary rain column.
 * Each character scrolls downward and wraps back to the top when it
 * passes the bottom of the screen.
 */
struct DigitChar {
  char character;   // The ASCII character to display ('0' or '1')
  uint16_t color;   // 16-bit RGB565 display color
  int16_t yOffset;  // Current vertical position on screen
};

const uint8_t charXPos =  6;          // X pixel position of the character column
const uint8_t charScale = 4;          // GFX font scale factor (4x the 8px base)
const uint8_t digitCharCount = 12;    // Number of characters in the scrolling column

extern int8_t charOffset;
extern DigitChar digitChars[digitCharCount];

/**
 * initDigit()
 *
 * Creates a new DigitChar at the given Y position with a random '0' or '1'.
 *
 * @param yOffset  Initial vertical position
 * @param color    16-bit RGB565 color
 * @return         Initialized DigitChar
 */
DigitChar initDigit(int yOffset, int color);

/**
 * drawDigital()
 *
 * Renders one frame of the digital scene: background, scrolling binary
 * digits, animated eyes with eyelashes, and expanding ripple effects.
 *
 * @param matrix  Reference to the LED matrix
 */
void drawDigital(Adafruit_Protomatter &matrix);

#endif
