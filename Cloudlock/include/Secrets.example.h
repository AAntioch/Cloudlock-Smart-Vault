#ifndef SECRETS_H
#define SECRETS_H

// Copy this file as Secrets.h and set your own local values.
const char WIFI_STA_SSID[] = "YOUR_WIFI_NAME";
const char WIFI_STA_PASSWORD[] = "YOUR_WIFI_PASSWORD";

// The IP address belongs to the computer that runs web_app.py.
const char FLASK_ALERT_URL[] = "http://192.168.1.100:5000/api/wifi-alert";
const char FLASK_ALERT_KEY[] = "choose-a-private-shared-key";

#endif
