#ifndef OBJECT_MOTION_SENSOR_H
#define OBJECT_MOTION_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "AlarmBuzzer.h"

class ObjectMotionSensor {
private:
  static const uint8_t REG_WHO_AM_I = 0x75;
  static const uint8_t REG_PWR_MGMT_1 = 0x6B;
  static const uint8_t REG_ACCEL_CONFIG = 0x1C;
  static const uint8_t REG_GYRO_CONFIG = 0x1B;
  static const uint8_t REG_ACCEL_XOUT_H = 0x3B;

  int sdaPin;
  int sclPin;
  uint8_t address;
  bool ready;
  bool armed;
  bool stolen;
  bool hasPrevious;

  float baseAx;
  float baseAy;
  float baseAz;
  float prevAx;
  float prevAy;
  float prevAz;
  float orientationDelta;
  float motionDelta;
  float motionScore;

  int movedCount;
  int safeCount;
  int failCount;
  unsigned long armedAtMs;
  unsigned long lastRetryMs;

  uint8_t readRegister(uint8_t reg) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    if (Wire.endTransmission(true) != 0) {
      return 0xFF;
    }
    delay(2);
    Wire.requestFrom(address, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0xFF;
  }

  void writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
  }

  int16_t readInt16() {
    int16_t high = Wire.read();
    int16_t low = Wire.read();
    return (high << 8) | low;
  }

  bool readAccel(float &ax, float &ay, float &az) {
    Wire.beginTransmission(address);
    Wire.write(REG_ACCEL_XOUT_H);
    if (Wire.endTransmission(true) != 0) {
      return false;
    }

    delay(2);

    if (Wire.requestFrom(address, (uint8_t)6) != 6) {
      return false;
    }

    ax = readInt16() / 16384.0f;
    ay = readInt16() / 16384.0f;
    az = readInt16() / 16384.0f;
    return true;
  }

  void scanI2C() {
    Serial.println("I2C scanner pe pinii MPU...");
    bool found = false;

    for (uint8_t i = 1; i < 127; i++) {
      Wire.beginTransmission(i);
      if (Wire.endTransmission() == 0) {
        found = true;
        Serial.print("I2C device gasit la 0x");
        if (i < 16) {
          Serial.print("0");
        }
        Serial.println(i, HEX);
        delay(2);
      }
    }

    if (!found) {
      Serial.println("I2C scanner: nu am gasit niciun device pe SDA=GPIO26, SCL=GPIO27.");
    }
  }

  bool selectAddress() {
    const uint8_t addresses[] = {0x68, 0x69};

    for (uint8_t i = 0; i < 2; i++) {
      address = addresses[i];
      uint8_t whoAmI = readRegister(REG_WHO_AM_I);

      Serial.print("MPU scan addr 0x");
      Serial.print(address, HEX);
      Serial.print(" WHO_AM_I = 0x");
      Serial.println(whoAmI, HEX);

      if (whoAmI != 0x00 && whoAmI != 0xFF) {
        Serial.print("MPU: folosesc adresa 0x");
        Serial.println(address, HEX);
        return true;
      }
    }

    Serial.println("MPU: nu raspunde pe 0x68 sau 0x69. Verifica VCC/GND/SDA/SCL/ADO/NCS.");
    return false;
  }

  bool calibrate() {
    Serial.println("MPU: calibrare pozitie obiect. Nu misca obiectul 2 secunde...");

    const int samples = 100;
    int valid = 0;
    float sumAx = 0.0f;
    float sumAy = 0.0f;
    float sumAz = 0.0f;

    for (int i = 0; i < samples; i++) {
      float ax, ay, az;
      if (readAccel(ax, ay, az)) {
        sumAx += ax;
        sumAy += ay;
        sumAz += az;
        valid++;
      }
      delay(20);
    }

    if (valid < 80) {
      Serial.println("MPU: calibrare esuata, prea putine citiri valide.");
      return false;
    }

    baseAx = sumAx / valid;
    baseAy = sumAy / valid;
    baseAz = sumAz / valid;
    prevAx = baseAx;
    prevAy = baseAy;
    prevAz = baseAz;
    hasPrevious = true;

    Serial.print("MPU: pozitie sigura memorata ax=");
    Serial.print(baseAx, 3);
    Serial.print(" ay=");
    Serial.print(baseAy, 3);
    Serial.print(" az=");
    Serial.println(baseAz, 3);
    return true;
  }

  bool getDeltas(float &orient, float &motion) {
    float ax, ay, az;
    if (!ready || !readAccel(ax, ay, az)) {
      return false;
    }

    orient = sqrtf(
      (ax - baseAx) * (ax - baseAx) +
      (ay - baseAy) * (ay - baseAy) +
      (az - baseAz) * (az - baseAz)
    );

    if (hasPrevious) {
      motion = sqrtf(
        (ax - prevAx) * (ax - prevAx) +
        (ay - prevAy) * (ay - prevAy) +
        (az - prevAz) * (az - prevAz)
      );
    } else {
      motion = 0.0f;
      hasPrevious = true;
    }

    prevAx = ax;
    prevAy = ay;
    prevAz = az;
    return true;
  }

