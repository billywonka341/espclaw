#if defined(ESP32)
#include "WebUI.h"
#include "ConfigManager.h"
#include <ArduinoJson.h>

AsyncWebServer server(80);
static WebChatCallback onMessageCallback = nullptr;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 AI Assistant</title>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;600&display=swap" rel="stylesheet">
    <style>
        :root {
            --bg-color: #0f172a;
            --glass-bg: rgba(30, 41, 59, 0.7);
            --glass-border: rgba(255, 255, 255, 0.1);
            --primary: #3b82f6;
            --primary-hover: #2563eb;
            --text-main: #f8fafc;
            --text-muted: #94a3b8;
        }
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: 'Inter', sans-serif;
            background: linear-gradient(135deg, #020617 0%, #0f172a 100%);
            color: var(--text-main);
            min-height: 100vh;
            display: flex;
            flex-direction: column;
        }
        nav {
            background: var(--glass-bg);
            backdrop-filter: blur(12px);
            border-bottom: 1px solid var(--glass-border);
            padding: 1rem 2rem;
            display: flex;
            justify-content: space-between;
            align-items: center;
            position: sticky;
            top: 0;
            z-index: 100;
        }
        .logo { font-size: 1.5rem; font-weight: 600; background: linear-gradient(90deg, #60a5fa, #a78bfa); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }
        .nav-links a {
            color: var(--text-muted);
            text-decoration: none;
            margin-left: 1.5rem;
            transition: color 0.3s ease;
        }
        .nav-links a:hover, .nav-links a.active { color: var(--text-main); }
        .container {
            flex: 1;
            display: flex;
            flex-direction: column;
            max-width: 800px;
            margin: 2rem auto;
            width: 100%;
            padding: 0 1rem;
        }
        .chat-box {
            flex: 1;
            background: var(--glass-bg);
            backdrop-filter: blur(16px);
            border: 1px solid var(--glass-border);
            border-radius: 16px;
            padding: 1.5rem;
            overflow-y: auto;
            display: flex;
            flex-direction: column;
            gap: 1rem;
            box-shadow: 0 25px 50px -12px rgba(0, 0, 0, 0.5);
            margin-bottom: 1rem;
            max-height: 60vh;
        }
        .msg { padding: 1rem; border-radius: 12px; max-width: 80%; animation: fadeIn 0.3s ease-up; line-height: 1.5; }
        .msg.user { align-self: flex-end; background: var(--primary); color: white; border-bottom-right-radius: 2px; }
        .msg.bot { align-self: flex-start; background: rgba(255, 255, 255, 0.05); border: 1px solid var(--glass-border); border-bottom-left-radius: 2px; }
        @keyframes fadeIn { from { opacity: 0; transform: translateY(10px); } to { opacity: 1; transform: translateY(0); } }
        .input-area {
            display: flex;
            gap: 0.75rem;
            background: var(--glass-bg);
            padding: 0.75rem;
            border-radius: 999px;
            border: 1px solid var(--glass-border);
        }
        input[type="text"] {
            flex: 1;
            background: transparent;
            border: none;
            color: white;
            padding: 0.5rem 1rem;
            font-size: 1rem;
            outline: none;
            font-family: inherit;
        }
        button {
            background: var(--primary);
            color: white;
            border: none;
            border-radius: 999px;
            padding: 0.5rem 1.5rem;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s ease;
            font-family: inherit;
        }
        button:hover { background: var(--primary-hover); transform: scale(1.05); }
        button:active { transform: scale(0.95); }
        
        .loader {
            display: none;
            align-self: flex-start;
            padding: 1rem;
            gap: 0.5rem;
        }
        .loader span { width: 8px; height: 8px; background: var(--primary); border-radius: 50%; animation: bounce 1.4s infinite ease-in-out both; }
        .loader span:nth-child(1) { animation-delay: -0.32s; }
        .loader span:nth-child(2) { animation-delay: -0.16s; }
        @keyframes bounce { 0%, 80%, 100% { transform: scale(0); } 40% { transform: scale(1); } }
        
        /* Config Form Styles */
        .config-form { display: none; flex-direction: column; gap: 1rem; background: var(--glass-bg); backdrop-filter: blur(16px); padding: 2rem; border-radius: 16px; border: 1px solid var(--glass-border); }
        .form-group { display: flex; flex-direction: column; gap: 0.5rem; }
        label { font-size: 0.9rem; color: var(--text-muted); font-weight: 600; }
        input.field { background: rgba(0,0,0,0.2); border: 1px solid var(--glass-border); color: white; padding: 0.75rem 1rem; border-radius: 8px; outline: none; transition: border-color 0.3s; }
        input.field:focus { border-color: var(--primary); }
    </style>
</head>
<body>
    <nav>
        <div class="logo">ESPClaw UI</div>
        <div class="nav-links">
            <a href="#" class="active" onclick="switchTab('chat')">Chat</a>
            <a href="#" onclick="switchTab('config')">Config</a>
        </div>
    </nav>
    <div class="container">
        <!-- Chat Interface -->
        <div id="chat-view" style="display: flex; flex-direction: column; height: 100%;">
            <div class="chat-box" id="cb">
                <div class="msg bot">Hello! I am your ESP32 Assistant. How can I help you today?</div>
                <div class="loader" id="loader"><span></span><span></span><span></span></div>
            </div>
            <div class="input-area">
                <input type="text" id="prompt" placeholder="Send a message..." onkeypress="if(event.key === 'Enter') sendMsg()">
                <button onclick="sendMsg()">Send</button>
            </div>
        </div>

        <!-- Config Interface -->
        <div id="config-view" class="config-form">
            <h2>System Configuration</h2>
            <div class="form-group"><label>WiFi SSID</label><input type="text" id="c_ssid" class="field"></div>
            <div class="form-group"><label>WiFi Password</label><input type="password" id="c_pass" class="field"></div>
            <div class="form-group"><label>Telegram Bot Token</label><input type="password" id="c_tgToken" class="field"></div>
            <div class="form-group"><label>Telegram Chat ID</label><input type="text" id="c_tgChat" class="field"></div>
            <div class="form-group">
                <label>LLM Provider</label>
                <select id="c_provider" class="field">
                    <option value="openai">OpenAI</option>
                    <option value="anthropic">Anthropic (Claude)</option>
                    <option value="gemini">Google Gemini</option>
                </select>
            </div>
            <div class="form-group"><label>API Key</label><input type="password" id="c_aiKey" class="field"></div>
            <button onclick="saveConfig()" style="margin-top: 1rem;">Save & Reboot</button>
        </div>
    </div>

    <script>
        function switchTab(tab) {
            document.querySelectorAll('.nav-links a').forEach(a => a.classList.remove('active'));
            document.querySelector(`.nav-links a[onclick="switchTab('${tab}')"]`).classList.add('active');
            document.getElementById('chat-view').style.display = tab === 'chat' ? 'flex' : 'none';
            document.getElementById('config-view').style.display = tab === 'config' ? 'flex' : 'none';
            if (tab === 'config') loadConfig();
        }

        async function sendMsg() {
            const input = document.getElementById('prompt');
            const text = input.value.trim();
            if(!text) return;
            
            appendMsg(text, 'user');
            input.value = '';
            
            const loader = document.getElementById('loader');
            loader.style.display = 'flex';
            
            try {
                const res = await fetch('/api/chat', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({message: text})
                });
                const data = await res.json();
                loader.style.display = 'none';
                appendMsg(data.reply, 'bot');
            } catch (err) {
                loader.style.display = 'none';
                appendMsg('Error communicating with ESP32.', 'bot');
            }
        }

        function appendMsg(text, sender) {
            const cb = document.getElementById('cb');
            const loader = document.getElementById('loader');
            const div = document.createElement('div');
            div.className = `msg ${sender}`;
            div.innerText = text;
            cb.insertBefore(div, loader);
            cb.scrollTop = cb.scrollHeight;
        }

        async function loadConfig() {
            const res = await fetch('/api/config');
            const cfg = await res.json();
            document.getElementById('c_ssid').value = cfg.ssid || '';
            document.getElementById('c_pass').value = cfg.pass || '';
            document.getElementById('c_tgToken').value = cfg.tgToken || '';
            document.getElementById('c_tgChat').value = cfg.tgChat || '';
            document.getElementById('c_provider').value = cfg.provider || 'openai';
            document.getElementById('c_aiKey').value = cfg.aiKey || '';
        }

        async function saveConfig() {
            const payload = {
                ssid: document.getElementById('c_ssid').value,
                pass: document.getElementById('c_pass').value,
                tgToken: document.getElementById('c_tgToken').value,
                tgChat: document.getElementById('c_tgChat').value,
                provider: document.getElementById('c_provider').value,
                aiKey: document.getElementById('c_aiKey').value
            };
            await fetch('/api/config', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify(payload)
            });
            alert('Settings saved. ESP32 is rebooting...');
        }
    </script>
