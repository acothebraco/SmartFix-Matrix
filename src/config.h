#pragma once

#include <Arduino.h>

// SmartFix Matrix firmware
#define FIRMWARE_VERSION "1.4.7"

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

// mDNS name when connected to home WiFi: http://smartfixmatrix.local/
static const char MDNS_HOSTNAME[] = "smartfixmatrix";

// Preferences namespace
static const char PREF_NAMESPACE[] = "smartfix";

// Defaults
static const uint8_t DEFAULT_BRIGHTNESS = 70;
static const uint16_t DEFAULT_SCROLL_INTERVAL = 35;
static const uint16_t MAX_SCROLL_TEXT_LEN = 160;
static const uint16_t MAX_LOGO_TEXT_LEN = 160;

// OTA default URL.
// This must point to the app-only OTA binary, not the full USB flash binary.
static const char DEFAULT_OTA_URL[] =
  "https://github.com/acothebraco/SmartFix-Matrix/releases/latest/download/SmartFix-Matrix-ota.bin";

// GitHub API endpoint used for update checks.
static const char GITHUB_RELEASE_API_URL[] =
  "https://api.github.com/repos/acothebraco/SmartFix-Matrix/releases/latest";

// Automatic firmware update check interval.
static const unsigned long OTA_AUTO_CHECK_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL;
