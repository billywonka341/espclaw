# ESPClaw Technical Documentation

Welcome to `espclaw`, an AI system scaled down for microcontrollers.

## Supported Hardware
1. **ESP8266 Family**: NodeMCU, Wemos D1 Mini 
2. **ESP32 Family**: ESP32 Wroom, Wrover, S2, C3, S3

## Architecture: How ESPClaw works

At its core, `espclaw` listens for triggers (like a Telegram message or Local Web UI post), asks an external LLM (like OpenAI) what to do using a strictly formatted prompt, parses the LLM's response, and executes the actions (tools) before replying.

### The ESP8266 Memory Challenge
ESP8266 only has ~80KB of RAM.
- **TLS Challenge**: TLS usually requires establishing a secure connection and verifying the server's certificate chain. A typical CA bundle takes massive amounts of RAM (and SPIFFS storage). `espclaw` uses `client.setInsecure()` to skip the CA verification. Furthermore, standard ESP32 TLS buffers are 16KB. `espclaw` overrides these to 2KB RX and 512B-1KB TX.
- **JSON Challenge**: LLM responses contain massive JSON objects. If `espclaw` tried to parse an API response using standard deserialization, it would OOM instantly. Instead, ArduinoJson 7 is configured with a `<Filter>` document depending on the provider:
  ```cpp
  // OpenAI Filter
  filter["choices"][0]["message"]["content"] = true;
  // Anthropic Filter
  filter["content"][0]["text"] = true;
  // Gemini Filter
  filter["candidates"][0]["content"]["parts"][0]["text"] = true;
  ```
  This is run *directly on the WiFiClientSecure TCP stream*, discarding everything except the actual message bytes.

### Tools: The Bracket Syntax
Instead of using standard OpenAI Function Calling (which requires massive JSON Schemas to define the schema structure, taking up precious RAM and API tokens), `espclaw` teaches the LLM via the System Prompt to output literal strings.

```text
You can control GPIO pins by outputting specific tool commands within your response in square brackets. For example:
- To turn ON GPIO 4: [GPIO_ON: 4]
- To turn OFF GPIO 4: [GPIO_OFF: 4]
```

When the LLM replies to the user, for example: `I have turned on the kitchen light for you. [GPIO_ON: 4]`, `espclaw` uses a simple `indexOf('[')` loop to extract that internal command, flip the GPIO pin via `digitalWrite`, and the chat user only ever sees the friendly text message.

### Advanced ESP32 Features

The ESP32 has plenty of horsepower (~320KB RAM in FreeRTOS). When `espclaw` detects an ESP32 build target during PlatformIO compilation (`#ifdef ESP32`), it unlocks the full feature set.
1. **ConfigManager**: The ESP32 ships with an NVS (Non-Volatile Storage) library (`Preferences.h`). Instead of relying on hardcoded C++ macros (like the ESP8266), the ESP32 dynamically reads `WIFI_SSID`, `OPENAI_API_KEY`, and `LLM_PROVIDER` from flash memory.
2. **ESPAsyncWebServer**: The ESP32 spins up a secondary asynchronous webserver on port 80.
3. **Captive Portal / AP Setup**: If the WiFi credentials in NVS are wrong or empty, the ESP32 acts as an Access Point (`ESPClaw-Config`).
4. **Local Web UI**: A beautifully styled, glass-morphism web app is served by the ESP32 asynchronously. It communicates with the controller using JSON `fetch()` calls.

## Adding Custom Tools
To add a new tool to `espclaw`, such as reading a Temperature sensor:
1. Open `config.h` (or the Web UI config page on ESP32) and update the System Prompt:
   ```text
   ... If the user asks for the temperature, you must output exactly [READ_TEMP] and I will substitute it.
   ```
2. In `main.cpp`, update `executeTools` or intercept the response string:
   ```cpp
   if (llmReply.indexOf("[READ_TEMP]") != -1) {
       float temp = readMySensor(); // your custom logic
       llmReply.replace("[READ_TEMP]", "The current temperature is " + String(temp) + "C.");
   }
   ```
