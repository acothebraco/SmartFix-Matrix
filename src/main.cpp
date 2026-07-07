#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 1

#define FIRMWARE_VERSION "0.6.0"

// WLAN Access Point
const char *AP_SSID = "SmartFix-Matrix";
const char *AP_PASSWORD = "smartfix123";

WebServer server(80);
Preferences prefs;
const char *PREF_NAMESPACE = "smartfix";

MatrixPanel_I2S_DMA *display = nullptr;

// Farben
uint16_t black;
uint16_t white;
uint16_t green;
uint16_t blue;
uint16_t red;
uint16_t yellow;

// Helligkeit
uint8_t matrixBrightness = 70;

// -----------------------------
// Display Modes
// -----------------------------
enum DisplayMode {
  MODE_SCROLL_TEXT = 0,
  MODE_LOGO_STATIC = 1,
  MODE_PIXEL_ART   = 2,
  MODE_RANDOM_FX   = 3
};

DisplayMode currentMode = MODE_SCROLL_TEXT;

bool autoModeDemo = true;
unsigned long lastModeChange = 0;
const unsigned long modeInterval = 10000;

// -----------------------------
// Laufschrift
// -----------------------------
String scrollText = "ELEKTRONIKSERVICE  -  REPARATUR  -  KONSOLEN  -  SMARTFIX  ";
const uint16_t MAX_SCROLL_TEXT_LEN = 160;
int16_t scrollX = PANEL_RES_X;
unsigned long lastScrollUpdate = 0;
const unsigned long scrollInterval = 35;

// -----------------------------
// Pixel / FX
// -----------------------------
unsigned long lastFxUpdate = 0;
unsigned long lastFullRedraw = 0;

// -----------------------------
// Helper
// -----------------------------
int16_t getTextPixelWidth(const String &text) {
  return text.length() * 6;
}

const char *getModeName(DisplayMode mode) {
  switch (mode) {
    case MODE_SCROLL_TEXT: return "SCROLL_TEXT";
    case MODE_LOGO_STATIC: return "LOGO_STATIC";
    case MODE_PIXEL_ART:   return "PIXEL_ART";
    case MODE_RANDOM_FX:   return "RANDOM_FX";
    default:               return "UNKNOWN";
  }
}

void loadSettings() {
  prefs.begin(PREF_NAMESPACE, false);

  matrixBrightness = prefs.getUChar("bright", 70);

  scrollText = prefs.getString("text", scrollText);

  if (scrollText.length() == 0) {
    scrollText = "ELEKTRONIKSERVICE  -  REPARATUR  -  KONSOLEN  -  SMARTFIX  ";
  }

  if (scrollText.length() > MAX_SCROLL_TEXT_LEN) {
    scrollText = scrollText.substring(0, MAX_SCROLL_TEXT_LEN);
  }

  int savedMode = prefs.getInt("mode", MODE_SCROLL_TEXT);
  if (savedMode < MODE_SCROLL_TEXT || savedMode > MODE_RANDOM_FX) {
    savedMode = MODE_SCROLL_TEXT;
  }

  currentMode = (DisplayMode)savedMode;
  autoModeDemo = prefs.getBool("auto", true);

  Serial.println("Settings loaded:");
  Serial.print("Brightness: ");
  Serial.println(matrixBrightness);
  Serial.print("Scroll Text: ");
  Serial.println(scrollText);
  Serial.print("Mode: ");
  Serial.println(getModeName(currentMode));
  Serial.print("Auto Demo: ");
  Serial.println(autoModeDemo ? "ON" : "OFF");
}

void saveModeSettings() {
  prefs.putInt("mode", (int)currentMode);
  prefs.putBool("auto", autoModeDemo);

  Serial.println("Mode settings saved");
}

void saveBrightnessSetting() {
  prefs.putUChar("bright", matrixBrightness);

  Serial.print("Brightness saved: ");
  Serial.println(matrixBrightness);
}

