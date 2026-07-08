#include "ota_update.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <NetworkClientSecure.h>
#include "config.h"
#include "app_state.h"
#include "settings.h"

static unsigned long lastAutoCheckMs = 0;

static String extractJsonString(const String &json, const String &key) {
  int keyPos = json.indexOf("\"" + key + "\"");
  if (keyPos < 0) return "";

  int colonPos = json.indexOf(':', keyPos);
  if (colonPos < 0) return "";

  int firstQuote = json.indexOf('"', colonPos + 1);
  if (firstQuote < 0) return "";

  int secondQuote = json.indexOf('"', firstQuote + 1);
  if (secondQuote < 0) return "";

  return json.substring(firstQuote + 1, secondQuote);
}

static String extractOtaAssetUrl(const String &json) {
  int searchPos = 0;

  while (true) {
    int namePos = json.indexOf("\"name\"", searchPos);
    if (namePos < 0) break;

    int urlPos = json.indexOf("\"browser_download_url\"", namePos);
    if (urlPos < 0) break;

    int nextNamePos = json.indexOf("\"name\"", namePos + 6);
    String block = json.substring(namePos, nextNamePos > 0 ? nextNamePos : json.length());

    if (block.indexOf("SmartFix-Matrix-ota.bin") >= 0) {
      return extractJsonString(block, "browser_download_url");
    }

    searchPos = urlPos + 1;
  }

  return "";
}

static int versionNumberAt(const String &version, uint8_t fieldIndex) {
  String v = version;
  v.trim();
  if (v.startsWith("v") || v.startsWith("V")) {
    v.remove(0, 1);
  }

  uint8_t currentField = 0;
  String part;

  for (uint16_t i = 0; i <= v.length(); i++) {
    char c = (i < v.length()) ? v[i] : '.';
    if (c == '.') {
      if (currentField == fieldIndex) {
        return part.toInt();
      }
      currentField++;
      part = "";
    } else if (c >= '0' && c <= '9') {
      part += c;
    }
  }

  return 0;
}

static bool isNewerVersion(const String &remoteVersion, const String &localVersion) {
  for (uint8_t i = 0; i < 3; i++) {
    int remote = versionNumberAt(remoteVersion, i);
    int local = versionNumberAt(localVersion, i);

    if (remote > local) return true;
    if (remote < local) return false;
  }

  return false;
}

void checkFirmwareUpdateFromGitHub(bool manualCheck) {
  if (WiFi.status() != WL_CONNECTED) {
    if (manualCheck) {
      lastOtaStatus = "Update Check Fehler: Home WiFi ist nicht verbunden.";
    }
    return;
  }

  NetworkClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(client, GITHUB_RELEASE_API_URL);
  http.addHeader("User-Agent", "SmartFix-Matrix");

  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    lastUpdateCheckText = "Fehler";
    if (manualCheck) {
      lastOtaStatus = "Update Check Fehler: HTTP " + String(httpCode);
    }
    http.end();
    return;
  }

  String payload = http.getString();
  http.end();

  String remoteVersion = extractJsonString(payload, "tag_name");
  String remoteOtaUrl = extractOtaAssetUrl(payload);

  if (remoteVersion.length() == 0) {
    lastUpdateCheckText = "Fehler";
    if (manualCheck) {
      lastOtaStatus = "Update Check Fehler: Keine Release Version gefunden.";
    }
    return;
  }

  latestFirmwareVersion = remoteVersion;
  latestFirmwareUrl = remoteOtaUrl.length() > 0 ? remoteOtaUrl : otaUrl;
  lastUpdateCheckText = "Gerade eben";
  firmwareUpdateAvailable = isNewerVersion(remoteVersion, String(FIRMWARE_VERSION));

  if (firmwareUpdateAvailable) {
    lastOtaStatus = "Neue Firmware verfuegbar: " + remoteVersion;
    if (remoteOtaUrl.length() > 0) {
      otaUrl = remoteOtaUrl;
      saveOtaSettings();
    }
  } else if (manualCheck) {
    lastOtaStatus = "Firmware ist aktuell: v" + String(FIRMWARE_VERSION);
  }

  Serial.print("Latest firmware: ");
  Serial.print(latestFirmwareVersion);
  Serial.print(" current: ");
  Serial.print(FIRMWARE_VERSION);
  Serial.print(" update available: ");
  Serial.println(firmwareUpdateAvailable ? "yes" : "no");
}

void handleOtaAutoCheck() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  unsigned long now = millis();
  if (lastAutoCheckMs == 0 || now - lastAutoCheckMs >= OTA_AUTO_CHECK_INTERVAL_MS) {
    lastAutoCheckMs = now;
    checkFirmwareUpdateFromGitHub(false);
  }
}

void startOtaUpdateFromSavedUrl() {
  if (WiFi.status() != WL_CONNECTED) {
    lastOtaStatus = "OTA Fehler: Home WiFi ist nicht verbunden.";
    Serial.println(lastOtaStatus);
    return;
  }

  if (otaUrl.length() == 0) {
    lastOtaStatus = "OTA Fehler: Keine Firmware URL gesetzt.";
    Serial.println(lastOtaStatus);
    return;
  }

  Serial.print("Starting OTA from: ");
  Serial.println(otaUrl);

  lastOtaStatus = "OTA gestartet. Bitte warten...";

  NetworkClientSecure client;
  client.setInsecure();

  httpUpdate.rebootOnUpdate(true);

  t_httpUpdate_return ret = httpUpdate.update(client, otaUrl);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      lastOtaStatus = "OTA Fehler: " + String(httpUpdate.getLastError()) + " " + httpUpdate.getLastErrorString();
      Serial.println(lastOtaStatus);
      break;

    case HTTP_UPDATE_NO_UPDATES:
      lastOtaStatus = "Keine neue Firmware gefunden.";
      Serial.println(lastOtaStatus);
      break;

    case HTTP_UPDATE_OK:
      lastOtaStatus = "OTA OK. Neustart...";
      Serial.println(lastOtaStatus);
      break;
  }
}
