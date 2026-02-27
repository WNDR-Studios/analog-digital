#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>

/**
 * Sensor - Reads a digital input and outputs USB MIDI in two modes: analog and digital.
 *
 * After init(), the sensor immediately plays a sustained note on the analog MIDI channel.
 * When the input pin transitions LOW-to-HIGH (trigger), the analog note stops and a
 * digital note begins on the digital MIDI channel. After the note duration elapses,
 * the digital note stops and the analog note resumes automatically.
 *
 * CC messages are sent on channel 16 for QLC+ lighting control.
 */
class Sensor {
private:
    int _inPin;                 // Digital input pin (sensor trigger)
    int _outPin;                // Digital output pin (drives LED display)
    int _midiNote;
    int _midiVelocity;
    int _midiChannelAnalog;    // MIDI channel for the sustained analog note
    int _midiChannelDigital;   // MIDI channel for the triggered digital note
    int _midiCCOn;             // CC number sent on trigger (midiChannelAnalog * 2)
    int _midiCCOff;            // CC number sent on release (midiChannelAnalog * 2 + 1)
    int _midiCCChannel;        // Shared CC channel (16) — QLC+ only listens on one channel
    int _state;                // Last known pin state for edge detection
    unsigned long _noteDuration;
    unsigned long _debounceTime; // How long input must stay HIGH before triggering (ms)
    bool _debouncing;           // True while waiting for debounce period to confirm
    bool _analogActive;         // True while the analog note is sustaining
    bool _digitalActive;        // True while the digital note is playing
    elapsedMillis _noteTimer;
    elapsedMillis _debounceTimer;

    /** Turns off the digital note and resumes analog once _noteDuration has elapsed. */
    void _checkNote();

public:
    /** @param inPin              Digital input pin to read the sensor from.
     *  @param outPin             Digital output pin to drive (e.g. LED display).
     *  @param midiChannelAnalog  MIDI channel for the sustained analog note.
     *  @param midiChannelDigital MIDI channel for the triggered digital note. */
    Sensor(int inPin, int outPin, int midiChannelAnalog, int midiChannelDigital);

    /** Configures pin modes and starts the analog note. Call once from setup(). */
    void init();

    /** Polls the sensor and manages note timing. Call every loop iteration. */
    void check();

    /** Sends note-on on the analog channel. */
    void playAnalog();

    /** Sends note-off on the analog channel. */
    void stopAnalog();

    /** Stops the analog note, sends note-on on the digital channel, and drives output pin HIGH. */
    void playDigital();

    /** Sends note-off on the digital channel, drives output pin LOW, and resumes analog. */
    void stopDigital();
};

#endif
