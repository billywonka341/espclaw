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
  String provider = String(LLM_PROVIDER);
  provider.toLowerCase();

  if (provider == "anthropic") {
    return askAnthropic(userMessage);
  } else if (provider == "gemini") {
    return askGemini(userMessage);
  } else {
    return askOpenAI(userMessage);
  }
}

String LLMClient::askOpenAI(const String &userMessage) {
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

String LLMClient::askAnthropic(const String &userMessage) {
  WiFiClientSecure client;
  client.setInsecure();

#if defined(ESP8266)
  client.setBufferSizes(2048, 1024);
#endif

  Serial.println("LLM: Connecting to api.anthropic.com...");
  if (!client.connect("api.anthropic.com", 443)) {
    Serial.println("LLM: Connection failed!");
    return "Error: Could not connect to Anthropic API.";
  }

  // Prepare JSON payload
  JsonDocument requestDoc;
  requestDoc["model"] = LLM_MODEL;
  requestDoc["max_tokens"] = 1024;
  requestDoc["system"] = SYSTEM_PROMPT;

  JsonArray messages = requestDoc["messages"].to<JsonArray>();

  JsonObject userMsg = messages.add<JsonObject>();
  userMsg["role"] = "user";
  userMsg["content"] = userMessage;

  String requestBody;
  serializeJson(requestDoc, requestBody);

  client.println("POST /v1/messages HTTP/1.1");
  client.println("Host: api.anthropic.com");
  client.println("Connection: close");
  client.println("Content-Type: application/json");
  client.print("x-api-key: ");
  client.println(OPENAI_API_KEY); // Reusing the same API key var
  client.println("anthropic-version: 2023-06-01");
  client.print("Content-Length: ");
  client.println(requestBody.length());
  client.println();
  client.print(requestBody);

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }

  // Minimal filter
  JsonDocument filter;
  filter["content"][0]["text"] = true;

  JsonDocument responseDoc;
  DeserializationError error = deserializeJson(
      responseDoc, client, DeserializationOption::Filter(filter));

  if (error) {
    Serial.print("LLM (Anthropic): JSON Parse Error: ");
    Serial.println(error.c_str());
    return String("Error parsing LLM response: ") + error.c_str();
  }

  const char *content = responseDoc["content"][0]["text"];
  if (content) {
    return String(content);
  }

  return "Error: Unexpected LLM response format.";
}

String LLMClient::askGemini(const String &userMessage) {
  WiFiClientSecure client;
  client.setInsecure();

#if defined(ESP8266)
  client.setBufferSizes(2048, 1024);
#endif

  Serial.println("LLM: Connecting to generativelanguage.googleapis.com...");
  if (!client.connect("generativelanguage.googleapis.com", 443)) {
    Serial.println("LLM: Connection failed!");
    return "Error: Could not connect to Gemini API.";
  }

  // Prepare JSON payload
  JsonDocument requestDoc;

  JsonObject systemInstruction =
      requestDoc["systemInstruction"].to<JsonObject>();
  JsonObject sysParts =
      systemInstruction["parts"].to<JsonArray>().add<JsonObject>();
  sysParts["text"] = SYSTEM_PROMPT;

  JsonArray contents = requestDoc["contents"].to<JsonArray>();
  JsonObject userContent = contents.add<JsonObject>();
  userContent["role"] = "user";
  JsonObject userParts = userContent["parts"].to<JsonArray>().add<JsonObject>();
  userParts["text"] = userMessage;

  String requestBody;
  serializeJson(requestDoc, requestBody);

  String url = String("/v1beta/models/") + LLM_MODEL +
               ":generateContent?key=" + OPENAI_API_KEY;

  client.print("POST ");
  client.print(url);
  client.println(" HTTP/1.1");
  client.println("Host: generativelanguage.googleapis.com");
  client.println("Connection: close");
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(requestBody.length());
  client.println();
  client.print(requestBody);

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }

  // Minimal filter
  JsonDocument filter;
  filter["candidates"][0]["content"]["parts"][0]["text"] = true;

  JsonDocument responseDoc;
  DeserializationError error = deserializeJson(
      responseDoc, client, DeserializationOption::Filter(filter));

  if (error) {
    Serial.print("LLM (Gemini): JSON Parse Error: ");
    Serial.println(error.c_str());
    return String("Error parsing LLM response: ") + error.c_str();
  }

  const char *content =
      responseDoc["candidates"][0]["content"]["parts"][0]["text"];
  if (content) {
    return String(content);
  }

  return "Error: Unexpected LLM response format.";
}
