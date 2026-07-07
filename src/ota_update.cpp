#include "ota_update.h"
#include <WiFi.h>
#include <HTTPUpdate.h>
#include <NetworkClientSecure.h>
#include "app_state.h"

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