void saveScrollTextSetting() {
  prefs.putString("text", scrollText);

  Serial.print("Scroll text saved: ");
  Serial.println(scrollText);
}

void setMode(DisplayMode newMode, bool saveSetting = true) {
  currentMode = newMode;

  display->clearScreen();

  scrollX = PANEL_RES_X;
  lastScrollUpdate = 0;
  lastFxUpdate = 0;
  lastFullRedraw = 0;

  Serial.print("Mode changed to: ");
  Serial.println(getModeName(currentMode));

  if (saveSetting) {
    saveModeSettings();
  }
}

// -----------------------------
// Matrix Init
// -----------------------------
void initMatrix() {
  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,
    PANEL_RES_Y,
    PANEL_CHAIN
  );

  // Waveshare ESP32-S3-RGB-Matrix Settings
  mxconfig.gpio.e = 9;
  mxconfig.clkphase = false;
  mxconfig.driver = HUB75_I2S_CFG::SHIFTREG;

  display = new MatrixPanel_I2S_DMA(mxconfig);

  if (!display->begin()) {
    Serial.println("ERROR: display->begin() failed!");
    while (true) {
      delay(1000);
    }
  }

  display->setBrightness8(matrixBrightness);
  display->clearScreen();

  black  = display->color565(0, 0, 0);
  white  = display->color565(255, 255, 255);
  green  = display->color565(0, 255, 80);
  blue   = display->color565(0, 120, 255);
  red    = display->color565(255, 0, 0);
  yellow = display->color565(255, 180, 0);
}

// -----------------------------
// Gemeinsame Zeichenelemente
// -----------------------------
void drawHeader() {
  display->setTextWrap(false);
  display->setTextSize(1);

  display->setCursor(2, 3);
  display->setTextColor(green);
  display->print("Smart");

  display->setCursor(34, 3);
  display->setTextColor(blue);
  display->print("Fix");

  display->drawLine(0, 13, 63, 13, blue);
}

// -----------------------------
// Mode 0: Laufschrift
// -----------------------------
void drawScrollingText() {
  unsigned long now = millis();

  if (now - lastScrollUpdate >= scrollInterval) {
    lastScrollUpdate = now;

    display->fillScreen(black);
    drawHeader();

    display->setTextWrap(false);
    display->setTextSize(1);
    display->setTextColor(white);
    display->setCursor(scrollX, 20);
    display->print(scrollText);

    scrollX--;

    int16_t textWidth = getTextPixelWidth(scrollText);

    if (scrollX < -textWidth) {
      scrollX = PANEL_RES_X;
    }
  }
}

// -----------------------------
// Mode 1: Statisches Logo / Startscreen
// -----------------------------
void drawLogoStatic() {
  unsigned long now = millis();

  if (now - lastFullRedraw < 500) {
    return;
  }

  lastFullRedraw = now;

  display->fillScreen(black);

  drawHeader();

  display->setTextSize(1);
  display->setTextColor(white);
  display->setCursor(5, 18);
  display->print("MATRIX");

  display->drawRect(0, 0, 64, 32, green);
  display->drawPixel(1, 1, blue);
  display->drawPixel(62, 1, blue);
  display->drawPixel(1, 30, blue);
  display->drawPixel(62, 30, blue);
}

// -----------------------------
// Mode 2: Pixel-Art Beispiel
// -----------------------------
void drawPixelArt() {
  unsigned long now = millis();

  if (now - lastFullRedraw < 500) {
    return;
  }

  lastFullRedraw = now;

  display->fillScreen(black);

  // Einfaches Pixel-Art Herz links
  display->fillRect(8, 8, 4, 4, red);
  display->fillRect(16, 8, 4, 4, red);
  display->fillRect(4, 12, 20, 4, red);
  display->fillRect(8, 16, 12, 4, red);
  display->fillRect(12, 20, 4, 4, red);

  // Text rechts
  display->setTextWrap(false);
  display->setTextSize(1);
  display->setTextColor(green);
  display->setCursor(31, 7);
  display->print("PIX");

  display->setTextColor(blue);
  display->setCursor(31, 18);
  display->print("ART");
}

