#include "Sensor.h"

/**
 * @param inPin              Digital input pin to read the sensor from.
 * @param outPin             Digital output pin to drive (e.g. LED display).
 * @param midiChannelAnalog  MIDI channel for the sustained analog note.
 * @param midiChannelDigital MIDI channel for the triggered digital note.
 */
Sensor::Sensor(int inPin, int outPin, int midiChannelAnalog, int midiChannelDigital) {
    _inPin = inPin;
    _outPin = outPin;
    _midiNote = 60;
    _midiVelocity = 100;
    _midiChannelAnalog = midiChannelAnalog;
    _midiChannelDigital = midiChannelDigital;

    // CC pair derived from the analog channel so QLC+ can distinguish sensors.
    _midiCCOn = midiChannelAnalog * 2;
    _midiCCOff = _midiCCOn + 1;

    // QLC+ only listens on one MIDI channel, so all CC messages share channel 16.
    _midiCCChannel = 16;

    _state = LOW;
    _noteDuration = 5000;
    _debounceTime = 250;
    _debouncing = false;
    _analogActive = false;
    _digitalActive = false;
    _noteTimer = 0;
    _debounceTimer = 0;
}

/**
 * Sends note-on on the analog channel. Ignored if already active.
 */
void Sensor::playAnalog() {
    if (_analogActive) return;
    usbMIDI.sendNoteOn(_midiNote, _midiVelocity, _midiChannelAnalog);
    _analogActive = true;
}

/**
 * Sends note-off on the analog channel. Ignored if not active.
 */
void Sensor::stopAnalog() {
    if (!_analogActive) return;
    usbMIDI.sendNoteOff(_midiNote, _midiVelocity, _midiChannelAnalog);
    _analogActive = false;
}

/**
 * Stops the analog note, sends note-on on the digital channel, drives output pin HIGH,
 * and starts the note duration timer. Ignored if digital is already active.
 */
void Sensor::playDigital() {
    if (_digitalActive) return;
    stopAnalog();
    digitalWrite(_outPin, HIGH);
    usbMIDI.sendNoteOn(_midiNote, _midiVelocity, _midiChannelDigital);
    usbMIDI.sendControlChange(_midiCCOn, 1, _midiCCChannel);
    _noteTimer = 0;
    _digitalActive = true;
}

/**
 * Sends note-off on the digital channel, drives output pin LOW, and resumes the analog note.
 * Ignored if digital is not active.
 */
void Sensor::stopDigital() {
    if (!_digitalActive) return;
    digitalWrite(_outPin, LOW);
    usbMIDI.sendNoteOff(_midiNote, _midiVelocity, _midiChannelDigital);
    usbMIDI.sendControlChange(_midiCCOff, 1, _midiCCChannel);
    _digitalActive = false;
    _noteTimer = 0;
    playAnalog();
}

/**
 * Auto-off timer: stops the digital note and resumes analog once _noteDuration has elapsed.
 * Called internally each loop iteration by check().
 */
void Sensor::_checkNote() {
    if (!_digitalActive) return;

    if (_noteTimer >= _noteDuration) {
        stopDigital();
    }
}

/**
 * Configures input and output pin modes, then starts the analog note.
 * Call once from setup().
 */
void Sensor::init() {
    pinMode(_inPin, INPUT);
    pinMode(_outPin, OUTPUT);
    digitalWrite(_outPin, LOW);
    playAnalog();
}

/**
 * Polls the sensor and manages note timing. Call every loop iteration.
 *
 * Detects rising edges (LOW → HIGH) on the input pin. The input must stay
 * HIGH for _debounceTime (250ms) before triggering, filtering out noise.
 * While a digital note is active, new triggers are ignored until it expires.
 */
void Sensor::check() {
    _checkNote();

    if (_digitalActive) return;

    int curState = digitalRead(_inPin);

    if (_debouncing) {
        if (curState == LOW) {
            // Input dropped before debounce period elapsed — false trigger
            _debouncing = false;
        } else if (_debounceTimer >= _debounceTime) {
            // Input held HIGH long enough — confirmed trigger
            _debouncing = false;
            _state = HIGH;
            playDigital();
        }
        return;
    }

    if (curState == _state) return;

    if (curState == HIGH) {
        // Input just went HIGH — start debounce timer
        _debouncing = true;
        _debounceTimer = 0;
    } else {
        _state = curState;
    }
}
