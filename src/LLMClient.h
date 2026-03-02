#pragma once
#include <Arduino.h>

class LLMClient {
public:
  // Sends a message to the LLM and returns the text response
  static String ask(const String &userMessage);
};
