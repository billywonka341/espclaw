#pragma once
#include <Arduino.h>

class LLMClient {
public:
  // Main routing function
  static String ask(const String &userMessage);

private:
  // Provider-specific callers
  static String askOpenAI(const String &userMessage);
  static String askAnthropic(const String &userMessage);
  static String askGemini(const String &userMessage);

  // Helper method to gather and log HA instructions
  static String getHAInstructions();
};