</body>
</html>
)rawliteral";

void initWebUI(WebChatCallback chatCallback) {
  onMessageCallback = chatCallback;

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html);
  });

  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["ssid"] = configManager.getWifiSsid();
    doc["pass"] = configManager.getWifiPassword();
    doc["tgToken"] = configManager.getTelegramBotToken();
    doc["tgChat"] = configManager.getTelegramChatId();
    doc["provider"] = configManager.getLlmProvider();
    doc["aiKey"] = configManager.getOpenAiApiKey();
    String out;
    serializeJson(doc, out);
    request->send(200, "application/json", out);
  });

  server.on(
      "/api/config", HTTP_POST,
      [](AsyncWebServerRequest *request) {
        // We handle JSON via body handler below
      },
      NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len,
         size_t index, size_t total) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, data, len);
        if (!error) {
          configManager.saveWifiConfig(doc["ssid"] | "", doc["pass"] | "");
          configManager.saveTelegramConfig(doc["tgToken"] | "",
                                           doc["tgChat"] | "");

          // Keep existing prompt and model, just update API key and provider
          configManager.saveLlmConfig(
              doc["aiKey"] | "", doc["provider"] | "openai",
              configManager.getLlmModel(), configManager.getSystemPrompt());

          request->send(200, "application/json", "{\"status\":\"ok\"}");
          delay(500);
          ESP.restart();
        } else {
          request->send(400, "application/json", "{\"status\":\"error\"}");
        }
      });

  server.on(
      "/api/chat", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len,
         size_t index, size_t total) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, data, len);
        String replyText = "Error processing request.";
        if (!error && onMessageCallback) {
          String userMsg = doc["message"] | "";
          onMessageCallback(userMsg, replyText);
        }
        JsonDocument outDoc;
        outDoc["reply"] = replyText;
        String outStr;
        serializeJson(outDoc, outStr);
        request->send(200, "application/json", outStr);
      });

  server.begin();
  Serial.println("WebUI started on HTTP port 80");
}
#endif
