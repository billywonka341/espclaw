#include "LLMClient.h"
#include "Logger.h"
#include "config.h"
#include <ArduinoJson.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

#include <WiFiClientSecure.h>

#ifndef BOARD_INFO
#define BOARD_INFO "Generic ESP Board"
#endif

#ifndef USER_PINS
#if defined(ESP32)
#define USER_PINS configManager.getUserPins().c_str()
#else
#define USER_PINS ""
#endif
#endif

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

  appLogger.log("LLM: Connecting to api.openai.com...");
  if (!client.connect("api.openai.com", 443)) {
    appLogger.log("LLM: Connection failed!");
    return "Error: Could not connect to OpenAI API.";
  }

  // Prepare JSON payload
  JsonDocument requestDoc;
  requestDoc["model"] = LLM_MODEL;

  JsonArray messages = requestDoc["messages"].to<JsonArray>();

  String finalPrompt = String(SYSTEM_PROMPT) +
                       "\n\nHardware Context: You are running on " +
                       String(BOARD_INFO) +
                       ". When controlling pins, ALWAYS use the integer "
                       "value inside the bracket tool! e.g., [GPIO_ON: 2]."
                       "\n\nUser Hardware Setup:\n" +
                       String(USER_PINS);

  JsonObject systemMessage = messages.add<JsonObject>();
  systemMessage["role"] = "system";
  systemMessage["content"] = finalPrompt;

  JsonObject userMsg = messages.add<JsonObject>();
  userMsg["role"] = "user";
  userMsg["content"] = userMessage;

  String requestBody;
  serializeJson(requestDoc, requestBody);

  // Build and log the curl command equivalent
  String curlCmd = "curl https://api.openai.com/v1/chat/completions \\\n";
  curlCmd += "  -H \"Content-Type: application/json\" \\\n";
  curlCmd +=
      "  -H \"Authorization: Bearer " + String(OPENAI_API_KEY) + "\" \\\n";
  curlCmd += "  -d '" + requestBody + "'";
  appLogger.log("\n--- Executing LLM Request ---");
  appLogger.log(curlCmd);

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
  filter["error"] = true; // Also catch API errors

  JsonDocument responseDoc;
  DeserializationError error = deserializeJson(
      responseDoc, client, DeserializationOption::Filter(filter));

  // Log response payload representation
  String responseDebug;
  serializeJson(responseDoc, responseDebug);
  appLogger.log("--- Raw API Response ---");
  appLogger.log(responseDebug);

  if (error) {
    appLogger.logf("LLM: JSON Parse Error: %s\n", error.c_str());
    return String("Error parsing LLM response: ") + error.c_str();
  }

  // Check if the API returned an explicit error
  if (responseDoc.containsKey("error")) {
    String errMsg = responseDoc["error"]["message"] | "Unknown API Error";
    return String("API Error: ") + errMsg;
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
  String modelToUse = String(LLM_MODEL);
  if (modelToUse == "gpt-3.5-turbo") {
    modelToUse = "claude-sonnet-4-6"; // Auto-correct default
  }
  requestDoc["model"] = modelToUse;
  requestDoc["max_tokens"] = 1024;
  String finalPrompt = String(SYSTEM_PROMPT) +
                       "\n\nHardware Context: You are running on " +
                       String(BOARD_INFO) +
                       ". When controlling pins, ALWAYS use the integer "
                       "value inside the bracket tool! e.g., [GPIO_ON: 2]."
                       "\n\nUser Hardware Setup:\n" +
                       String(USER_PINS);

  requestDoc["system"] = finalPrompt;

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
  filter["error"] = true;

  JsonDocument responseDoc;
  DeserializationError error = deserializeJson(
      responseDoc, client, DeserializationOption::Filter(filter));

  if (error) {
    Serial.print("LLM (Anthropic): JSON Parse Error: ");
    Serial.println(error.c_str());
    return String("Error parsing LLM response: ") + error.c_str();
  }

  if (responseDoc.containsKey("error")) {
    String errMsg = responseDoc["error"]["message"] | "Unknown API Error";
    return String("API Error: ") + errMsg;
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
  String finalPrompt = String(SYSTEM_PROMPT) +
                       "\n\nHardware Context: You are running on " +
                       String(BOARD_INFO) +
                       ". When controlling pins, ALWAYS use the integer "
                       "value inside the bracket tool! e.g., [GPIO_ON: 2]."
                       "\n\nUser Hardware Setup:\n" +
                       String(USER_PINS);

  sysParts["text"] = finalPrompt;

  JsonArray contents = requestDoc["contents"].to<JsonArray>();
  JsonObject userContent = contents.add<JsonObject>();
  userContent["role"] = "user";
  JsonObject userParts = userContent["parts"].to<JsonArray>().add<JsonObject>();
  userParts["text"] = userMessage;

  String requestBody;
  serializeJson(requestDoc, requestBody);

  String modelToUse = String(LLM_MODEL);
  if (modelToUse == "gpt-3.5-turbo") {
    modelToUse = "gemini-3-flash-preview"; // Auto-correct default
  }

  String url = String("/v1beta/models/") + modelToUse +
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

  bool isChunked = false;
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line.startsWith("Transfer-Encoding: chunked")) {
      isChunked = true;
    }
    if (line == "\r") {
      break;
    }
  }

  // Minimal filter
  JsonDocument filter;
  filter["candidates"][0]["content"]["parts"][0]["text"] = true;
  filter["error"] = true;

  JsonDocument responseDoc;
  DeserializationError error;

  if (isChunked) {
    // ArduinoJson struggles with raw chunked HTTP streams. We read the chunks
    // manually.
    String body = "";
    while (client.connected()) {
      String hexSize = client.readStringUntil('\n');
      hexSize.trim();
      if (hexSize.length() == 0)
        continue;
      long chunkSize = strtol(hexSize.c_str(), NULL, 16);
      if (chunkSize == 0)
        break;

      size_t readLen = 0;
      while (readLen < chunkSize && client.connected()) {
        if (client.available()) {
          char c = client.read();
          body += c;
          readLen++;
        }
      }
      // Read the trailing \r\n after the chunk
      client.readStringUntil('\n');
    }
    error = deserializeJson(responseDoc, body,
                            DeserializationOption::Filter(filter));
  } else {
    error = deserializeJson(responseDoc, client,
                            DeserializationOption::Filter(filter));
  }

  if (error) {
    Serial.print("LLM (Gemini): JSON Parse Error: ");
    Serial.println(error.c_str());
    return String("Error parsing LLM response: ") + error.c_str();
  }

  if (responseDoc.containsKey("error")) {
    String errMsg = responseDoc["error"]["message"] | "Unknown API Error";
    return String("API Error: ") + errMsg;
  }

  const char *content =
      responseDoc["candidates"][0]["content"]["parts"][0]["text"];
  if (content) {
    return String(content);
  }

  return "Error: Unexpected LLM response format.";
}
