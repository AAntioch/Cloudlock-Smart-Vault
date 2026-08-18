#ifndef WIFI_ALERT_CLIENT_H
#define WIFI_ALERT_CLIENT_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "Config.h"

class WiFiAlertClient {
private:
  bool enabled() {
    return strlen(FLASK_ALERT_URL) > 0;
  }

public:
  bool sendObjectStolenAlert() {
    if (!enabled()) {
      Serial.println("Email WiFi: FLASK_ALERT_URL nu este configurat.");
      return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Email WiFi: ESP32 nu este conectat la WiFi.");
      return false;
    }

    WiFiClient client;
    HTTPClient http;
    http.setTimeout(3500);

    if (!http.begin(client, FLASK_ALERT_URL)) {
      Serial.println("Email WiFi: nu pot deschide conexiunea HTTP catre Flask.");
      return false;
    }

    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Cloudlock-Key", FLASK_ALERT_KEY);

    const char *payload = "{\"event\":\"OBJECT_STOLEN\",\"message\":\"ALERT !!!  The object has been stolen !\"}";
    int code = http.POST(payload);
    String response = http.getString();
    http.end();

    if (code >= 200 && code < 300) {
      Serial.println("Email WiFi: alerta trimisa catre Flask.");
      return true;
    }

    Serial.print("Email WiFi: eroare HTTP ");
    Serial.print(code);
    if (response.length() > 0) {
      Serial.print(" / ");
      Serial.print(response);
    }
    Serial.println();
    return false;
  }
};

#endif