public:
  ObjectMotionSensor(int sda, int scl) {
    sdaPin = sda;
    sclPin = scl;
    address = 0x68;
    ready = false;
    armed = false;
    stolen = false;
    hasPrevious = false;
    baseAx = baseAy = baseAz = 0.0f;
    prevAx = prevAy = prevAz = 0.0f;
    orientationDelta = motionDelta = motionScore = 0.0f;
    movedCount = safeCount = failCount = 0;
    armedAtMs = 0;
    lastRetryMs = 0;
  }

  bool begin() {
    Wire.begin(sdaPin, sclPin);
    Wire.setClock(50000);

    scanI2C();
    ready = selectAddress();
    if (!ready) {
      return false;
    }

    writeRegister(REG_PWR_MGMT_1, 0x00);
    delay(100);
    writeRegister(REG_ACCEL_CONFIG, 0x00);
    writeRegister(REG_GYRO_CONFIG, 0x00);
    failCount = 0;
    return true;
  }

  bool arm() {
    stolen = false;
    movedCount = 0;
    safeCount = 0;
    orientationDelta = motionDelta = motionScore = 0.0f;

    if (!ready) {
      armed = false;
      return false;
    }

    armed = calibrate();
    armedAtMs = millis();
    return armed;
  }

  void retryIfNeeded(unsigned long now) {
    if (ready || now - lastRetryMs < 5000) {
      return;
    }

    lastRetryMs = now;
    Serial.println("MPU: incerc reconectare...");
    ready = begin();
    armed = arm();
    Serial.println(armed ? "MPU: reconectat si armat." : "MPU: reconectarea a esuat.");
  }

  void update() {
    if (!ready || !armed) {
      stolen = false;
      AlarmBuzzer::alarmActive = false;
      return;
    }

    if (millis() - armedAtMs < 1000) {
      AlarmBuzzer::alarmActive = false;
      return;
    }

    float orient = 0.0f;
    float motion = 0.0f;
    if (!getDeltas(orient, motion)) {
      failCount++;
      movedCount = 0;
      safeCount = 0;

      if (failCount >= 20) {
        Serial.println("MPU: prea multe citiri esuate, dezactivez temporar monitorizarea.");
        ready = false;
        armed = false;
        stolen = false;
        AlarmBuzzer::alarmActive = false;
      }
      return;
    }

    failCount = 0;
    orientationDelta = orient;
    motionDelta = motion;
    motionScore = motionScore * 0.88f + motionDelta;

    bool movedByOrientation = orientationDelta > 0.35f;
    bool movedByMotion = motionDelta > 0.07f;
    bool movedByFastMotion = motionDelta > 0.22f;
    bool movedByScore = motionScore > 0.32f;

    if (movedByFastMotion) {
      movedCount = 2;
      safeCount = 0;
    } else if (movedByOrientation || movedByMotion || movedByScore) {
      movedCount++;
      safeCount = 0;
    } else if (orientationDelta < 0.18f && motionDelta < 0.035f && motionScore < 0.144f) {
      safeCount++;
      movedCount = 0;
      if (!stolen) {
        motionScore = 0.0f;
      }
    } else {
      movedCount = 0;
      safeCount = 0;
    }

    if (!stolen && movedCount >= 2) {
      stolen = true;
      Serial.print("ALERTA: obiect miscat, orientationDelta=");
      Serial.print(orientationDelta, 3);
      Serial.print(" motionDelta=");
      Serial.print(motionDelta, 3);
      Serial.print(" motionScore=");
      Serial.println(motionScore, 3);
    }

    if (stolen && safeCount >= 6) {
      stolen = false;
      motionScore = 0.0f;
      Serial.print("Obiectul a revenit la pozitia initiala, orientationDelta=");
      Serial.print(orientationDelta, 3);
      Serial.print(" motionDelta=");
      Serial.print(motionDelta, 3);
      Serial.print(" motionScore=");
      Serial.println(motionScore, 3);
    }

    AlarmBuzzer::alarmActive = stolen;
  }

  bool isReady() {
    return ready;
  }

  bool isArmed() {
    return armed;
  }

  bool isStolen() {
    return stolen;
  }

  float orient() {
    return orientationDelta;
  }

  float motion() {
    return motionDelta;
  }

  float score() {
    return motionScore;
  }
};

#endif
