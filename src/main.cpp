#include "LLMClient.h"
#include "TelegramBot.h"
#include "config.h"
#include "version.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#if defined(ESP8266)
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include "ConfigManager.h"
#include "WebUI.h"
#include <HTTPClient.h>
#include <WiFi.h>
#endif

#include "Logger.h"
#include <time.h>

TelegramBot bot;
unsigned long lastPollTime = 0;

// Simple tool parser for our agent
void executeTools(const String &llmResponse) {
  int index = 0;
  while ((index = llmResponse.indexOf('[', index)) != -1) {
    int endIndex = llmResponse.indexOf(']', index);
    if (endIndex == -1)
      break;

    String command = llmResponse.substring(index + 1, endIndex);
    command.trim();

    if (command.startsWith("GPIO_ON:")) {
      int pin = command.substring(8).toInt();
      pinMode(pin, OUTPUT);
      digitalWrite(pin, HIGH);
      appLogger.logf("TOOL: Toggled GPIO %d ON\n", pin);
    } else if (command.startsWith("GPIO_OFF:")) {
      int pin = command.substring(9).toInt();
      pinMode(pin, OUTPUT);
      digitalWrite(pin, LOW);
      appLogger.logf("TOOL: Toggled GPIO %d OFF\n", pin);
    } else if (command.startsWith("HA_CALL:")) {
      int haIndex = command.substring(8).toInt();
      appLogger.logf("TOOL: Triggered Home Assistant Call index %d\n", haIndex);

#if defined(ESP32)
      String urlsStr = configManager.getHaUrls();
      String token = configManager.getHaToken();
#else
      String urlsStr = HA_URLS;
      String token = HA_TOKEN;
#endif

      JsonDocument doc;
      if (!deserializeJson(doc, urlsStr) && doc.is<JsonArray>()) {
        JsonArray arr = doc.as<JsonArray>();
        if (haIndex >= 0 && (size_t)haIndex < arr.size()) {
          String targetUrl = arr[haIndex]["url"].as<String>();
          appLogger.log("Executing HA POST to: " + targetUrl);

          WiFiClient client;
          HTTPClient http;
          if (http.begin(client, targetUrl)) {
            http.addHeader("Content-Type", "application/json");
            if (token.length() > 0) {
              http.addHeader("Authorization", "Bearer " + token);
            }
            int httpCode =
                http.POST("{}"); // empty JSON required for many HA calls
            appLogger.logf("HA Response Code: %d\n", httpCode);
            if (httpCode > 0) {
              String payload = http.getString();
              appLogger.log("HA Response: " + payload);
            }
            http.end();
          } else {
            appLogger.log("Failed to begin HTTP client to HA");
          }
        } else {
          appLogger.log("Invalid HA index requested by LLM.");
        }
      }
    }

    index = endIndex + 1;
  }
}

