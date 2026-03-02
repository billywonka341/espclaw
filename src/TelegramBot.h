#pragma once
#include <Arduino.h>

class TelegramBot {
public:
  // Initializes the bot with the given API token
  void begin(const String &token);

  // Polls the Telegram API for new messages.
  // Call this inside the loop() frequently.
  void poll();

  // Sends a message to a destination chat ID
  bool sendMessage(const String &chatId, const String &text);

  // Callbacks for incoming messages
  typedef void (*MessageCallback)(const String &chatId, const String &text);
  void onMessage(MessageCallback callback);

private:
  String _token;
  long _lastUpdateId;
  MessageCallback _callback;
};
