#pragma once

#include <Arduino.h>

// SmartFix Matrix firmware
#define FIRMWARE_VERSION "1.1.0"

// Current hardware: one 64x32 HUB75 panel.
// Later:
// - 2 panels side-by-side = 128x32: PANEL_CHAIN 2, PANEL_RES_X 64, PANEL_RES_Y 32
// - 2 panels stacked = usually special virtual-matrix mapping needed
// - 2x2 panels = 128x64, usually needs virtual matrix handling
#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 1

// Waveshare ESP32-S3-RGB-Matrix AP
static const char AP_SSID[] = "SmartFix-Matrix";
static const char AP_PASSWORD[] = "smartfix123";

// Preferences namespace
static const char PREF_NAMESPACE[] = "smartfix";

// Defaults
static const uint8_t DEFAULT_BRIGHTNESS = 70;
static const uint16_t DEFAULT_SCROLL_INTERVAL = 35;
static const uint16_t MAX_SCROLL_TEXT_LEN = 160;
static const uint16_t MAX_LOGO_TEXT_LEN = 32;

// OTA default URL.
// Replace in Web UI with your real GitHub release asset URL if needed.
static const char DEFAULT_OTA_URL[] =
  "https://github.com/acothebraco/SmartFix-Matrix/releases/latest/download/firmware.bin";
