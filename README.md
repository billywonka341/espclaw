# espclaw v2.3

> **v2.3 Release Update**
> - 📡 **Multi-ESP Grouping**: Easily manage multiple espclaw devices in a single Telegram chat via unique `Device IDs` and new commands like `/setup`, `/multiespclaw`, and `/changegpiodescription`!
>
> **v2.2 Release Update (March 3, 2026)**
> - 🔌 **Dynamic Hardware Control**: The AI now natively understands the physical pins of your specific board (e.g. D4 vs GPIO 2).
> - 🎛️ **Web UI Configurable Pins**: You can now define exactly what hardware is connected to what pin (like fans or relays) directly in the WebUI so the AI knows exactly what to control!
> - ✨ **ESP32 Web UI Overhaul**: New built-in "Logs" interface that captures API traces in real-time.
> - 🤖 **2026 Enterprise Models**: Explicit support added for the latest 2026 models like `gpt-5.2`, `claude-opus-4-6`, and `gemini-3-flash-preview` straight from the UI.
> - 📡 **cURL API Tracing**: The device now automatically synthesizes and logs the formatted `curl` command it sends to OpenAI, Anthropic, or Gemini for your debugging convenience.
> - 🧠 **Broader AI Personas**: The system prompts have been expanded so the AI will not only control hardware but gracefully answer general knowledge questions.

The smartest, smallest AI personal assistant designed to run on incredibly constrained hardware (ESP8266) while seamlessly scaling up to powerful capabilities on the ESP32.

With extensive optimizations, `espclaw` brings LLMs (OpenAI, Anthropic, OpenRouter) straight to your local network and Telegram, making your microcontroller truly intelligent.

---

## 🚀 Features

### Cross-Platform Magic
- **ESP8266**: Ultra-optimized memory footprint. Squeezes an entire AI agent into just ~80KB of RAM using streaming JSON parsers and insecure TLS buffering.
- **ESP32**: Premium upgrades including a **stunning, glassmorphic Web UI** and a dynamic **Configuration Portal** backed by non-volatile storage (NVS).

### Core Capabilities
- **Natural Language Tooling**: Automatically toggle GPIO pins via natural language commands (e.g., `[GPIO_ON: 4]`).
- **Telegram Integration**: Chat with your board from anywhere in the world.
- **Local Web UI (ESP32)**: Beautiful asynchronous local network chat interface with an onboard real-time Logs dashboard.
- **Over-The-Air Setup (ESP32)**: No need to hardcode WiFi! The ESP32 boots into Access Point mode if it can't connect, allowing you to configure everything from a browser.

### 🔌 Dynamic Hardware Control (Relays, Fans, LEDs)
`espclaw` isn't just a chatbot; it physically interacts with the real world! You can control hardware from anywhere on the planet simply by messaging your bot on **Telegram** (or using the local Web UI).
1. **ESP32**: Open the *Config* tab in the Web UI. Under the **Hardware Pins & Components** box, simply tell the AI what you plugged in in plain English! (e.g., *"GPIO 4 is connected to the living room fan relay. GPIO 13 is the red LED."*)
2. **ESP8266**: Open `src/config.h` and edit the `#define USER_PINS` string with the same English description.
3. Open Telegram and message your bot: *"Can you turn on the living room fan?"* The AI will automatically read your hardware definitions, figure out that the fan is on pin 4, and silently toggle the GPIO high!

---

## 🧠 How We Conquered the ESP8266's 80KB RAM

The ESP8266 only has ~80KB of data RAM (with roughly ~40KB free after Wi-Fi and system initialization). Running TLS and parsing complex LLM JSON responses usually crushes this chip memory-wise. Here is how `espclaw` avoids Out-of-Memory (OOM) crashes:

