#if defined(ESP32)
#include "ConfigManager.h"

ConfigManager configManager;

ConfigManager::ConfigManager() {}

void ConfigManager::begin() {
  preferences.begin("espclaw", false);
  loadAll();
}

void ConfigManager::loadAll() {
  wifiSsid = preferences.getString("wifiSsid", "YOUR_WIFI_SSID");
  wifiPassword = preferences.getString("wifiPass", "YOUR_WIFI_PASSWORD");
  telegramBotToken =
      preferences.getString("tgToken", "YOUR_TELEGRAM_BOT_TOKEN");
  telegramChatId = preferences.getString("tgChat", "YOUR_CHAT_ID");
  openAiApiKey = preferences.getString("aiKey", "YOUR_OPENAI_API_KEY");
  llmProvider = preferences.getString("llmProv", "openai");
  llmModel = preferences.getString("llmMode", "gpt-3.5-turbo");
  systemPrompt = preferences.getString(
      "sysPrompt",
      "You are an AI assistant running on an ESP32 microcontroller. You have "
      "limited memory but can control hardware devices.\n\nYou can control "
      "GPIO pins by outputting specific tool commands within your response in "
      "square brackets. For example:\n- To turn ON GPIO 4: [GPIO_ON: 4]\n- To "
      "turn OFF GPIO 4: [GPIO_OFF: 4]\n\nYou can explicitly chat with the user "
      "and answer general questions about any topic. Keep your responses "
      "and helpful.");
  userPins = preferences.getString("usrPins", "");
  multiEspEnabled = preferences.getBool("multiEsp", false);
  deviceId = preferences.getString("deviceId", "");
  haToken = preferences.getString("haToken", "");
  haUrls = preferences.getString("haUrls", "[]");
}

String ConfigManager::getWifiSsid() { return wifiSsid; }
String ConfigManager::getWifiPassword() { return wifiPassword; }
String ConfigManager::getTelegramBotToken() { return telegramBotToken; }
String ConfigManager::getTelegramChatId() { return telegramChatId; }
String ConfigManager::getOpenAiApiKey() { return openAiApiKey; }
String ConfigManager::getLlmProvider() { return llmProvider; }
String ConfigManager::getLlmModel() { return llmModel; }
String ConfigManager::getSystemPrompt() { return systemPrompt; }
String ConfigManager::getUserPins() { return userPins; }
bool ConfigManager::getMultiEspEnabled() { return multiEspEnabled; }
String ConfigManager::getDeviceId() { return deviceId; }
String ConfigManager::getHaToken() { return haToken; }
String ConfigManager::getHaUrls() { return haUrls; }

void ConfigManager::setUserPins(const String &pins) {
  userPins = pins;
  preferences.putString("usrPins", userPins);
}

void ConfigManager::setMultiEspConfig(bool enabled, const String &devId) {
  multiEspEnabled = enabled;
  deviceId = devId;
  preferences.putBool("multiEsp", multiEspEnabled);
  preferences.putString("deviceId", deviceId);
}

void ConfigManager::saveWifiConfig(const String &ssid, const String &password) {
  wifiSsid = ssid;
  wifiPassword = password;
  preferences.putString("wifiSsid", wifiSsid);
  preferences.putString("wifiPass", wifiPassword);
}

void ConfigManager::saveTelegramConfig(const String &botToken,
                                       const String &chatId) {
  telegramBotToken = botToken;
  telegramChatId = chatId;
  preferences.putString("tgToken", telegramBotToken);
  preferences.putString("tgChat", telegramChatId);
}

void ConfigManager::saveLlmConfig(const String &apiKey, const String &provider,
                                  const String &model, const String &prompt,
                                  const String &pins) {
  openAiApiKey = apiKey;
  llmProvider = provider;
  llmModel = model;
  systemPrompt = prompt;
  userPins = pins;
  preferences.putString("aiKey", openAiApiKey);
  preferences.putString("llmProv", llmProvider);
  preferences.putString("llmMode", llmModel);
  preferences.putString("sysPrompt", systemPrompt);
  preferences.putString("usrPins", userPins);
}

void ConfigManager::saveHaConfig(const String &token, const String &urls) {
  haToken = token;
  haUrls = urls;
  preferences.putString("haToken", haToken);
  preferences.putString("haUrls", haUrls);
}

#endif
