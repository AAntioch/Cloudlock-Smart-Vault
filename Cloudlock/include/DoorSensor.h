#ifndef DOOR_SENSOR_H
#define DOOR_SENSOR_H

#include <Arduino.h>

class DoorSensor {
private:
  int pin;
  bool lastOpen;
  bool firstRead;

public:
  DoorSensor(int sensorPin) {
    pin = sensorPin;
    lastOpen = false;
    firstRead = true;
  }

  void begin() {
    pinMode(pin, INPUT_PULLUP);
  }

  bool isOpen() {
    return digitalRead(pin) == HIGH;
  }

  bool changed(bool currentOpen) {
    if (firstRead || currentOpen != lastOpen) {
      firstRead = false;
      lastOpen = currentOpen;
      return true;
    }
    return false;
  }
};

#endif