1. **Insecure TLS Buffer Minimization**: We bypass CA Certificate Validation (`client.setInsecure()`) and shrink the RX/TX buffers heavily (`client.setBufferSizes()`). Without this, the cert chain alone would consume all available RAM.
2. **Streaming JSON Filtering**: Traditional JSON parsing loads the entire response string into memory before decoding. Instead, `espclaw` uses `ArduinoJson 7` with `DeserializationOption::Filter` directly on the incoming TCP stream. It instructs the parser to *only* keep the actual `content` of the LLM reply and immediately discard the rest of the JSON bloat (headers, usage stats, context).
3. **No Heavy Tool Registries**: Standard ReAct agents send huge JSON Schemas for tools. We skip this. The `SYSTEM_PROMPT` simply teaches the LLM a syntax (`[GPIO_ON: PIN]`). The board intercepts these brackets using basic string methods, requiring virtually zero memory.
4. **BearSSL over mbedTLS**: Leverages the heavily optimized BearSSL library native to the `espressif8266` core.

---

## 🛠️ Setup Instructions

### Step 1: Install Prerequisites
1. **Download Visual Studio Code**: If you don't have it, download and install [VS Code](https://code.visualstudio.com/).
2. **Install PlatformIO Extension**: Open VS Code, go to the Extensions tab (Ctrl+Shift+X or Cmd+Shift+X), search for "PlatformIO IDE", and click Install. Wait for the installation to finish and reload the window if prompted.

### Step 2: Clone and Open the Project
1. **Clone the repository** (or download and extract the ZIP from the Releases tab):
   ```bash
   git clone https://github.com/yourusername/espclaw-esp8266.git
   ```
2. **Open the folder** in VS Code:
   - Go to `File > Open Folder...` and select the `espclaw-esp8266` directory.
3. Wait a few moments for PlatformIO to automatically detect the `platformio.ini` file and install the required toolchains and libraries for the ESP8266 and ESP32 platforms behind the scenes.

### Step 3: Configure and Flash

#### For ESP8266 (d1_mini, NodeMCU)
Because the ESP8266 lacks the RAM for a full web server alongside TLS, configuration is handled at compile time.
1. Open `src/config.h` in VS Code.
2. Enter your credentials (`WIFI_SSID`, `WIFI_PASSWORD`, `TELEGRAM_BOT_TOKEN`, `OPENAI_API_KEY`, `LLM_PROVIDER`).
   - `OPENAI_API_KEY` is completely generic and is used for Anthropic or Gemini too.
   - `LLM_PROVIDER` can be set to `"openai"`, `"anthropic"`, or `"gemini"`.
3. Connect your ESP8266 via USB.
4. Open the PlatformIO Core CLI (the little terminal icon on the bottom blue status bar of VS Code) and run:
   ```bash
   pio run -t upload -e d1_mini
   ```

#### For ESP32 (esp32dev, Wroom, S3, C3, etc.)
1. Connect your ESP32 via USB.
2. Open the PlatformIO Core CLI and flash the firmware. You do not need to configure anything in `config.h`. 
   ```bash
   pio run -t upload -e esp32dev
   ```
3. On first boot (or if your WiFi changes), the ESP32 will host an Access Point named **ESPClaw-Config** (Password: `12345678`).
4. Connect your phone or laptop to this `ESPClaw-Config` WiFi network, then open `http://192.168.4.1` in your browser.
5. Fill in your home WiFi and API credentials in the **Config Portal** and hit Save.
6. The ESP32 will reboot, securely transition to your home network, and serve the beautiful Chat UI on its new local IP!

---

## 📚 Documentation
For an in-depth dive into the system architecture, check out the [Full Documentation](docs/DOCUMENTATION.md).

## 📥 Releases
Automated binaries are generated for every release. Check the [Releases tab](../../releases) to download pre-compiled `.bin` files for both the ESP8266 and ESP32 platforms!

---

## 📄 License
This project is licensed under the [MIT License](LICENSE).

You are free to use, modify, and distribute this software, as long as the original copyright notice and permission notice are included in all copies or substantial portions of the software.
