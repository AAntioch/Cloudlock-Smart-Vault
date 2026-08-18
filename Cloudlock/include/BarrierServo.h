#ifndef BARRIER_SERVO_H
#define BARRIER_SERVO_H

#include <Arduino.h>
#include <ESP32Servo.h>
#include "Config.h"

class BarrierServo {
private:
  int pin;
  int lockedAngle;
  int unlockedAngle;
  bool unlocked;
  unsigned long lastToggleMs;
  Servo servo;

public:
  BarrierServo(int servoPin, int locked, int unlockedPosition) {
    pin = servoPin;
    lockedAngle = locked;
    unlockedAngle = unlockedPosition;
    unlocked = false;
    lastToggleMs = 0;
  }

  void begin() {
    servo.setPeriodHertz(50);
    servo.attach(pin, 500, 2400);
    setUnlocked(false);
  }

  void setUnlocked(bool value) {
    unlocked = value;
    servo.write(unlocked ? unlockedAngle : lockedAngle);

    Serial.print("Servo status: ");
    Serial.println(unlocked ? "DOOR UNLOCKED - bariera la 90 grade" : "DOOR LOCKED - bariera la 0 grade");
  }

  void update(unsigned long now) {
    if (now - lastToggleMs >= SERVO_TOGGLE_MS) {
      lastToggleMs = now;
      setUnlocked(!unlocked);
    }
  }

  bool isUnlocked() {
    return unlocked;
  }
};

#endif