// -----------------------------
// Mode 3: Random Pixel FX
// -----------------------------
void drawRandomFx() {
  unsigned long now = millis();

  if (now - lastFullRedraw > 3000) {
    lastFullRedraw = now;
    display->fillScreen(black);

    display->setTextWrap(false);
    display->setTextSize(1);
    display->setTextColor(green);
    display->setCursor(3, 3);
    display->print("RANDOM");

    display->setTextColor(blue);
    display->setCursor(18, 14);
    display->print("FX");
  }

  if (now - lastFxUpdate >= 20) {
    lastFxUpdate = now;

    int x = random(0, PANEL_RES_X);
    int y = random(0, PANEL_RES_Y);

    uint16_t color;

    int r = random(0, 4);
    if (r == 0) color = green;
    else if (r == 1) color = blue;
    else if (r == 2) color = white;
    else color = yellow;

    display->drawPixel(x, y, color);
  }
}

// -----------------------------
// Demo Mode Switch
// -----------------------------
void handleAutoModeDemo() {
  if (!autoModeDemo) {
    return;
  }

  unsigned long now = millis();

  if (now - lastModeChange >= modeInterval) {
    lastModeChange = now;

    int nextMode = (int)currentMode + 1;

    if (nextMode > MODE_RANDOM_FX) {
      nextMode = MODE_SCROLL_TEXT;
    }

    setMode((DisplayMode)nextMode);
  }
}

// -----------------------------
// Webinterface
// -----------------------------
String htmlButton(const String &label, const String &url) {
  return "<a class='btn' href='" + url + "'>" + label + "</a>";
}

String htmlEscape(const String &input) {
  String output = input;
  output.replace("&", "&amp;");
  output.replace("\"", "&quot;");
  output.replace("'", "&#39;");
  output.replace("<", "&lt;");
  output.replace(">", "&gt;");
  return output;
}

