#include "Sensor.h"

// Array size and sensorCount must match to avoid out-of-bounds access.
// Each Sensor is constructed as: Sensor(inPin, outPin, midiChannel)
int sensorCount = 5;

Sensor sensors[5] = {
  Sensor(0, 6, 1),
  Sensor(1, 7, 2),
  Sensor(2, 8, 3),
  Sensor(3, 9, 4),
  Sensor(4, 10, 5),
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

/** Initializes all sensor pin modes. */
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

/** Debug helper: fires all sensors at once, waits for them to auto-off, then pauses. */
void test() {
  for (int i = 0; i < sensorCount; i++) {
    sensors[i].playNote();
  }

  delay(4200);

  for (int i = 0; i < sensorCount; i++) {
    sensors[i].check();
  }

  delay(5000);
}