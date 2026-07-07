#include "wifi_manager.h"
#include "config.h"
#include "app_state.h"

static unsigned long lastReconnectAttempt = 0;
static const unsigned long reconnectInterval = 30000;

void setupWiFi() {
  WiFi.mode(WIFI_AP_STA);

  bool apOk = WiFi.softAP(AP_SSID, AP_PASSWORD);

  if (apOk) {
    Serial.println("WiFi Access Point started");
    Serial.print("SSID: ");
    Serial.println(AP_SSID);
    Serial.print("Password: ");
    Serial.println(AP_PASSWORD);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("ERROR: WiFi Access Point failed!");
  }

  if (homeWifiEnabled && homeWifiSsid.length() > 0) {
    Serial.print("Connecting to home WiFi: ");
    Serial.println(homeWifiSsid);

    WiFi.begin(homeWifiSsid.c_str(), homeWifiPassword.c_str());
  }
}

void handleWiFiReconnect() {
  if (!homeWifiEnabled || homeWifiSsid.length() == 0) {
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  unsigned long now = millis();
  if (now - lastReconnectAttempt >= reconnectInterval) {
    lastReconnectAttempt = now;
    Serial.println("Reconnecting to home WiFi...");
    WiFi.disconnect(false);
    WiFi.begin(homeWifiSsid.c_str(), homeWifiPassword.c_str());
  }
}

String getWiFiStatusText() {
  if (!homeWifiEnabled) {
    return "Home WiFi AUS";
  }

  if (WiFi.status() == WL_CONNECTED) {
    return "Verbunden";
  }

  return "Nicht verbunden";
}

String getStaIpText() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }

  return "-";
}

void disconnectHomeWiFi() {
  WiFi.disconnect(false, true);
}
