#pragma once

#include <Arduino.h>
#include "config.h"

enum DisplayMode {
  MODE_SCROLL_TEXT = 0,
  MODE_LOGO_STATIC = 1,
  MODE_PIXEL_ART   = 2,
  MODE_RANDOM_FX   = 3
};

enum LogoEffectMode {
  LOGO_EFFECT_STATIC     = 0,
  LOGO_EFFECT_TYPEWRITER = 1,
  LOGO_EFFECT_FADE       = 2,
  LOGO_EFFECT_SLIDE      = 3,
  LOGO_EFFECT_SHIMMER    = 4,
  LOGO_EFFECT_SPARKLE    = 5,
  LOGO_EFFECT_PULSE      = 6,
  LOGO_EFFECT_WAVE       = 7,
  LOGO_EFFECT_BOUNCE     = 8,
  LOGO_EFFECT_GLITCH     = 9,
  LOGO_EFFECT_SCANLINE   = 10,
  LOGO_EFFECT_DUAL_SLIDE = 11
};

enum LogoColorMode {
  LOGO_COLOR_BRAND      = 0,  // Smart=green, Fix=blue. Generic text alternates words green/blue.
  LOGO_COLOR_GREEN      = 1,
  LOGO_COLOR_BLUE       = 2,
  LOGO_COLOR_WHITE      = 3,
  LOGO_COLOR_YELLOW     = 4,
  LOGO_COLOR_RED        = 5,
  LOGO_COLOR_TWO_WORDS  = 6,  // word 1 green, word 2 blue, repeat
  LOGO_COLOR_RAINBOW    = 7   // green/blue/yellow/red/white by word
};

enum ScrollTextEffectMode {
  SCROLL_EFFECT_NORMAL  = 0,
  SCROLL_EFFECT_RAINBOW = 1,
  SCROLL_EFFECT_WAVE    = 2,
  SCROLL_EFFECT_SPARKLE = 3,
  SCROLL_EFFECT_COMET   = 4,
  SCROLL_EFFECT_FLASH      = 5,
  SCROLL_EFFECT_DUAL_SLIDE = 6
};

// Current app state
extern DisplayMode currentMode;
extern bool autoModeDemo;
extern unsigned long lastModeChange;
extern const unsigned long modeInterval;

// Text / logo
extern String scrollText;
extern String logoText;

// Display settings
extern uint8_t matrixBrightness;
extern uint16_t scrollInterval;
extern uint8_t scrollTextColorMode;
extern uint8_t scrollTextEffectMode;
extern uint8_t logoEffectMode;
extern uint8_t logoColorMode;

// Web UI language
extern String uiLanguage;
bool isGermanUi();

// Home WiFi / OTA
extern bool homeWifiEnabled;
extern String homeWifiSsid;
extern String homeWifiPassword;
extern String otaUrl;
extern String lastOtaStatus;
extern String latestFirmwareVersion;
extern String latestFirmwareUrl;
extern String lastUpdateCheckText;
extern bool firmwareUpdateAvailable;

const char *getModeName(DisplayMode mode);
const char *getSpeedName();
const char *getScrollTextColorName();
const char *getScrollTextEffectName();
const char *getLogoEffectName();
const char *getLogoColorName();
int16_t getTextPixelWidth(const String &text);

void setMode(DisplayMode newMode, bool saveSetting = true);