String htmlPage() {
  String autoStatus = autoModeDemo ? "AKTIV" : "AUS";

  String page = "";
  page += "<!DOCTYPE html><html lang='de'><head>";
  page += "<meta charset='UTF-8'>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  page += "<title>SmartFix Matrix</title>";
  page += "<style>";
  page += "body{margin:0;font-family:Arial,Helvetica,sans-serif;background:#0b0f14;color:#e5e7eb;}";
  page += ".wrap{max-width:760px;margin:0 auto;padding:22px;}";
  page += ".card{background:#111827;border:1px solid #1f2937;border-radius:18px;padding:20px;margin-bottom:16px;box-shadow:0 10px 30px rgba(0,0,0,.35);}";
  page += "h1{margin:0 0 6px;font-size:28px;color:#22c55e;}";
  page += "h2{margin:0 0 14px;font-size:18px;color:#60a5fa;}";
  page += ".sub{color:#9ca3af;margin-bottom:18px;}";
  page += ".status{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:12px;}";
  page += ".pill{background:#020617;border:1px solid #334155;border-radius:12px;padding:10px;}";
  page += ".label{font-size:12px;color:#94a3b8;}";
  page += ".value{font-size:17px;font-weight:bold;color:#f8fafc;margin-top:4px;}";
  page += ".buttons{display:grid;grid-template-columns:1fr 1fr;gap:10px;}";
  page += ".btn{display:block;text-align:center;text-decoration:none;background:#2563eb;color:white;padding:14px;border-radius:12px;font-weight:bold;}";
  page += ".btn:hover{background:#1d4ed8;}";
  page += ".btn.green{background:#16a34a;}";
  page += ".btn.green:hover{background:#15803d;}";
  page += ".small{font-size:13px;color:#94a3b8;margin-top:18px;text-align:center;}";
  page += "</style></head><body>";
  page += "<div class='wrap'>";
  page += "<div class='card'>";
  page += "<h1>SmartFix Matrix</h1>";
  page += "<div class='sub'>ESP32-S3 RGB Matrix Control Panel</div>";
  page += "<div class='status'>";
  page += "<div class='pill'><div class='label'>Firmware</div><div class='value'>v";
  page += FIRMWARE_VERSION;
  page += "</div></div>";
  page += "<div class='pill'><div class='label'>Mode</div><div class='value'>";
  page += getModeName(currentMode);
  page += "</div></div>";
  page += "<div class='pill'><div class='label'>Auto Demo</div><div class='value'>";
  page += autoStatus;
  page += "</div></div>";
  page += "<div class='pill'><div class='label'>Brightness</div><div class='value'>";
  page += String(matrixBrightness);
  page += " / 255</div></div>";
  page += "</div></div>";

  page += "<div class='card'>";
  page += "<h2>Modus ausw&auml;hlen</h2>";
  page += "<div class='buttons'>";
  page += htmlButton("Laufschrift", "/mode?m=0");
  page += htmlButton("Static Logo", "/mode?m=1");
  page += htmlButton("Pixel Art", "/mode?m=2");
  page += htmlButton("Random FX", "/mode?m=3");
  page += "</div></div>";

  page += "<div class='card'>";
  page += "<h2>Laufschrift Text</h2>";
  page += "<form action='/set-text' method='GET'>";
  page += "<input name='t' maxlength='160' value='";
  page += htmlEscape(scrollText);
  page += "' style='width:100%;box-sizing:border-box;background:#020617;color:#e5e7eb;border:1px solid #334155;border-radius:12px;padding:14px;font-size:16px;margin-bottom:12px;'>";
  page += "<button class='btn green' type='submit' style='border:0;width:100%;cursor:pointer;'>Text speichern</button>";
  page += "</form>";
  page += "<div class='sub' style='margin-top:12px;'>Maximal 160 Zeichen. Umlaute testen wir sp&auml;ter mit eigener Font-Unterst&uuml;tzung.</div>";
  page += "</div>";

  page += "<div class='card'>";
  page += "<h2>Auto Demo</h2>";
  page += "<div class='buttons'>";
  page += "<a class='btn green' href='/auto'>Auto Demo starten</a>";
  page += htmlButton("Refresh", "/");
  page += "</div></div>";

  page += "<div class='card'>";
  page += "<h2>Helligkeit</h2>";
  page += "<div class='buttons'>";
  page += htmlButton("25%", "/brightness?v=40");
  page += htmlButton("50%", "/brightness?v=80");
  page += htmlButton("75%", "/brightness?v=130");
  page += htmlButton("100%", "/brightness?v=200");
  page += "</div></div>";

  page += "<div class='card'>";
  page += "<h2>System</h2>";
  page += "<div class='buttons'>";
  page += htmlButton("Werkseinstellungen", "/factory-reset");
  page += htmlButton("Refresh", "/");
  page += "</div></div>";

  page += "<div class='small'>SmartFix Elektronikservice &bull; Designed for 64x32 HUB75 RGB Matrix</div>";
  page += "</div></body></html>";

  return page;
}

