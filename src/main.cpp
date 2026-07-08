#include <Arduino.h>

#include "config.h"
#include "app_state.h"
#include "settings.h"
#include "matrix_display.h"
#include "wifi_manager.h"
#include "web_interface.h"
#include "animations.h"
#include "ota_update.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println("SMARTFIX MATRIX");
  Serial.print("Firmware v");
  Serial.println(FIRMWARE_VERSION);
  Serial.println("Modular firmware");
  Serial.println("================================");

  randomSeed(esp_random());

  loadSettings();

  initMatrix();
  Serial.println("Matrix initialized OK");

  setupWiFi();
  setupWebServer();

  setMode(currentMode, false);
  lastModeChange = millis();
}

void loop() {
  handleWebServer();
  handleWiFiReconnect();
  handleOtaAutoCheck();
  handleCurrentAnimation();
}
