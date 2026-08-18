#ifndef VAULT_DISPLAY_H
#define VAULT_DISPLAY_H

#include <Arduino.h>
#include <string.h>

class VaultDisplay {
private:
  int sclPin;
  int sdaPin;
  int csPin;
  int rstPin;
  int blPin;
  int lastMode;
  bool alertVisible;

  static const uint16_t W = 240;
  static const uint16_t H = 240;
  static const uint16_t X_OFFSET = 0;
  static const uint16_t Y_OFFSET = 40;

  static const uint8_t FONT[29][5];

  void clock() {
    digitalWrite(sclPin, HIGH);
    digitalWrite(sclPin, LOW);
  }

  void send9(bool dc, uint8_t value) {
    uint16_t word = (uint16_t(dc) << 8) | value;
    for (int bit = 8; bit >= 0; bit--) {
      digitalWrite(sdaPin, (word >> bit) & 1);
      clock();
    }
  }

  void command(uint8_t value) {
    digitalWrite(csPin, LOW);
    send9(false, value);
    digitalWrite(csPin, HIGH);
  }

  void data(uint8_t value) {
    digitalWrite(csPin, LOW);
    send9(true, value);
    digitalWrite(csPin, HIGH);
  }

  void dataN(const uint8_t *values, size_t length) {
    digitalWrite(csPin, LOW);
    for (size_t i = 0; i < length; i++) {
      send9(true, values[i]);
    }
    digitalWrite(csPin, HIGH);
  }

  void reset() {
    if (rstPin < 0) {
      delay(120);
      return;
    }

    pinMode(rstPin, OUTPUT);
    digitalWrite(rstPin, HIGH);
    delay(10);
    digitalWrite(rstPin, LOW);
    delay(20);
    digitalWrite(rstPin, HIGH);
    delay(120);
  }

  void setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    x0 += X_OFFSET;
    x1 += X_OFFSET;
    y0 += Y_OFFSET;
    y1 += Y_OFFSET;

    command(0x2A);
    uint8_t xData[4] = {uint8_t(x0 >> 8), uint8_t(x0), uint8_t(x1 >> 8), uint8_t(x1)};
    dataN(xData, 4);

    command(0x2B);
    uint8_t yData[4] = {uint8_t(y0 >> 8), uint8_t(y0), uint8_t(y1 >> 8), uint8_t(y1)};
    dataN(yData, 4);

    command(0x2C);
  }

  int fontIndex(char ch) {
    if (ch >= 'A' && ch <= 'Z') {
      return ch - 'A';
    }
    if (ch == '!') {
      return 26;
    }
    if (ch == '-') {
      return 27;
    }
    return 28;
  }

  void drawPixel(int x, int y, uint16_t color) {
    if (x < 0 || y < 0 || x >= W || y >= H) {
      return;
    }

    setWindow(x, y, x, y);
    digitalWrite(csPin, LOW);
    send9(true, color >> 8);
    send9(true, color & 0xFF);
    digitalWrite(csPin, HIGH);
  }

  void drawChar(int x, int y, char ch, uint16_t color, uint8_t scale) {
    int index = fontIndex(ch);
    for (int col = 0; col < 5; col++) {
      uint8_t bits = FONT[index][col];
      for (int row = 0; row < 7; row++) {
        if (bits & (1 << row)) {
          for (int dx = 0; dx < scale; dx++) {
            for (int dy = 0; dy < scale; dy++) {
              drawPixel(x + col * scale + dx, y + row * scale + dy, color);
            }
          }
        }
      }
    }
  }

  void drawTextCentered(const char *text, int y, uint16_t color, uint8_t scale) {
    int charWidth = 6 * scale;
    int x = (W - (int)strlen(text) * charWidth) / 2;
    for (int i = 0; text[i]; i++) {
      drawChar(x + i * charWidth, y, text[i], color, scale);
    }
  }

  void fillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
    setWindow(x, y, x + width - 1, y + height - 1);

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    digitalWrite(csPin, LOW);
    for (uint32_t i = 0; i < (uint32_t)width * height; i++) {
      send9(true, hi);
      send9(true, lo);
    }
    digitalWrite(csPin, HIGH);
  }

  void fillScreen(uint16_t color) {
    fillRect(0, 0, W, H, color);
  }

  void drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
    fillRect(x, y, width, 2, color);
    fillRect(x, y + height - 2, width, 2, color);
    fillRect(x, y, 2, height, color);
    fillRect(x + width - 2, y, 2, height, color);
  }

  void drawSafe(bool doorOpen) {
    uint16_t color = doorOpen ? 0x07E0 : 0xF800;

    fillScreen(0x0000);
    drawRect(4, 4, W - 8, H - 8, color);
    drawTextCentered(doorOpen ? "UNLOCKED" : "LOCKED", doorOpen ? 52 : 48, color, doorOpen ? 3 : 4);
    drawTextCentered("THE OBJECT", 120, 0xFFFF, 2);
    drawTextCentered("IS SAFE", 150, 0xFFFF, 2);
  }

  void drawAlert(bool doorOpen, bool visible) {
    fillScreen(visible ? 0x0000 : 0xF800);
    if (!visible) {
      return;
    }

    drawRect(4, 4, W - 8, H - 8, 0xF800);
    drawTextCentered(doorOpen ? "UNLOCKED" : "LOCKED", doorOpen ? 32 : 36, 0xF800, doorOpen ? 3 : 4);
    drawTextCentered("ALERT!", 76, 0xF800, 3);
    drawTextCentered("THE OBJECT", 124, 0xFFFF, 2);
    drawTextCentered("HAS BEEN", 154, 0xFFFF, 2);
    drawTextCentered("STOLEN!", 184, 0xFFFF, 2);
  }

