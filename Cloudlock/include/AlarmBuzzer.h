#ifndef ALARM_BUZZER_H
#define ALARM_BUZZER_H

#include <Arduino.h>

class AlarmBuzzer {
private:
  int pin;
  bool activeHigh;
  volatile bool on;

public:
  static volatile bool alarmActive;
  static AlarmBuzzer *instance;
  static const unsigned long BEEP_ON_MS = 60;
  static const unsigned long BEEP_OFF_MS = 80;

  AlarmBuzzer(int buzzerPin, bool activeHighMode) {
    pin = buzzerPin;
    activeHigh = activeHighMode;
    on = false;
  }

  void begin() {
    pinMode(pin, OUTPUT);
    set(false);
    instance = this;
    xTaskCreatePinnedToCore(taskFunction, "buzzer", 2048, nullptr, 1, nullptr, 0);
  }

  void set(bool value) {
    on = value;
    digitalWrite(pin, value == activeHigh ? HIGH : LOW);
  }

  void updatePattern() {
    if (alarmActive) {
      set(true);
      vTaskDelay(pdMS_TO_TICKS(BEEP_ON_MS));
      set(false);
      vTaskDelay(pdMS_TO_TICKS(BEEP_OFF_MS));
    } else {
      set(false);
      vTaskDelay(pdMS_TO_TICKS(20));
    }
  }

  static void taskFunction(void *parameter) {
    (void)parameter;
    for (;;) {
      if (instance != nullptr) {
        instance->updatePattern();
      } else {
        vTaskDelay(pdMS_TO_TICKS(20));
      }
    }
  }
};

#endif