void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void redirectHome() {
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleModeChange() {
  if (server.hasArg("m")) {
    int mode = server.arg("m").toInt();

    if (mode >= MODE_SCROLL_TEXT && mode <= MODE_RANDOM_FX) {
      autoModeDemo = false;
      setMode((DisplayMode)mode, true);
    }
  }

  redirectHome();
}

void handleAutoDemo() {
  autoModeDemo = true;
  lastModeChange = millis();

  saveModeSettings();

  Serial.println("Auto mode demo enabled from web");
  redirectHome();
}

void handleBrightness() {
  if (server.hasArg("v")) {
    int value = server.arg("v").toInt();

    if (value < 5) value = 5;
    if (value > 255) value = 255;

    matrixBrightness = value;
    display->setBrightness8(matrixBrightness);

    saveBrightnessSetting();

    Serial.print("Brightness changed to: ");
    Serial.println(matrixBrightness);
  }

  redirectHome();
}

void handleSetText() {
  if (server.hasArg("t")) {
    String newText = server.arg("t");

    newText.trim();

    if (newText.length() == 0) {
      newText = "SMARTFIX ELEKTRONIKSERVICE";
    }

    if (newText.length() > MAX_SCROLL_TEXT_LEN) {
      newText = newText.substring(0, MAX_SCROLL_TEXT_LEN);
    }

    scrollText = newText + "   ";

    scrollX = PANEL_RES_X;
    saveScrollTextSetting();

    autoModeDemo = false;
    setMode(MODE_SCROLL_TEXT, true);

    Serial.print("New scroll text from web: ");
    Serial.println(scrollText);
  }

  redirectHome();
}

void handleFactoryReset() {
  prefs.clear();

  server.send(200, "text/html",
              "<html><body style='background:#0b0f14;color:white;font-family:Arial;text-align:center;padding-top:40px;'>"
              "<h1>SmartFix Matrix</h1>"
              "<p>Einstellungen wurden gel&ouml;scht.</p>"
              "<p>Neustart...</p>"
              "</body></html>");

  delay(1000);
  ESP.restart();
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/mode", handleModeChange);
  server.on("/auto", handleAutoDemo);
  server.on("/brightness", handleBrightness);
  server.on("/set-text", handleSetText);
  server.on("/factory-reset", handleFactoryReset);

  server.onNotFound([]() {
    server.send(404, "text/plain", "404 - Not found");
  });

  server.begin();
  Serial.println("Webserver started");
}

void setupWiFiAP() {
  WiFi.mode(WIFI_AP);

  bool result = WiFi.softAP(AP_SSID, AP_PASSWORD);

  if (result) {
    Serial.println("WiFi Access Point started");
    Serial.print("SSID: ");
    Serial.println(AP_SSID);
    Serial.print("Password: ");
    Serial.println(AP_PASSWORD);
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("ERROR: WiFi Access Point failed!");
  }
}

// -----------------------------
// Serielle Steuerung
// -----------------------------
void handleSerialCommands() {
  if (!Serial.available()) {
    return;
  }

  char cmd = Serial.read();

  if (cmd == '0') {
    autoModeDemo = false;
    setMode(MODE_SCROLL_TEXT);
  } else if (cmd == '1') {
    autoModeDemo = false;
    setMode(MODE_LOGO_STATIC);
  } else if (cmd == '2') {
    autoModeDemo = false;
    setMode(MODE_PIXEL_ART);
  } else if (cmd == '3') {
    autoModeDemo = false;
    setMode(MODE_RANDOM_FX);
  } else if (cmd == 'a' || cmd == 'A') {
    autoModeDemo = true;
    lastModeChange = millis();
    saveModeSettings();
    Serial.println("Auto mode demo enabled");
  }
}

// -----------------------------
// Setup / Loop
// -----------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println("SMARTFIX MATRIX");
  Serial.print("Firmware v");
  Serial.println(FIRMWARE_VERSION);
  Serial.println("WiFi AP + Webinterface + Preferences enabled");
  Serial.println("================================");

  randomSeed(esp_random());

  loadSettings();

  initMatrix();

  Serial.println("Matrix initialized OK");

  setupWiFiAP();
  setupWebServer();

  setMode(currentMode, false);
  lastModeChange = millis();
}

void loop() {
  server.handleClient();

  handleSerialCommands();
  handleAutoModeDemo();

  switch (currentMode) {
    case MODE_SCROLL_TEXT:
      drawScrollingText();
      break;

    case MODE_LOGO_STATIC:
      drawLogoStatic();
      break;

    case MODE_PIXEL_ART:
      drawPixelArt();
      break;

    case MODE_RANDOM_FX:
      drawRandomFx();
      break;
  }
}