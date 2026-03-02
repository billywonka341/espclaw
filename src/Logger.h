#pragma once

#include <Arduino.h>
#include <vector>

class Logger {
public:
  Logger();

  // Logs a simple string
  void log(const String &msg);

  // Formats and logs a string
  void logf(const char *format, ...);

  // Returns all logs separated by newlines
  String getLogs();

  // Clear all logs
  void clear();

private:
  std::vector<String> logLines;
  const size_t maxLines = 50; // Keep memory usage bounded
};

extern Logger appLogger;
