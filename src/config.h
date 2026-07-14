#pragma once

#include <Arduino.h>

// DIY LED Matrix firmware
#define FIRMWARE_VERSION "1.4.13"

// Physical HUB75 panel setup.
// PANEL_CHAIN is the maximum number of 64x32 panels connected side-by-side.
// The active visible width can be selected in the Web Config: 1 panel = 64x32, 2 panels = 128x32.
#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define MIN_PANEL_COUNT 1
#define MAX_PANEL_COUNT 2
#define PANEL_CHAIN MAX_PANEL_COUNT
#define MAX_MATRIX_RES_X (PANEL_RES_X * PANEL_CHAIN)
#define MATRIX_RES_Y PANEL_RES_Y

// Waveshare ESP32-S3-RGB-Matrix AP
static const char AP_SSID[] = "DIY-LED-Matrix";
static const char AP_PASSWORD[] = "diyled123";

// mDNS name when connected to home WiFi: http://diyledmatrix.local/
static const char MDNS_HOSTNAME[] = "diyledmatrix";

// Preferences namespace
static const char PREF_NAMESPACE[] = "diyledmatrix";

// Defaults
static const uint8_t DEFAULT_BRIGHTNESS = 70;
static const uint16_t DEFAULT_SCROLL_INTERVAL = 35;
static const uint16_t DEFAULT_LOGO_INTERVAL = 35;
static const uint16_t MAX_SCROLL_TEXT_LEN = 160;
static const uint16_t MAX_LOGO_TEXT_LEN = 160;

// OTA default URL.
// This must point to the app-only OTA binary, not the full USB flash binary.
static const char DEFAULT_OTA_URL[] =
  "https://github.com/acothebraco/DIY-LED-Matrix/releases/latest/download/DIY-LED-Matrix-ota.bin";

// GitHub API endpoint used for update checks.
static const char GITHUB_RELEASE_API_URL[] =
  "https://api.github.com/repos/acothebraco/DIY-LED-Matrix/releases/latest";

// Automatic firmware update check interval.
static const unsigned long OTA_AUTO_CHECK_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL;
