# Cloudlock: Wi-Fi Smart Vault

An ESP32-based smart vault that combines door monitoring, object movement detection, remote servo control, persistent event logging and e-mail security alerts.

The firmware hosts a responsive Wi-Fi dashboard directly on the ESP32. The protected object is monitored by an MPU-6500-compatible IMU, while the MC38 magnetic contact monitors the vault door. A theft event triggers the TFT alert display, RGB status LED, intermittent buzzer, NVS event logging and an HTTP request to a small Flask-to-Gmail notification gateway.

## Main Capabilities

- MC38 magnetic door sensing with live `LOCKED` / `UNLOCKED` state.
- MPU-based object motion detection with calibration and noise filtering.
- MG90S servo barrier controlled remotely from the browser.
- ST7789 display, RGB LED feedback and TMB12A05 audible alarm.
- ESP32-hosted Wi-Fi dashboard with command and event history.
- ESP32 NVS storage for the latest 10 commands and 10 security events.
- Gmail alert forwarding through a local Flask gateway.

## Project Documentation

The complete technical documentation, wiring map, software architecture, API routes, security setup and test checklist are available here:

**[Open the Cloudlock project README](Cloudlock/README.md)**

## Stack

`ESP32` `Arduino` `PlatformIO` `C++` `Wi-Fi` `I2C` `SPI` `NVS` `Flask` `SMTP`

## Author

**Bizău Mario Ștefan**  
Student at the Faculty of Automation and Computers, Systems Engineering at the Politehnica University of Timisoara
