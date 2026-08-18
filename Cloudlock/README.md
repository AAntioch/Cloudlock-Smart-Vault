# Cloudlock: Wi-Fi Smart Vault

Cloudlock is an embedded security project built around an ESP32. It simulates a compact smart vault that monitors a door, protects an object placed inside it, provides local visual and acoustic feedback, and exposes a Wi-Fi control dashboard.

The project was designed as an academic embedded systems assignment, then extended into a portfolio-oriented implementation: hardware integration, object-oriented firmware, persistent event logging, web control, and e-mail alert delivery all work together as one system.

## Highlights

- Detects the vault door state with an MC38 normally-closed magnetic contact.
- Uses an MG90S servo as a physical locking barrier, controlled from the web interface.
- Detects object movement with an MPU-6500-compatible IMU and raises an alarm.
- Shows live status and alerts on a 1.54 inch ST7789 TFT display.
- Drives an RGB LED automatically from the door state, with an optional manual color picker in the UI.
- Stores the last 10 web commands and last 10 vault events in ESP32 NVS memory.
- Sends an e-mail alert when an object is detected as stolen.
- Serves the monitoring and control dashboard directly from the ESP32 over Wi-Fi.

## System Architecture

```text
MC38 door contact -----------+                         +--> ST7789 TFT display
MPU-6500 motion sensor ------+--> ESP32 firmware ------+--> RGB status LED
Web dashboard commands ------+                         +--> MG90S servo barrier
                                                       +--> TMB12A05 buzzer
                                                       +--> NVS persistent logs
                                                       +--> HTTP alert request
                                                                  |
                                                                  v
                                                        Flask e-mail gateway
                                                                  |
                                                                  v
                                                              Gmail SMTP
```

The ESP32 is the main controller. It reads physical inputs, applies the vault rules, controls the output devices, saves events in NVS, and hosts the Wi-Fi dashboard. Flask is deliberately kept as a small local e-mail gateway: it receives an HTTP alert from the ESP32 and sends the notification through Gmail SMTP.

## Demo Scenarios

| Scenario | System behaviour |
|---|---|
| Door closed | MC38 reports `LOCKED`, the RGB LED is red, and the TFT shows that the object is safe. |
| Door open | MC38 reports `UNLOCKED`, the RGB LED is green, and the TFT keeps showing the object state. |
| Object moved | The MPU detects a meaningful movement relative to its calibrated position. The buzzer beeps, the TFT blinks an alert, the event is saved, and an e-mail request is sent. |
| Object returned | The MPU detects a stable state again, the buzzer stops, and the display returns to the safe message. |
| Remote barrier control | The user presses `LOCK` or `UNLOCK` in the browser; the ESP32 moves the MG90S servo and saves the command in NVS. |

## Hardware

| Component | Purpose |
|---|---|
| ESP32 DevKit | Main microcontroller, Wi-Fi server, hardware controller and NVS storage. |
| MC38 magnetic contact | Detects whether the vault door is physically closed or open. |
| MG90S servo motor | Simulates the mechanical lock/barrier: 0 degrees means locked and 90 degrees means unlocked. |
| MPU-6500-compatible IMU | Detects movement of the protected object using three-axis acceleration measurements. |
| ST7789 TFT LCD | Shows door state, object state and theft alerts locally. |
| TMB12A05 buzzer | Provides an intermittent acoustic alarm while the object is marked as stolen. |
| RGB LED module | Red for a closed door, green for an open door, plus a manual RGB dashboard feature. |

## Wiring

All modules must share a common ESP32 ground.

