#ifndef PERSISTENT_LOG_H
#define PERSISTENT_LOG_H

#include <Arduino.h>
#include <Preferences.h>

class PersistentLog {
private:
  static const int MAX_ITEMS = 10;

  Preferences prefs;

  String keyFor(const char *prefix, int index) {
    return String(prefix) + String(index);
  }

  int getCount(const char *countKey) {
    int count = prefs.getInt(countKey, 0);
    if (count < 0) {
      return 0;
    }
    if (count > MAX_ITEMS) {
      return MAX_ITEMS;
    }
    return count;
  }

  void addItem(const char *prefix, const char *countKey, const String &value) {
    int count = getCount(countKey);

    for (int i = MAX_ITEMS - 1; i > 0; i--) {
      String previous = prefs.getString(keyFor(prefix, i - 1).c_str(), "");
      if (previous.length() > 0) {
        prefs.putString(keyFor(prefix, i).c_str(), previous);
      }
    }

    prefs.putString(keyFor(prefix, 0).c_str(), value);
    if (count < MAX_ITEMS) {
      count++;
    }
    prefs.putInt(countKey, count);
  }

  void printJsonString(const String &value) {
    Serial.print("\"");
    for (int i = 0; i < value.length(); i++) {
      char ch = value[i];
      if (ch == '"' || ch == '\\') {
        Serial.print("\\");
      }
      Serial.print(ch);
    }
    Serial.print("\"");
  }

  void printJsonArray(const char *prefix, const char *countKey) {
    int count = getCount(countKey);
    Serial.print("[");

    for (int i = 0; i < count; i++) {
      if (i > 0) {
        Serial.print(",");
      }
      printJsonString(prefs.getString(keyFor(prefix, i).c_str(), ""));
    }

    Serial.print("]");
  }

  String jsonString(const String &value) {
    String result = "\"";
    for (int i = 0; i < value.length(); i++) {
      char ch = value[i];
      if (ch == '"' || ch == '\\') {
        result += "\\";
      }
      result += ch;
    }
    result += "\"";
    return result;
  }

  String jsonArray(const char *prefix, const char *countKey) {
    int count = getCount(countKey);
    String result = "[";

    for (int i = 0; i < count; i++) {
      if (i > 0) {
        result += ",";
      }
      result += jsonString(prefs.getString(keyFor(prefix, i).c_str(), ""));
    }

    result += "]";
    return result;
  }

public:
  void begin() {
    prefs.begin("cloudlock", false);
  }

  void addCommand(const String &command) {
    addItem("cmd", "cmdCount", command);
  }

  void addEvent(const String &event) {
    addItem("evt", "evtCount", event);
  }

  void deleteEvent(int index) {
    int count = getCount("evtCount");
    if (index < 0 || index >= count) {
      return;
    }

    for (int i = index; i < count - 1; i++) {
      String next = prefs.getString(keyFor("evt", i + 1).c_str(), "");
      prefs.putString(keyFor("evt", i).c_str(), next);
    }

    prefs.remove(keyFor("evt", count - 1).c_str());
    prefs.putInt("evtCount", count - 1);
  }

  void printCommandsJson() {
    printJsonArray("cmd", "cmdCount");
  }

  void printEventsJson() {
    printJsonArray("evt", "evtCount");
  }

  String commandsJson() {
    return jsonArray("cmd", "cmdCount");
  }

  String eventsJson() {
    return jsonArray("evt", "evtCount");
  }
};

#endif
