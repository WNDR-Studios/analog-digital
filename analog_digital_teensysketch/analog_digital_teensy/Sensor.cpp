#include "Sensor.h"

/**
 * @param inPin       Digital input pin to read the sensor from.
 * @param outPin      Digital output pin to drive (e.g. LED display).
 * @param midiChannel MIDI channel (1-5) for this sensor's note messages.
 */
Sensor::Sensor(int inPin, int outPin, int midiChannel) {
    _inPin = inPin;
    _outPin = outPin;
    _midiNote = 60;
    _midiVelocity = 100;
    _midiChannel = midiChannel;

    // Each sensor gets a unique CC pair derived from its channel number,
    // so QLC+ can distinguish which sensor triggered on a single CC channel.
    _midiCCOn = midiChannel * 2;
    _midiCCOff = _midiCCOn + 1;

    // QLC+ only listens on one MIDI channel, so all CC messages share channel 16.
    _midiCCChannel = 16;

    _state = LOW;
    _noteDuration = 5000;
    _noteOn = false;
    _noteTimer = 0;
}

/**
 * Triggers MIDI note-on, CC message, and drives output pin HIGH.
 * Ignored if a note is already playing on this sensor.
 */
void Sensor::playNote() {
  Serial.print("Playing note on ");
  Serial.println(_midiChannel);
  if (_noteOn) return;

  digitalWrite(_outPin, HIGH);
  usbMIDI.sendNoteOn(_midiNote, _midiVelocity, _midiChannel);
  usbMIDI.sendControlChange(_midiCCOn, 1, _midiCCChannel);
  _noteTimer = 0;
  _noteOn = true;
}

/**
 * Auto-off timer: turns off the note and output pin once _noteDuration has elapsed.
 * Called internally each loop iteration by check().
 */
void Sensor::_checkNote() {
  if (!_noteOn) return;

  if (_noteTimer >= _noteDuration) {
    digitalWrite(_outPin, LOW);
    usbMIDI.sendNoteOff(_midiNote, _midiVelocity, _midiChannel);
    usbMIDI.sendControlChange(_midiCCOff, 1, _midiCCChannel);
    _noteOn = false;
    _noteTimer = 0;
  }
}

/**
 * Configures input and output pin modes. Call once from setup().
 */
void Sensor::init() {
    pinMode(_inPin, INPUT);
    pinMode(_outPin, OUTPUT);
    digitalWrite(_outPin, LOW);
}

/**
 * Polls the sensor and manages note timing. Call every loop iteration.
 *
 * Detects rising edges (LOW → HIGH) on the input pin to trigger playNote().
 * While a note is active, new triggers are ignored until the note expires.
 */
void Sensor::check() {
    _checkNote();

    if (_noteOn) return;

    int curState = digitalRead(_inPin);
    if (curState == _state) return;

    if (curState == HIGH) {
      playNote();
    }

    _state = curState;
}
