#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>

/**
 * Sensor - Reads a digital input and outputs USB MIDI + a digital output signal.
 *
 * Each sensor monitors an input pin for a LOW-to-HIGH transition. When triggered,
 * it sends a MIDI note and CC message, drives an output pin HIGH (for LED displays),
 * and automatically turns everything off after a timed duration.
 */
class Sensor {
private:
    int _inPin;            // Digital input pin (sensor trigger)
    int _outPin;           // Digital output pin (drives LED display)
    int _midiNote;
    int _midiVelocity;
    int _midiChannel;      // Per-sensor MIDI channel for note messages
    int _midiCCOn;         // CC number sent on note-on (midiChannel * 2)
    int _midiCCOff;        // CC number sent on note-off (midiChannel * 2 + 1)
    int _midiCCChannel;    // Shared CC channel (16) — QLC+ only listens on one channel
    int _state;            // Last known pin state for edge detection
    unsigned long _noteDuration;
    unsigned long _debounceTime; // How long input must stay HIGH before triggering (ms)
    bool _debouncing;      // True while waiting for debounce period to confirm
    bool _noteOn;
    elapsedMillis _noteTimer;
    elapsedMillis _debounceTimer;

    /** Turns off the note and output pin once _noteDuration has elapsed. */
    void _checkNote();

public:
    /** @param inPin    Digital input pin to read the sensor from.
     *  @param outPin   Digital output pin to drive (e.g. LED display).
     *  @param midiChannel  MIDI channel (1-15) for this sensor's note messages. */
    Sensor(int inPin, int outPin, int midiChannel);

    /** Configures input and output pin modes. Call once from setup(). */
    void init();

    /** Polls the sensor and manages note timing. Call every loop iteration. */
    void check();

    /** Triggers MIDI note-on, CC message, and drives output pin HIGH. */
    void playNote();
};

#endif