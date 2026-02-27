#include "Sensor.h"

// Array size and sensorCount must match to avoid out-of-bounds access.
// Each Sensor is constructed as: Sensor(inPin, outPin, midiChannelAnalog, midiChannelDigital)
// Channels are assigned in pairs: sensor 1 = ch 1/2, sensor 2 = ch 3/4, etc.
int sensorCount = 5;

Sensor sensors[5] = {
  Sensor(0, 33, 1,  2),
  Sensor(1, 34, 3,  4),
  Sensor(2, 35, 5,  6),
  Sensor(3, 36, 7,  8),
  Sensor(4, 37, 9, 10),
};

class Sensor;
void setupSensors();
void checkSensors();
void test();

void setup() {
  Serial.begin(115200);
  setupSensors();
}

void loop() {
  checkSensors();
  // test();
}

/** Initializes all sensor pin modes and starts their analog notes. */
void setupSensors() {
  for (int i = 0; i < sensorCount; i++) {
    sensors[i].init();
  }
}

/** Polls all sensors and manages their note timers. */
void checkSensors() {
  for (int i = 0; i < sensorCount; i++) {
    sensors[i].check();
  }
}

/** Debug helper: fires all sensors into digital mode, waits for them to auto-off, then pauses. */
void test() {
  // delay(6000);
  for (int i = 0; i < sensorCount; i++) {
    sensors[i].playDigital();
    delay(6000);
    sensors[i].check();
    delay(6000);
  }
  // delay(6000);
  // for (int i = 0; i < sensorCount; i++) {
  //   sensors[i].check();
  // }
}
