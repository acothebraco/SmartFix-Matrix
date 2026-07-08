#pragma once

#include <Arduino.h>

void startOtaUpdateFromSavedUrl();
void checkFirmwareUpdateFromGitHub(bool manualCheck = false);
void handleOtaAutoCheck();
