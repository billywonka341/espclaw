#pragma once

#if defined(ESP32)
#include <Arduino.h>
#include <ESPAsyncWebServer.h>

extern AsyncWebServer server;

typedef void (*WebChatCallback)(const String &message, String &response);

void initWebUI(WebChatCallback chatCallback);
void loopWebUI();

#endif