public:
  VaultDisplay(int scl, int sda, int cs, int rst, int bl) {
    sclPin = scl;
    sdaPin = sda;
    csPin = cs;
    rstPin = rst;
    blPin = bl;
    lastMode = -1;
    alertVisible = true;
  }

  void begin() {
    pinMode(csPin, OUTPUT);
    pinMode(sclPin, OUTPUT);
    pinMode(sdaPin, OUTPUT);

    digitalWrite(csPin, HIGH);
    digitalWrite(sclPin, LOW);

    if (blPin >= 0) {
      pinMode(blPin, OUTPUT);
      digitalWrite(blPin, HIGH);
    }

    reset();
    command(0x01);
    delay(150);
    command(0x11);
    delay(120);
    command(0x3A);
    data(0x55);
    command(0x36);
    data(0x00);
    command(0x29);
    delay(50);
  }

  void show(bool doorOpen, bool objectStolen, bool force = false) {
    int mode = objectStolen ? 2 : (doorOpen ? 1 : 0);
    if (!force && mode == lastMode) {
      return;
    }

    lastMode = mode;
    alertVisible = true;

    if (objectStolen) {
      drawAlert(doorOpen, alertVisible);
    } else {
      drawSafe(doorOpen);
    }
  }

  void blinkAlert(bool doorOpen) {
    alertVisible = !alertVisible;
    drawAlert(doorOpen, alertVisible);
  }

  bool isAlertMode() {
    return lastMode == 2;
  }
};

const uint8_t VaultDisplay::FONT[29][5] = {
  {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36},
  {0x3E, 0x41, 0x41, 0x41, 0x22}, {0x7F, 0x41, 0x41, 0x22, 0x1C},
  {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
  {0x3E, 0x41, 0x49, 0x49, 0x7A}, {0x7F, 0x08, 0x08, 0x08, 0x7F},
  {0x00, 0x41, 0x7F, 0x41, 0x00}, {0x20, 0x40, 0x41, 0x3F, 0x01},
  {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
  {0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F},
  {0x3E, 0x41, 0x41, 0x41, 0x3E}, {0x7F, 0x09, 0x09, 0x09, 0x06},
  {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
  {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01},
  {0x3F, 0x40, 0x40, 0x40, 0x3F}, {0x1F, 0x20, 0x40, 0x20, 0x1F},
  {0x3F, 0x40, 0x38, 0x40, 0x3F}, {0x63, 0x14, 0x08, 0x14, 0x63},
  {0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43},
  {0x00, 0x00, 0x5F, 0x00, 0x00}, {0x08, 0x08, 0x08, 0x08, 0x08},
  {0x00, 0x00, 0x00, 0x00, 0x00}
};

#endif
