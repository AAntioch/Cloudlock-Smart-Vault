#ifndef DOOR_STATUS_LED_H
#define DOOR_STATUS_LED_H

#include <Arduino.h>

class DoorStatusLed {
private:
  static const int RED_CHANNEL = 4;
  static const int GREEN_CHANNEL = 5;
  static const int BLUE_CHANNEL = 6;
  static const int PWM_FREQUENCY = 5000;
  static const int PWM_RESOLUTION = 8;

  int redPin;
  int greenPin;
  int bluePin;
  bool manualMode;

public:
  DoorStatusLed(int red, int green, int blue) {
    redPin = red;
    greenPin = green;
    bluePin = blue;
    manualMode = false;
  }

  void begin() {
    ledcSetup(RED_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcSetup(GREEN_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcSetup(BLUE_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttachPin(redPin, RED_CHANNEL);
    ledcAttachPin(greenPin, GREEN_CHANNEL);
    ledcAttachPin(bluePin, BLUE_CHANNEL);
    showClosed();
  }

  void setColor(uint8_t red, uint8_t green, uint8_t blue) {
    ledcWrite(RED_CHANNEL, red);
    ledcWrite(GREEN_CHANNEL, green);
    ledcWrite(BLUE_CHANNEL, blue);
  }

  void showClosed() {
    setColor(255, 0, 0);
  }

  void showOpen() {
    setColor(0, 255, 0);
  }

  void setManualColor(uint8_t red, uint8_t green, uint8_t blue) {
    manualMode = true;
    setColor(red, green, blue);
  }

  void useDoorMode(bool doorOpen) {
    manualMode = false;
    update(doorOpen);
  }

  bool isManualMode() {
    return manualMode;
  }

  void update(bool doorOpen) {
    if (manualMode) {
      return;
    }

    if (doorOpen) {
      showOpen();
    } else {
      showClosed();
    }
  }
};

#endif
