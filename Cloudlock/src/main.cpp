#include <Arduino.h>
#include "Config.h"
#include "DoorSensor.h"
#include "BarrierServo.h"
#include "DoorStatusLed.h"
#include "AlarmBuzzer.h"
#include "VaultDisplay.h"
#include "ObjectMotionSensor.h"
#include "PersistentLog.h"
#include "WiFiAlertClient.h"
#include "WiFiVaultServer.h"

volatile bool AlarmBuzzer::alarmActive = false;
AlarmBuzzer *AlarmBuzzer::instance = nullptr;

DoorSensor door(DOOR_SENSOR_PIN);
BarrierServo barrier(SERVO_PIN, 0, 90);
DoorStatusLed doorLed(RGB_RED_PIN, RGB_GREEN_PIN, RGB_BLUE_PIN);
AlarmBuzzer buzzer(BUZZER_PIN, BUZZER_ACTIVE_HIGH);
VaultDisplay display(TFT_SCL, TFT_SDA, TFT_CS, TFT_RST, TFT_BL);
ObjectMotionSensor objectSensor(MPU_SDA, MPU_SCL);
PersistentLog persistentLog;
WiFiAlertClient wifiAlertClient;
WiFiVaultServer wifiServer(door, barrier, doorLed, objectSensor, persistentLog);

bool lastDoorOpen = false;
bool lastObjectStolen = false;
bool lastDisplayedDoorOpen = false;
bool firstDisplay = true;
bool firstObjectState = true;
unsigned long lastMpuReadMs = 0;
unsigned long lastStatusPrintMs = 0;
unsigned long lastAlertBlinkMs = 0;

void setup() {
  Serial.begin(115200);
  delay(2500);
  Serial.println();
  Serial.println("BOOT Smart Vault");
  persistentLog.begin();
  persistentLog.addEvent("SYSTEM_BOOT");

  door.begin();
  doorLed.begin();
  buzzer.begin();
  barrier.begin();
  display.begin();

  objectSensor.begin();
  objectSensor.arm();
  wifiServer.begin();

  Serial.println();
  Serial.println("======== Cloudlock Smart Vault - Ready ========");
  Serial.println("Conexiuni componente:");
  Serial.println("MC38: GPIO17 + GND");
  Serial.println("RGB LED: R=GPIO12, G=GPIO2/D2, B=GPIO4, GND=GND");
  Serial.println("Servo MG90S: semnal=GPIO14, VCC=5V, GND=GND");
  Serial.println("LCD ST7789: SCL=GPIO32, SDA=GPIO33, CS=GPIO25, RES=3V3, BLO=3V3, VCC=3V3, GND=GND");
  Serial.println("MPU: VCC=3V3, GND=GND, SDA=GPIO26, SCL=GPIO27, ADO=GND, NCS=3V3");
  Serial.println("Buzzer TMB12A05: +=GPIO13, -=GND");
  Serial.println("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
}

void loop() {
  unsigned long now = millis();
  wifiServer.handleClient();

  bool doorOpen = door.isOpen();
  doorLed.update(doorOpen);

  objectSensor.retryIfNeeded(now);

  if (door.changed(doorOpen)) {
    lastDoorOpen = doorOpen;
    Serial.println(doorOpen ? "USA DESCHISA" : "USA INCHISA");
    persistentLog.addEvent(doorOpen ? "DOOR_OPENED" : "DOOR_CLOSED");
    display.show(doorOpen, objectSensor.isStolen(), true);
  }

  if (now - lastMpuReadMs >= MPU_READ_MS) {
    lastMpuReadMs = now;
    objectSensor.update();

    bool objectStolen = objectSensor.isStolen();
    if (firstObjectState) {
      firstObjectState = false;
      lastObjectStolen = objectStolen;
    } else if (objectStolen != lastObjectStolen) {
      lastObjectStolen = objectStolen;
      persistentLog.addEvent(objectStolen ? "OBJECT_STOLEN" : "OBJECT_SAFE_AGAIN");
      if (objectStolen) {
        wifiAlertClient.sendObjectStolenAlert();
      }
    }
  }

  bool screenNeedsUpdate = firstDisplay ||
                           doorOpen != lastDisplayedDoorOpen ||
                           display.isAlertMode() != objectSensor.isStolen();

  if (screenNeedsUpdate) {
    firstDisplay = false;
    lastDisplayedDoorOpen = doorOpen;
    display.show(doorOpen, objectSensor.isStolen());
    lastAlertBlinkMs = now;
  } else if (objectSensor.isStolen() && now - lastAlertBlinkMs >= ALERT_BLINK_MS) {
    lastAlertBlinkMs = now;
    display.blinkAlert(doorOpen);
  }

  if (now - lastStatusPrintMs >= STATUS_PRINT_MS) {
    lastStatusPrintMs = now;
    Serial.print("Status periodic: ");
    Serial.print(doorOpen ? "USA DESCHISA" : "USA INCHISA");
    Serial.print(" / obiect ");
    Serial.print(objectSensor.isStolen() ? "FURAT/ALERTA" : "SAFE");
    Serial.print(" / MPU ");
    Serial.print(objectSensor.isReady() ? "OK" : "EROARE");
    Serial.print(" / armat ");
    Serial.print(objectSensor.isArmed() ? "DA" : "NU");
    Serial.print(" / orient ");
    Serial.print(objectSensor.orient(), 3);
    Serial.print(" / motion ");
    Serial.print(objectSensor.motion(), 3);
    Serial.print(" / score ");
    Serial.println(objectSensor.score(), 3);
  }

  delay(50);
}
