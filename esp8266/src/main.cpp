#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <stddef.h>
#include <string.h>
#include "blaze_caps.h"
#include "generated_web.h"

// ESP8266-specific backend. The frontend is shared with the ESP32 target.
// No firmware flashing, OTA updater, display driving or UART takeover exists here.
#if defined(BLAZE_D1_MINI)
static constexpr const char *BOARD = "esp8266-d1-mini-reference";
#else
static constexpr const char *BOARD = "esp8266-nodemcu-esp12e-reference";
#endif
static constexpr uint32_t LAB_LEASE_MS = 20000;
static constexpr size_t EEPROM_BYTES = 128;
static constexpr uint32_t CREDS_MAGIC = 0xB1A2E826;
static ESP8266WebServer server(80);

struct Credentials {
  uint32_t marker;
  uint8_t version;
  char ap[21];
  char token[33];
  uint32_t checksum;
};
static Credentials credentials = {};
static int activePin = -1;
static bool activeOutput = false;
static int lastLevel = 0;
static uint32_t rising = 0, falling = 0;
static uint32_t startAt = 0, lastSample = 0;
static bool initialized = false;

static uint32_t credentialChecksum(const Credentials &c) {
  // FNV-1a is a corruption check, not authentication or key derivation.
  uint32_t hash = 2166136261UL;
  const uint8_t *ptr = reinterpret_cast<const uint8_t *>(&c);
  for (size_t i = 0; i < offsetof(Credentials, checksum); i++) {
    hash ^= ptr[i]; hash *= 16777619UL;
  }
  return hash;
}
static void fillSecret(char *out, size_t chars) {
  constexpr char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789";
  // ESP.random() has a hardware entropy source when Wi-Fi is active.
  for (size_t i = 0; i < chars; i++) {
    const uint32_t r = ESP.random();
    out[i] = alphabet[r % (sizeof(alphabet) - 1)];
  }
  out[chars] = '\0';
}
static bool provisionDevice() {
  EEPROM.begin(EEPROM_BYTES);
  EEPROM.get(0, credentials);
  const bool valid = credentials.marker == CREDS_MAGIC &&
    credentials.version == 1 &&
    credentials.ap[20] == '\0' && credentials.token[32] == '\0' &&
    credentials.checksum == credentialChecksum(credentials);
  if (!valid) {
    memset(&credentials, 0, sizeof(credentials));
    credentials.marker = CREDS_MAGIC;
    credentials.version = 1;
    fillSecret(credentials.ap, 20);
    fillSecret(credentials.token, 32);
    credentials.checksum = credentialChecksum(credentials);
    EEPROM.put(0, credentials);
    if (!EEPROM.commit()) {
      EEPROM.end();
      memset(&credentials, 0, sizeof(credentials));
      return false; // Never expose an AP with unpersisted/unknown credentials.
    }
  }
  EEPROM.end();
  return true;
}
static bool authenticated() {
  String candidate = server.header("X-Blaze-Token");
  uint8_t diff = static_cast<uint8_t>(candidate.length() ^ 32U);
  for (unsigned i=0; i<32; ++i) {
    const char ch = i < candidate.length() ? candidate[i] : '\0';
    diff |= static_cast<uint8_t>(ch ^ credentials.token[i]);
  }
  return diff == 0;
}
static void commonHeaders() {
  server.sendHeader("Cache-Control","no-store");
  server.sendHeader("X-Content-Type-Options","nosniff");
}
template <typename Doc>
static void sendJson(int status, const Doc &doc) {
  String payload;
  serializeJson(doc, payload);
  commonHeaders();
  server.send(status, "application/json; charset=utf-8", payload);
}
static void errorJson(int status, const char *message) {
  StaticJsonDocument<256> doc;
  doc["status"]="error";
  doc["message"]=message;
  sendJson(status, doc);
}
static bool requireAdmin() {
  if (authenticated()) return true;
  errorJson(401,"Admin token required. Read it from the device serial console.");
  return false;
}
static void releasePin() {
  if (activePin >= 0) {
    // Switch to high impedance. A LOW signal is NOT universally safe for loads.
    pinMode(activePin, INPUT);
  }
  activePin = -1;
  activeOutput = false;
  lastLevel = 0;
}
static void tickLab() {
  if (activePin < 0) return;
  uint32_t now = millis();
  if (static_cast<uint32_t>(now - startAt) >= LAB_LEASE_MS) {
    releasePin();
    return;
  }
  if (!activeOutput && static_cast<uint32_t>(now - lastSample) >= 20) {
    const int value = digitalRead(activePin);
    if (value != lastLevel) {
      if (value == HIGH) ++rising; else ++falling;
      lastLevel = value;
    }
    lastSample = now;
  }
}
static void statusGet() {
  StaticJsonDocument<512> doc;
  doc["product"]="BlazeMCU ESP Starter";
  doc["version"]="0.1.0-preview-esp8266";
  doc["board"]=BOARD;
  doc["cores"]=1;
  doc["heap_free"]=ESP.getFreeHeap();
  doc["flash_bytes"]=ESP.getFlashChipRealSize();
  doc["portal"]="192.168.4.1";
  doc["gpio_mode"]="session_only_20s";
  doc["loader"]="disabled_pending_recovery_validation";
  sendJson(200, doc);
}
static void pinsGet() {
  commonHeaders();
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json; charset=utf-8", "");
  server.sendContent(String("{\"board\":\"") + BOARD + "\",\"pins\":[");
  for (size_t i = 0; i < BLAZE8266_PIN_COUNT; i++) {
    const Blaze8266Pin &p = BLAZE8266_PINS[i];
    StaticJsonDocument<768> doc;
    doc["number"]=p.number;
    doc["label"]=p.label;
    doc["exposed"]=p.exposed;
    doc["input"]=p.input;
    doc["output"]=p.output;
    doc["adc"]=p.adc;
    doc["pwm"]=p.pwm;
    doc["strap"]=p.strap;
    doc["reserved"]=p.reserved;
    doc["category"]=p.category;
    doc["warning"]=p.warning;
    String part;
    serializeJson(doc, part);
    if (i) server.sendContent(",");
    server.sendContent(part);
    yield();
  }
  server.sendContent("]}");
  server.sendContent("");
}
static void gpioGet() {
  tickLab();
  StaticJsonDocument<384> doc;
  doc["status"] = activePin < 0 ? "idle" : "running";
  doc["pin"] = activePin;
  doc["mode"] = activeOutput ? "output" : "input";
  doc["level"] = activePin < 0 ? 0 : (activeOutput ? lastLevel : digitalRead(activePin));
  doc["rising_edges"] = rising;
  doc["falling_edges"] = falling;
  const uint32_t passed = static_cast<uint32_t>(millis() - startAt);
  doc["expires_in_ms"] = activePin < 0 ? 0 : (passed >= LAB_LEASE_MS ? 0 : LAB_LEASE_MS - passed);
  sendJson(200, doc);
}
static void gpioPost() {
  if (!requireAdmin()) return;
  String raw=server.arg("plain");
  if (raw.length()==0 || raw.length()>256) {
    errorJson(413,"A JSON body of at most 256 bytes is required");
    return;
  }
  StaticJsonDocument<320> doc;
  if (deserializeJson(doc, raw) || !doc["pin"].is<int>() ||
      !doc["mode"].is<const char*>() ||
      (doc.containsKey("level") && !doc["level"].is<int>()) ||
      (doc.containsKey("ack") && !doc["ack"].is<bool>()) ||
      (doc.containsKey("pullup") && !doc["pullup"].is<bool>())) {
    errorJson(400,"Malformed or invalid GPIO request"); return;
  }
  const int pin=doc["pin"].as<int>();
  const char *mode=doc["mode"];
  if (strcmp(mode,"input")!=0 && strcmp(mode,"output")!=0) {
    errorJson(400,"Mode must be input or output"); return;
  }
  const bool output=strcmp(mode,"output")==0;
  const int level=doc["level"] | 0;
  const bool ack=doc["ack"] | false;
  const bool pullup=doc["pullup"] | false;
  if (level!=0 && level!=1) {
    errorJson(400,"Output state must be HIGH or LOW"); return;
  }
  if (pullup && (output || pin==16)) {
    errorJson(422,"Internal pull-up unsupported for requested GPIO/mode"); return;
  }
  const Blaze8266Decision decision=blaze8266Policy(pin,output,ack);
  if (decision==BLAZE8266_ACK_REQUIRED) {
    errorJson(409,"Sensitive pin: acknowledge the warning before testing"); return;
  }
  if (decision==BLAZE8266_DENY) {
    errorJson(422,"Unsupported digital operation: flash, absent, analog-only or input-only pin"); return;
  }
  releasePin();
  rising=falling=0;
  if (output) {
    digitalWrite(pin,level ? HIGH : LOW); // preload latch before output enable
    pinMode(pin,OUTPUT);
  } else {
    pinMode(pin,pullup ? INPUT_PULLUP : INPUT);
  }
  activePin=pin;
  activeOutput=output;
  lastLevel=output ? level : digitalRead(pin);
  startAt=lastSample=millis();
  StaticJsonDocument<256> response;
  response["status"]="running";
  response["message"]="Test active for up to 20 seconds; STOP to release the pin";
  sendJson(200,response);
}
static void gpioStop() {
  if (!requireAdmin()) return;
  releasePin();
  StaticJsonDocument<128> doc;
  doc["status"]="idle";
  sendJson(200,doc);
}
static void loaderDisabled() {
  if (!requireAdmin()) return;
  errorJson(501,"FLASH and PREBOOT unavailable: image validation and recovery not yet proven on ESP8266");
}
static void indexGet() {
  commonHeaders();
  server.sendHeader("Content-Security-Policy",
                    "default-src 'self'; style-src 'self' 'unsafe-inline'; script-src 'self' 'unsafe-inline'; connect-src 'self'; object-src 'none'; base-uri 'none'");
  server.send_P(200, PSTR("text/html; charset=utf-8"), BLAZE_INDEX_HTML);
}
static void beginServer() {
  const String headers[]={"X-Blaze-Token"};
  server.collectHeaders(headers,1);
  server.on("/",HTTP_GET,indexGet);
  server.on("/api/status",HTTP_GET,statusGet);
  server.on("/api/pins",HTTP_GET,pinsGet);
  server.on("/api/gpio",HTTP_GET,gpioGet);
  server.on("/api/gpio",HTTP_POST,gpioPost);
  server.on("/api/gpio/stop",HTTP_POST,gpioStop);
  server.on("/api/firmware/flash",HTTP_POST,loaderDisabled);
  server.on("/api/firmware/preboot",HTTP_POST,loaderDisabled);
  server.onNotFound([](){ errorJson(404,"Unknown route"); });
  server.begin();
}
void setup() {
  Serial.begin(115200);
  Serial.println();
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA); // Enable radio BEFORE generating first-boot entropy.
  WiFi.disconnect();
  delay(100);
  if (!provisionDevice()) {
    Serial.println(F("FATAL: credential provisioning failed; AP disabled."));
    return;
  }
  const uint32_t chip=ESP.getChipId();
  char ssid[24];
  snprintf(ssid,sizeof(ssid),"BlazeMCU-%04X",static_cast<unsigned>(chip & 0xFFFFU));
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(ssid,credentials.ap,1,false,4)) {
    Serial.println(F("FATAL: access-point creation failed."));
    return;
  }
  beginServer();
  initialized=true;
  Serial.printf("BlazeMCU ESP8266 PREVIEW: %s; http://192.168.4.1\n", ssid);
  Serial.printf("Per-device WPA2 AP password (serial only): %s\n",credentials.ap);
  Serial.printf("Admin token (serial only): %s\n",credentials.token);
  Serial.println(F("WARNING: serial credentials must not appear in customer logs."));
  Serial.println(F("GPIO tests are session-only; FLASH and PREBOOT disabled."));
}
void loop() {
  if (initialized) {
    server.handleClient();
    tickLab();
  }
  delay(1); // Yield to Wi-Fi/watchdog. No blocking lab delays.
}
