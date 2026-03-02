#include "Logger.h"

Logger appLogger;

Logger::Logger() { logLines.reserve(maxLines); }

void Logger::log(const String &msg) {
  Serial.println(msg); // Keep console output

  if (logLines.size() >= maxLines) {
    logLines.erase(logLines.begin());
  }
  logLines.push_back(msg);
}

void Logger::logf(const char *format, ...) {
  char buf[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  log(String(buf));
}

String Logger::getLogs() {
  String out;
  for (const String &s : logLines) {
    out += s + "\n";
  }
  return out;
}

void Logger::clear() { logLines.clear(); }
