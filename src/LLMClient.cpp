#include "LLMClient.h"
#include "config.h"
#include <ArduinoJson.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

#include <WiFiClientSecure.h>
String LLMClient::ask(const String &userMessage) {
  WiFiClientSecure client;
  // Skip certificate validation to save significant memory and connection time.
  // For a production device with more RAM (ESP32), verify the CA cert here!
  client.setInsecure();

  // Set buffer sizes to minimize RAM usage on ESP8266.
#if defined(ESP8266)
  client.setBufferSizes(2048, 1024);
#endif

  Serial.println("LLM: Connecting to api.openai.com...");
  if (!client.connect("api.openai.com", 443)) {
    Serial.println("LLM: Connection failed!");
    return "Error: Could not connect to OpenAI API.";
  }

  // Prepare JSON payload
  JsonDocument requestDoc;
  requestDoc["model"] = LLM_MODEL;

  JsonArray messages = requestDoc["messages"].to<JsonArray>();

  JsonObject systemMessage = messages.add<JsonObject>();
  systemMessage["role"] = "system";
  systemMessage["content"] = SYSTEM_PROMPT;

  JsonObject userMsg = messages.add<JsonObject>();
  userMsg["role"] = "user";
  userMsg["content"] = userMessage;

  String requestBody;
  serializeJson(requestDoc, requestBody);

  // Send HTTP POST Request Manually
  client.println("POST /v1/chat/completions HTTP/1.1");
  client.println("Host: api.openai.com");
  client.println("Connection: close");
  client.println("Content-Type: application/json");
  client.print("Authorization: Bearer ");
  client.println(OPENAI_API_KEY);
  client.print("Content-Length: ");
  client.println(requestBody.length());
  client.println();
  client.print(requestBody);

  // Wait for response and skip headers
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      // End of headers
      break;
    }
  }

  // Create a filter to ONLY Keep the assistant's message content in memory and
  // discard the rest of the JSON. This is CRUCIAL for ESP8266 to avoid
  // Out-Of-Memory (OOM) crashes.
  JsonDocument filter;
  filter["choices"][0]["message"]["content"] = true;

  JsonDocument responseDoc;
  DeserializationError error = deserializeJson(
      responseDoc, client, DeserializationOption::Filter(filter));

  if (error) {
    Serial.print("LLM: JSON Parse Error: ");
    Serial.println(error.c_str());
    return String("Error parsing LLM response: ") + error.c_str();
  }

  const char *content = responseDoc["choices"][0]["message"]["content"];
  if (content) {
    return String(content);
  }

  return "Error: Unexpected LLM response format.";
}