| Component | Module pin | ESP32 connection | Notes |
|---|---|---|---|
| MC38 | wire 1 | GPIO17 | Uses `INPUT_PULLUP`; the contact has no polarity. |
| MC38 | wire 2 | GND | `LOW` means the magnet is close and the door is closed. |
| MG90S | signal (yellow/orange) | GPIO14 | PWM signal. |
| MG90S | VCC (red) | 5V | Servo power. |
| MG90S | GND (brown/black) | GND | Must be common with ESP32 GND. |
| TMB12A05 | `+` | GPIO13 | Active-high digital output. |
| TMB12A05 | `-` | GND | Return path. |
| RGB module | `R` | GPIO12 | PWM channel. |
| RGB module | `G` | GPIO2 | PWM channel. |
| RGB module | `B` | GPIO4 | PWM channel. |
| RGB module | `GND` / `-` | GND | Common ground. |
| MPU module | VCC | 3V3 | Do not power this module from 5V. |
| MPU module | GND | GND | Common ground. |
| MPU module | SDA | GPIO26 | I2C data. |
| MPU module | SCL | GPIO27 | I2C clock. |
| MPU module | ADO | GND | Selects I2C address `0x68`. |
| MPU module | NCS | 3V3 | Keeps the module in I2C mode. |
| ST7789 | SCL | GPIO32 | Software SPI clock. |
| ST7789 | SDA | GPIO33 | Software SPI data line. |
| ST7789 | CS | GPIO25 | Chip-select line. |
| ST7789 | RES | 3V3 | Reset is held high. |
| ST7789 | BLO | 3V3 | Backlight enabled. |
| ST7789 | VCC | 3V3 | Display power. |
| ST7789 | GND | GND | Common ground. |

> The ST7789 module used in this project operates in a three-wire, nine-bit software SPI style. The firmware applies a display offset of `X=0`, `Y=40`, matching this specific 240x240 module.

## Firmware Design

The firmware is intentionally organised with one class per major responsibility:

| File | Responsibility |
|---|---|
| `DoorSensor.h` | Reads MC38 state and detects door-state changes. |
| `BarrierServo.h` | Controls the MG90S locking position. |
| `DoorStatusLed.h` | Controls automatic and manual RGB LED modes using PWM. |
| `AlarmBuzzer.h` | Runs the intermittent buzzer pattern in a FreeRTOS task. |
| `ObjectMotionSensor.h` | Reads, calibrates and evaluates MPU acceleration data over I2C. |
| `VaultDisplay.h` | Draws vault status and blinking theft alerts on the ST7789. |
| `PersistentLog.h` | Keeps the latest 10 commands and 10 events in NVS via `Preferences`. |
| `WiFiVaultServer.h` | Connects to Wi-Fi and serves the dashboard/API on port 80. |
| `WiFiAlertClient.h` | Sends the `OBJECT_STOLEN` HTTP alert to Flask. |
| `main.cpp` | Coordinates component initialisation, rules, display refreshes and event logging. |

### Motion Detection Strategy

At startup, the MPU is calibrated while the object remains still. The firmware stores an initial three-axis acceleration reference. It then samples the sensor every 200 ms and combines:

- orientation change relative to the calibrated position;
- movement between consecutive samples;
- a small accumulated motion score for quick movements.

This approach catches both orientation changes and abrupt horizontal movement while filtering typical sensor noise. The alarm becomes active only after the motion rule is met; it is not tied directly to the door state.

## Web Dashboard

The ESP32 serves the interface directly on port 80. Once it joins the configured Wi-Fi network, open its IP address in a browser, for example `http://192.168.1.50`.

The dashboard provides:

- live `LOCKED` / `UNLOCKED` door state;
- live `SAFE` / `STOLEN` object state;
- `LOCK` / `UNLOCK` remote barrier control;
- MPU health and armed state;
- RGB color picker and `AUTO DOOR` mode;
- the last 10 saved commands;
- the last 10 saved vault events, with individual deletion.

### ESP32 API Endpoints

| Method | Route | Description |
|---|---|---|
| `GET` | `/` | Web dashboard. |
| `GET` | `/api/status` | Current door, object, servo, MPU, logs and dashboard state. |
| `POST` | `/api/barrier?cmd=LOCK` | Locks the barrier. |
| `POST` | `/api/barrier?cmd=UNLOCK` | Unlocks the barrier. |
| `POST` | `/api/rgb?color=%23RRGGBB` | Sets a manual RGB color. |
| `POST` | `/api/rgb/auto` | Returns the RGB module to automatic door-state mode. |
| `POST` | `/api/events/delete?index=N` | Deletes an event from the NVS-backed list. |

## Persistent Storage

Cloudlock uses ESP32 NVS (Non-Volatile Storage) through Arduino's `Preferences` library. Unlike RAM, NVS retains information after resets and power loss.

The project stores up to 10 entries in each category:

