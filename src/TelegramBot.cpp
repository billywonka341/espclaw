#include "TelegramBot.h"
#include <ArduinoJson.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

#include <WiFiClientSecure.h>
void TelegramBot::begin(const String &token) {
  _token = token;
  _lastUpdateId = 0;
  _callback = nullptr;
}

void TelegramBot::onMessage(MessageCallback callback) { _callback = callback; }

void TelegramBot::poll() {
  if (!_callback)
    return;

  WiFiClientSecure client;
  client.setInsecure(); // Required for ESP8266 memory constraints
#if defined(ESP8266)
  client.setBufferSizes(2048, 512);
#endif

  if (!client.connect("api.telegram.org", 443)) {
    return;
  }

  String url = "/bot" + _token +
               "/getUpdates?offset=" + String(_lastUpdateId + 1) +
               "&limit=1&timeout=2";

  client.println("GET " + url + " HTTP/1.1");
  client.println("Host: api.telegram.org");
  client.println("Connection: close");
  client.println();

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }

  // Filter to severely reduce RAM consumption when parsing the JSON tree
  JsonDocument filter;
  filter["ok"] = true;
  filter["result"][0]["update_id"] = true;
  filter["result"][0]["message"]["chat"]["id"] = true;
  filter["result"][0]["message"]["text"] = true;

  JsonDocument doc;
  DeserializationError error =
      deserializeJson(doc, client, DeserializationOption::Filter(filter));

  if (error) {
    return; // No updates or parsing failed
  }

  if (!doc["ok"])
    return;

  JsonArray result = doc["result"];
  if (result.size() > 0) {
    JsonObject update = result[0];
    long updateId = update["update_id"];
    String chatId = update["message"]["chat"]["id"].as<String>();
    const char *text = update["message"]["text"];

    if (updateId > _lastUpdateId) {
      _lastUpdateId = updateId;
    }

    if (text && _callback) {
      _callback(chatId, String(text));
    }
  }
}

bool TelegramBot::sendMessage(const String &chatId, const String &text) {
  WiFiClientSecure client;
  client.setInsecure();
#if defined(ESP8266)
  client.setBufferSizes(512, 1024);
#endif

  if (!client.connect("api.telegram.org", 443)) {
    return false;
  }

  JsonDocument doc;
  doc["chat_id"] = chatId;
  doc["text"] = text;

  String payload;
  serializeJson(doc, payload);

  String url = "/bot" + _token + "/sendMessage";

  client.println("POST " + url + " HTTP/1.1");
  client.println("Host: api.telegram.org");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.print("Content-Length: ");
  client.println(payload.length());
  client.println();
  client.print(payload);

  return true;
}
