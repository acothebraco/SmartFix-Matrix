#include "web_interface.h"

#include <WebServer.h>
#include <WiFi.h>
#include "config.h"
#include "app_state.h"
#include "settings.h"
#include "matrix_display.h"
#include "wifi_manager.h"
#include "ota_update.h"

static WebServer server(80);

static String htmlEscape(const String &input) {
  String output = input;
  output.replace("&", "&amp;");
  output.replace("\"", "&quot;");
  output.replace("'", "&#39;");
  output.replace("<", "&lt;");
  output.replace(">", "&gt;");
  return output;
}

static String htmlButton(const String &label, const String &url) {
  return "<a class='btn' href='" + url + "'>" + label + "</a>";
}

static void redirectHome() {
  server.sendHeader("Location", "/");
  server.send(303);
}

static String htmlPage() {
  String autoStatus = autoModeDemo ? "AKTIV" : "AUS";

  String page;
  page += "<!DOCTYPE html><html lang='de'><head>";
  page += "<meta charset='UTF-8'>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  page += "<title>SmartFix Matrix</title>";
  page += "<style>";
  page += "body{margin:0;font-family:Arial,Helvetica,sans-serif;background:#0b0f14;color:#e5e7eb;}";
  page += ".wrap{max-width:780px;margin:0 auto;padding:22px;}";
  page += ".card{background:#111827;border:1px solid #1f2937;border-radius:18px;padding:20px;margin-bottom:16px;box-shadow:0 10px 30px rgba(0,0,0,.35);}";
  page += "h1{margin:0 0 6px;font-size:28px;color:#22c55e;}";
  page += "h2{margin:0 0 14px;font-size:18px;color:#60a5fa;}";
  page += ".sub{color:#9ca3af;margin-bottom:18px;}";
  page += ".status{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:12px;}";
  page += ".pill{background:#020617;border:1px solid #334155;border-radius:12px;padding:10px;}";
  page += ".label{font-size:12px;color:#94a3b8;}";
  page += ".value{font-size:17px;font-weight:bold;color:#f8fafc;margin-top:4px;word-break:break-word;}";
  page += ".buttons{display:grid;grid-template-columns:1fr 1fr;gap:10px;}";
  page += ".btn{display:block;text-align:center;text-decoration:none;background:#2563eb;color:white;padding:14px;border-radius:12px;font-weight:bold;}";
  page += ".btn:hover{background:#1d4ed8;}";
  page += ".btn.green{background:#16a34a;}";
  page += ".btn.green:hover{background:#15803d;}";
  page += "input{width:100%;box-sizing:border-box;background:#020617;color:#e5e7eb;border:1px solid #334155;border-radius:12px;padding:14px;font-size:16px;margin-bottom:12px;}";
  page += "button.btn{border:0;width:100%;cursor:pointer;}";
  page += ".small{font-size:13px;color:#94a3b8;margin-top:18px;text-align:center;}";
  page += "</style>";
  page += "<script>";
  page += "window.addEventListener('beforeunload',function(){sessionStorage.setItem('sf_scroll',window.scrollY);});";
  page += "window.addEventListener('load',function(){var y=sessionStorage.getItem('sf_scroll');if(y!==null){window.scrollTo(0,parseInt(y));sessionStorage.removeItem('sf_scroll');}});";
  page += "</script>";
  page += "</head><body><div class='wrap'>";

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

  page += "<div class='pill'><div class='label'>Scroll Speed</div><div class='value'>";
  page += getSpeedName();
  page += " / ";
  page += String(scrollInterval);
  page += " ms</div></div>";

  page += "<div class='pill'><div class='label'>Textfarbe</div><div class='value'>";
  page += getScrollTextColorName();
  page += "</div></div>";

  page += "<div class='pill'><div class='label'>Logo Effekt</div><div class='value'>";
  page += getLogoEffectName();
  page += "</div></div>";

  page += "<div class='pill'><div class='label'>Home WiFi</div><div class='value'>";
  page += getWiFiStatusText();
  page += "<br>";
  page += htmlEscape(getStaIpText());
  page += "</div></div>";

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
  page += "<input name='t' maxlength='160' value='" + htmlEscape(scrollText) + "'>";
  page += "<button class='btn green' type='submit'>Text speichern</button>";
  page += "</form>";
  page += "<div class='sub' style='margin-top:12px;'>Maximal 160 Zeichen. Umlaute testen wir sp&auml;ter mit eigener Font-Unterst&uuml;tzung.</div>";
  page += "</div>";

  page += "<div class='card' id='logo-text'>";
  page += "<h2>Logo Text</h2>";
  page += "<form action='/set-logo-text' method='GET'>";
  page += "<input name='t' maxlength='32' value='" + htmlEscape(logoText) + "'>";
  page += "<button class='btn green' type='submit'>Logo Text speichern</button>";
  page += "</form>";
  page += "<div class='sub' style='margin-top:12px;'>Bei SmartFix wird eine eigene lesbare Pixel-Font mit gr&uuml;n/blauem Branding verwendet.</div>";
  page += "</div>";

  page += "<div class='card'>";
  page += "<h2>Logo Effekt</h2>";
  page += "<div class='buttons'>";
  page += htmlButton("Statisch", "/logo-effect?e=0");
  page += htmlButton("Buchstabe f&uuml;r Buchstabe", "/logo-effect?e=1");
  page += htmlButton("Fade In / Out", "/logo-effect?e=2");
  page += htmlButton("Slide In", "/logo-effect?e=3");
  page += htmlButton("Shimmer", "/logo-effect?e=4");
  page += htmlButton("Sparkle", "/logo-effect?e=5");
  page += htmlButton("Pulse Glow", "/logo-effect?e=6");
  page += htmlButton("Refresh", "/");
  page += "</div>";
  page += "<div class='sub' style='margin-top:12px;'>Effekte betreffen nur den oberen Logo/Header. Farben bleiben Smart=gr&uuml;n und Fix=blau.</div>";
  page += "</div>";

  page += "<div class='card'>";
  page += "<h2>Auto Demo</h2>";
  page += "<div class='buttons'>";
  page += "<a class='btn green' href='/auto'>Auto Demo starten</a>";
  page += htmlButton("Refresh", "/");
  page += "</div></div>";

  page += "<div class='card'>";
  page += "<h2>Laufschrift Geschwindigkeit</h2>";
  page += "<div class='buttons'>";
  page += htmlButton("Langsam", "/speed?v=70");
  page += htmlButton("Mittel", "/speed?v=35");
  page += htmlButton("Schnell", "/speed?v=18");
  page += htmlButton("Turbo", "/speed?v=8");
  page += "</div>";
  page += "<div class='sub' style='margin-top:12px;'>Kleiner Wert = schnellere Laufschrift.</div>";
  page += "</div>";

  page += "<div class='card'>";
  page += "<h2>Laufschrift Farbe</h2>";
  page += "<div class='buttons'>";
  page += htmlButton("Weiss", "/text-color?c=0");
  page += htmlButton("Gruen", "/text-color?c=1");
  page += htmlButton("Blau", "/text-color?c=2");
  page += htmlButton("Gelb", "/text-color?c=3");
  page += htmlButton("Rot", "/text-color?c=4");
  page += htmlButton("Refresh", "/");
  page += "</div>";
  page += "<div class='sub' style='margin-top:12px;'>Farbe der laufenden Textzeile.</div>";
  page += "</div>";

  page += "<div class='card'>";
  page += "<h2>Helligkeit</h2>";
  page += "<div class='buttons'>";
  page += htmlButton("25%", "/brightness?v=40");
  page += htmlButton("50%", "/brightness?v=80");
  page += htmlButton("75%", "/brightness?v=130");
  page += htmlButton("100%", "/brightness?v=200");
  page += "</div></div>";

  page += "<div class='card'>";
  page += "<h2>Heim WLAN</h2>";
  page += "<form action='/wifi-save' method='POST'>";
  page += "<input name='ssid' maxlength='64' placeholder='SSID' value='" + htmlEscape(homeWifiSsid) + "'>";
  page += "<input name='pass' maxlength='64' placeholder='WLAN Passwort' type='password' value='" + htmlEscape(homeWifiPassword) + "'>";
  page += "<button class='btn green' type='submit'>Mit Heim WLAN verbinden</button>";
  page += "</form>";
  page += "<div class='buttons' style='margin-top:10px;'>";
  page += htmlButton("Heim WLAN l&ouml;schen", "/wifi-forget");
  page += htmlButton("Refresh", "/");
  page += "</div>";
  page += "<div class='sub' style='margin-top:12px;'>Der SmartFix-Matrix Access Point bleibt zus&auml;tzlich aktiv.</div>";
  page += "</div>";

  page += "<div class='card'>";
  page += "<h2>GitHub OTA Firmware</h2>";
  page += "<form action='/ota-save' method='POST'>";
  page += "<input name='url' maxlength='220' value='" + htmlEscape(otaUrl) + "'>";
  page += "<button class='btn green' type='submit'>OTA URL speichern</button>";
  page += "</form>";
  page += "<div class='buttons' style='margin-top:10px;'>";
  page += htmlButton("OTA Update starten", "/ota-start");
  page += htmlButton("Refresh", "/");
  page += "</div>";
  page += "<div class='sub' style='margin-top:12px;'>Status: ";
  page += htmlEscape(lastOtaStatus);
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

static void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

static void handleModeChange() {
  if (server.hasArg("m")) {
    int mode = server.arg("m").toInt();

    if (mode >= MODE_SCROLL_TEXT && mode <= MODE_RANDOM_FX) {
      autoModeDemo = false;
      setMode((DisplayMode)mode, true);
    }
  }

  redirectHome();
}

static void handleAutoDemo() {
  autoModeDemo = true;
  lastModeChange = millis();
  saveModeSettings();
  Serial.println("Auto mode demo enabled from web");
  redirectHome();
}

static void handleBrightness() {
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

static void handleTextColor() {
  if (server.hasArg("c")) {
    int value = server.arg("c").toInt();

    if (value < 0) value = 0;
    if (value > 4) value = 4;

    scrollTextColorMode = (uint8_t)value;
    saveTextColorSetting();

    autoModeDemo = false;
    setMode(MODE_SCROLL_TEXT, true);

    Serial.print("Text color changed to: ");
    Serial.println(getScrollTextColorName());
  }

  redirectHome();
}

static void handleLogoEffect() {
  if (server.hasArg("e")) {
    int value = server.arg("e").toInt();

    if (value < LOGO_EFFECT_STATIC) value = LOGO_EFFECT_STATIC;
    if (value > LOGO_EFFECT_PULSE) value = LOGO_EFFECT_PULSE;

    logoEffectMode = (uint8_t)value;
    saveLogoEffectSetting();

    clearDisplay();

    Serial.print("Logo effect changed to: ");
    Serial.println(getLogoEffectName());
  }

  redirectHome();
}

static void handleSpeed() {
  if (server.hasArg("v")) {
    int value = server.arg("v").toInt();

    if (value < 5) value = 5;
    if (value > 200) value = 200;

    scrollInterval = (uint16_t)value;
    saveSpeedSetting();

    autoModeDemo = false;
    setMode(MODE_SCROLL_TEXT, true);

    Serial.print("Scroll speed changed to: ");
    Serial.print(scrollInterval);
    Serial.println(" ms");
  }

  redirectHome();
}

static void handleSetText() {
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

    saveScrollTextSetting();

    autoModeDemo = false;
    setMode(MODE_SCROLL_TEXT, true);

    Serial.print("New scroll text from web: ");
    Serial.println(scrollText);
  }

  redirectHome();
}

static void handleSetLogoText() {
  if (server.hasArg("t")) {
    String newLogoText = server.arg("t");

    newLogoText.trim();

    if (newLogoText.length() == 0) {
      newLogoText = "SmartFix";
    }

    if (newLogoText.length() > MAX_LOGO_TEXT_LEN) {
      newLogoText = newLogoText.substring(0, MAX_LOGO_TEXT_LEN);
    }

    logoText = newLogoText;

    saveLogoTextSetting();
    clearDisplay();

    Serial.print("New logo text from web: ");
    Serial.println(logoText);
  }

  redirectHome();
}

static void handleWifiSave() {
  if (server.hasArg("ssid")) {
    homeWifiSsid = server.arg("ssid");
    homeWifiSsid.trim();

    homeWifiPassword = server.arg("pass");
    homeWifiEnabled = homeWifiSsid.length() > 0;

    saveWiFiSettings();

    server.send(200, "text/html",
                "<html><body style='background:#0b0f14;color:white;font-family:Arial;text-align:center;padding-top:40px;'>"
                "<h1>SmartFix Matrix</h1>"
                "<p>WLAN gespeichert. Neustart...</p>"
                "</body></html>");

    delay(1000);
    ESP.restart();
    return;
  }

  redirectHome();
}

static void handleWifiForget() {
  homeWifiEnabled = false;
  homeWifiSsid = "";
  homeWifiPassword = "";
  saveWiFiSettings();
  disconnectHomeWiFi();

  server.send(200, "text/html",
              "<html><body style='background:#0b0f14;color:white;font-family:Arial;text-align:center;padding-top:40px;'>"
              "<h1>SmartFix Matrix</h1>"
              "<p>Heim WLAN gel&ouml;scht. Neustart...</p>"
              "</body></html>");

  delay(1000);
  ESP.restart();
}

static void handleOtaSave() {
  if (server.hasArg("url")) {
    otaUrl = server.arg("url");
    otaUrl.trim();

    if (otaUrl.length() == 0) {
      otaUrl = DEFAULT_OTA_URL;
    }

    saveOtaSettings();
  }

  redirectHome();
}

static void handleOtaStart() {
  server.send(200, "text/html",
              "<html><body style='background:#0b0f14;color:white;font-family:Arial;text-align:center;padding-top:40px;'>"
              "<h1>SmartFix Matrix OTA</h1>"
              "<p>OTA Update wurde gestartet.</p>"
              "<p>Bitte warten. Bei Erfolg startet das Ger&auml;t automatisch neu.</p>"
              "</body></html>");

  delay(500);
  startOtaUpdateFromSavedUrl();
}

static void handleFactoryReset() {
  factoryResetSettings();

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
  server.on("/", HTTP_GET, handleRoot);
  server.on("/mode", HTTP_GET, handleModeChange);
  server.on("/auto", HTTP_GET, handleAutoDemo);
  server.on("/brightness", HTTP_GET, handleBrightness);
  server.on("/speed", HTTP_GET, handleSpeed);
  server.on("/text-color", HTTP_GET, handleTextColor);
  server.on("/logo-effect", HTTP_GET, handleLogoEffect);
  server.on("/set-text", HTTP_GET, handleSetText);
  server.on("/set-logo-text", HTTP_GET, handleSetLogoText);
  server.on("/wifi-save", HTTP_POST, handleWifiSave);
  server.on("/wifi-forget", HTTP_GET, handleWifiForget);
  server.on("/ota-save", HTTP_POST, handleOtaSave);
  server.on("/ota-start", HTTP_GET, handleOtaStart);
  server.on("/factory-reset", HTTP_GET, handleFactoryReset);

  server.onNotFound([]() {
    server.send(404, "text/plain", "404 - Not found");
  });

  server.begin();
  Serial.println("Webserver started");
}

void handleWebServer() {
  server.handleClient();
}