- web commands such as `WIFI_LOCK`, `WIFI_UNLOCK`, `WIFI_RGB_COLOR`;
- vault events such as `SYSTEM_BOOT`, `DOOR_OPENED`, `DOOR_CLOSED`, `OBJECT_STOLEN`, and `OBJECT_SAFE_AGAIN`.

## E-mail Alerts

When the object is marked as stolen, the ESP32 performs this flow:

```text
MPU movement -> ESP32 detects OBJECT_STOLEN -> POST /api/wifi-alert -> Flask -> Gmail SMTP
```

`web_app.py` is a minimal Flask service that validates a shared key and sends the e-mail asynchronously. Gmail credentials are read only from environment variables, never from committed source code.

## Prerequisites

- VS Code with the PlatformIO extension.
- ESP32 DevKit connected by USB for firmware upload and serial debugging.
- Python 3.10 or newer for the e-mail gateway.
- A Wi-Fi router or phone hotspot shared by the ESP32 and the device used to open the dashboard.
- A Gmail App Password if e-mail alerts are enabled.

## Setup and Run

### 1. Configure private values

Private Wi-Fi and alert settings are intentionally excluded from Git.

```bash
cp include/Secrets.example.h include/Secrets.h
```

Open `include/Secrets.h` and configure:

- `WIFI_STA_SSID` and `WIFI_STA_PASSWORD` for the router or phone hotspot;
- `FLASK_ALERT_URL` with the IP address of the computer running `web_app.py`;
- `FLASK_ALERT_KEY` with the same private value used by Flask.

Example URL format:

```cpp
const char FLASK_ALERT_URL[] = "http://192.168.1.20:5000/api/wifi-alert";
```

### 2. Upload the ESP32 firmware

Open the `Cloudlock` folder in VS Code. PlatformIO installs the libraries declared in `platformio.ini`:

- `ESP32Servo`
- `Adafruit GFX Library`
- `Adafruit ST7735 and ST7789 Library`

Connect the board, then select **PlatformIO: Upload**. The default serial monitor speed is `115200` baud.

If your board is not available as `/dev/ttyUSB0`, update or remove `upload_port` and `monitor_port` in `platformio.ini` and let PlatformIO choose the correct port.

### 3. Open the dashboard

After boot and MPU calibration, the serial monitor prints the ESP32 Wi-Fi address:

```text
WiFi conectat. IP: 192.168.1.50
WiFi web server pornit pe portul 80.
```

Open `http://192.168.1.50` from a device connected to the same network.

### 4. Start the Flask e-mail gateway

Create the virtual environment and install Flask:

```bash
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
```

Start the service with a Gmail App Password. Do not use your normal Gmail password.

```bash
CLOUDLOCK_MAIL_USERNAME="your.sender@gmail.com" \
CLOUDLOCK_MAIL_PASSWORD="your-16-character-app-password" \
CLOUDLOCK_MAIL_TO="recipient@example.com" \
CLOUDLOCK_ALERT_KEY="the-same-private-key-from-Secrets.h" \
.venv/bin/python web_app.py
```

Verify the service locally at `http://127.0.0.1:5000/api/health`.

## Test Checklist

1. Keep the MPU still during its two-second calibration after boot.
2. Bring the MC38 magnet close: verify `LOCKED`, red LED and TFT status.
3. Separate the magnet: verify `UNLOCKED`, green LED and TFT status.
4. Use the dashboard switch: verify that `LOCK` moves the servo to 0 degrees and `UNLOCK` to 90 degrees.
5. Move the protected object/MPU: verify buzzer beeps, TFT alert, web status, NVS event and e-mail delivery.
6. Return the object to a stable initial position: verify that the buzzer stops and the state becomes safe again.
7. Check the dashboard logs and delete one event to verify NVS log management.

## Repository Safety

Before pushing this project, verify that these files are not staged:

- `include/Secrets.h`
- `.env`
- `.venv/`
- `.pio/`

The repository includes `include/Secrets.example.h` instead, so another developer can configure their own local environment without receiving personal credentials.

## Author

**Mario Stefan Bizau**  
Embedded Systems and Computer Engineering Student

Cloudlock was developed as an academic project and expanded as a practical demonstration of ESP32 firmware design, Wi-Fi control, sensor integration, persistent storage, and e-mail alerting.
