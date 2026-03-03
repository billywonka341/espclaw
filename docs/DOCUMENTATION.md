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

### Dynamic Hardware Control & Context Injection
In v2.2+, `espclaw` injects specific board intelligence *before* it talks to the LLM. During compilation, macros defining the physical board structure (e.g. `BOARD_INFO="NodeMCU v2. Pins: D1=5..."`) are baked into the firmware.
Furthermore, users can configure **exactly what is connected** to the microcontroller pins so the AI knows how to interact with the real world.
- **ESP32:** Users type plain-English descriptions (e.g., *"GPIO 4 is connected to the living room fan relay"*) into the Web UI's "Hardware Pins" configuration box. This is saved to NVS.
- **ESP8266:** Users set the `#define USER_PINS` macro in `src/config.h`.

When you message the bot from anywhere in the world via **Telegram** (or the local Web UI) asking to *"turn on the fan"*, `espclaw` dynamically pieces together the base `SYSTEM_PROMPT`, the physical `BOARD_INFO`, and your custom `USER_PINS` hardware profile. The LLM instantly figures out which GPIO controls the fan and autonomously replies with the exact `[GPIO_ON: 4]` command.

### Multi-ESP & Telegram Command Interface (ESP32)

In v2.3+, `espclaw` supports dropping multiple ESP32 boards into the **same Telegram Group Chat**. You can assign each board a unique `Device ID` either dynamically or via the local WebUI. The boards will gracefully filter messages addressed to them.

**Supported Multi-ESP Telegram Commands:**
- `/setup`: Replies with connection services, LLM provider, and model info. Includes a warning that configuration text must be exact.
- `/multiespclaw`: Globally sets the board into Multi-ESP mode, randomly generates a unique device ID (e.g. `esp6382`), and replies with the newly assigned ID.
- `/gpiodescription` (or `/<deviceId> /gpiodescription`): Returns the device's currently saved hardware pin configuration description.
- `/changegpiodescription <new details>` (or `/<deviceId> /changegpiodescription <new details>`): Instantly overwrites the saved device GPIO configuration dynamically without opening the WebUI.
- `/<deviceId> /<newDeviceId>`: Instantly rename the device to a new ID.
- `/<deviceId> <prompt>`: Allows you to chat with or send tooling commands exclusively to that specific board within the group chat! Messages not targeting the board's name are intelligently ignored to save memory cycles.

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