void onTelegramMessage(const String &chatId, const String &text) {
  appLogger.logf("Message from %s: %s\n", chatId.c_str(), text.c_str());

  String allowed = String(TELEGRAM_ALLOWED_CHAT_ID);
  if (allowed != "YOUR_CHAT_ID" && allowed != "" && chatId != allowed) {
    bot.sendMessage(chatId, "Unauthorized user.");
    appLogger.log("Blocked unauthorized user.");
    return;
  }

  String cmdText = text;
  cmdText.trim();

#if defined(ESP32)
  bool multiEsp = configManager.getMultiEspEnabled();
  String devId = configManager.getDeviceId();

  if (multiEsp) {
    String prefix = "/" + devId;
    if (cmdText.startsWith(prefix + " ")) {
      cmdText = cmdText.substring(prefix.length() + 1);
      cmdText.trim();
    } else {
      if (cmdText != "/multiespclaw") {
        return; // Ignore if not addressed to this device
      }
    }
  }
#endif

  if (cmdText == "/setup") {
#if defined(ESP32)
    String prov = configManager.getLlmProvider();
    String mod = configManager.getLlmModel();
#else
    String prov = String(LLM_PROVIDER);
    String mod = String(LLM_MODEL);
#endif
    String setupMsg = "Device Configuration:\n";
    setupMsg += "- Provider: " + prov + "\n";
    setupMsg += "- Model: " + mod + "\n";
    setupMsg += "\nNote: When configuring via chat, text has to be exact.";
    bot.sendMessage(chatId, setupMsg);
    return;
  }

  if (cmdText == "/multiespclaw") {
#if defined(ESP32)
    int r = random(1000, 10000); // 1000 to 9999
    String newId = "esp" + String(r);
    configManager.setMultiEspConfig(true, newId);
    bot.sendMessage(chatId,
                    "Switched to Multi-ESP mode. New Device ID: " + newId);
#else
    bot.sendMessage(chatId, "Multi-ESP mode is only supported on ESP32.");
#endif
    return;
  }

  if (cmdText == "/gpiodescription") {
#if defined(ESP32)
    bot.sendMessage(chatId, "Current GPIO Description:\n" +
                                configManager.getUserPins());
#else
    bot.sendMessage(chatId, "Current GPIO Description:\n" + String(USER_PINS) +
                                "\n(Change via config.h on ESP8266)");
#endif
    return;
  }

  if (cmdText.startsWith("/changegpiodescription ")) {
#if defined(ESP32)
    String newPins =
        cmdText.substring(23); // length of "/changegpiodescription "
    newPins.trim();
    configManager.setUserPins(newPins);
    bot.sendMessage(
        chatId,
        "GPIO description updated successfully.\nNew Description:\n" + newPins);
#else
    bot.sendMessage(
        chatId,
        "Changing GPIO description dynamically is only supported on ESP32.");
#endif
    return;
  }

#if defined(ESP32)
  if (cmdText.startsWith("/") && cmdText.indexOf(" ") == -1 && multiEsp) {
    String newId = cmdText.substring(1);
    configManager.setMultiEspConfig(true, newId);
    bot.sendMessage(chatId, "Device ID successfully changed to: " + newId);
    return;
  }
#endif

  bot.sendMessage(chatId, "Let me think...");
  String llmReply = LLMClient::ask(cmdText);
  executeTools(llmReply);
  bot.sendMessage(chatId, llmReply);
}

#if defined(ESP32)
void onWebChatMessage(const String &message, String &response) {
  appLogger.logf("Web Chat Message: %s\n", message.c_str());

  if (message.startsWith("/gpiodescription")) {
    response =
        "Currently saved hardware definition: \n" + configManager.getUserPins();
    return;
  }

  if (message.startsWith("/changegpiodescription ")) {
    String newPins =
        message.substring(String("/changegpiodescription ").length());
    newPins.trim();
    configManager.setUserPins(newPins);
    response = "Hardware definition successfully updated to: \n" + newPins;
    return;
  }

  if (message.startsWith("/setup") || message.startsWith("/multiespclaw")) {
    response =
        "This command shouldn't be used typically on the WebUI. Use Telegram!";
    return;
  }

  response = LLMClient::ask(message);
  executeTools(response);
}
#endif

void setup() {
  Serial.begin(115200);
  appLogger.logf("\n\n--- espclaw v%s ---\n", VERSION);

#if defined(ESP32)
  configManager.begin();
#endif

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

#if defined(ESP32)
  if (WiFi.status() != WL_CONNECTED) {
    appLogger.log("\nFailed to connect. Starting AP mode for configuration.");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESPClaw-Config", "12345678");
    appLogger.log("AP IP: " + WiFi.softAPIP().toString());
  } else {
    appLogger.log("\nConnected! IP: " + WiFi.localIP().toString());
  }

  // Initialize Web UI
  initWebUI(onWebChatMessage);
#else
  if (WiFi.status() == WL_CONNECTED) {
    appLogger.log("\nConnected! IP: " + WiFi.localIP().toString());
  }
#endif

  if (WiFi.status() == WL_CONNECTED) {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.print("Waiting for NTP time sync: ");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
      delay(500);
      Serial.print(".");
      now = time(nullptr);
    }
    appLogger.log("\nTime synced.");

    bot.begin(TELEGRAM_BOT_TOKEN);
    bot.onMessage(onTelegramMessage);
  }

  appLogger.log("System initialized. Ready to chat!");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (millis() - lastPollTime > POLLING_INTERVAL_MS) {
      bot.poll();
      lastPollTime = millis();
    }
  }

#if defined(ESP32)
  loopWebUI();
#endif
}
