#ifndef CONFIG_H
#define CONFIG_H

const int DOOR_SENSOR_PIN = 17;
const int SERVO_PIN = 14;
const int BUZZER_PIN = 13;
const bool BUZZER_ACTIVE_HIGH = true;

const int RGB_RED_PIN = 12;
const int RGB_GREEN_PIN = 2;
const int RGB_BLUE_PIN = 4;

const int MPU_SDA = 26;
const int MPU_SCL = 27;

const int TFT_SCL = 32;
const int TFT_SDA = 33;
const int TFT_CS = 25;
const int TFT_RST = -1;
const int TFT_BL = -1;

const unsigned long SERVO_TOGGLE_MS = 5000;
const unsigned long MPU_READ_MS = 200;
const unsigned long STATUS_PRINT_MS = 1000;
const unsigned long ALERT_BLINK_MS = 500;

#include "Secrets.h"

#endif
