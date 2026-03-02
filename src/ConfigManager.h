#pragma once

#if defined(ESP32)
#include <Arduino.h>
#include <Preferences.h>

class ConfigManager {
public:
  ConfigManager();
  void begin();

  // Getters
  String getWifiSsid();
  String getWifiPassword();
  String getTelegramBotToken();
  String getTelegramChatId();
  String getOpenAiApiKey();
  String getLlmProvider();
  String getLlmModel();
  String getSystemPrompt();

  // Setters
  void saveWifiConfig(const String &ssid, const String &password);
  void saveTelegramConfig(const String &botToken, const String &chatId);
  void saveLlmConfig(const String &apiKey, const String &provider,
                     const String &model, const String &prompt);

private:
  Preferences preferences;

  String wifiSsid;
  String wifiPassword;
  String telegramBotToken;
  String telegramChatId;
  String openAiApiKey;
  String llmProvider;
  String llmModel;
  String systemPrompt;

  void loadAll();
};

extern ConfigManager configManager;
#endif
