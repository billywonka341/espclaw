#include "LLMClient.h"
#include "TelegramBot.h"
#include "config.h"
#include "version.h"
#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include "ConfigManager.h"
#include "WebUI.h"
#include <WiFi.h>
#endif

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
      Serial.printf("TOOL: Toggled GPIO %d ON\n", pin);
    } else if (command.startsWith("GPIO_OFF:")) {
      int pin = command.substring(9).toInt();
      pinMode(pin, OUTPUT);
      digitalWrite(pin, LOW);
      Serial.printf("TOOL: Toggled GPIO %d OFF\n", pin);
    }

    index = endIndex + 1;
  }
}

void onTelegramMessage(const String &chatId, const String &text) {
  Serial.printf("Message from %s: %s\n", chatId.c_str(), text.c_str());

  String allowed = String(TELEGRAM_ALLOWED_CHAT_ID);
  if (allowed != "YOUR_CHAT_ID" && allowed != "" && chatId != allowed) {
    bot.sendMessage(chatId, "Unauthorized user.");
    Serial.println("Blocked unauthorized user.");
    return;
  }

  bot.sendMessage(chatId, "Let me think...");
  String llmReply = LLMClient::ask(text);
  executeTools(llmReply);
  bot.sendMessage(chatId, llmReply);
}

#if defined(ESP32)
void onWebChatMessage(const String &message, String &response) {
  Serial.printf("Web Chat Message: %s\n", message.c_str());
  response = LLMClient::ask(message);
  executeTools(response);
}
#endif

void setup() {
  Serial.begin(115200);
  Serial.printf("\n\n--- espclaw v%s ---\n", VERSION);

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
    Serial.println("\nFailed to connect. Starting AP mode for configuration.");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESPClaw-Config", "12345678");
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
  }

  // Initialize Web UI
  initWebUI(onWebChatMessage);
#else
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
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
    Serial.println("\nTime synced.");

    bot.begin(TELEGRAM_BOT_TOKEN);
    bot.onMessage(onTelegramMessage);
  }

  Serial.println("System initialized. Ready to chat!");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (millis() - lastPollTime > POLLING_INTERVAL_MS) {
      bot.poll();
      lastPollTime = millis();
    }
  }
}
