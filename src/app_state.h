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
  LOGO_EFFECT_PULSE      = 6
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
extern uint8_t logoEffectMode;
extern uint8_t logoColorMode;

// Home WiFi / OTA
extern bool homeWifiEnabled;
extern String homeWifiSsid;
extern String homeWifiPassword;
extern String otaUrl;
extern String lastOtaStatus;

const char *getModeName(DisplayMode mode);
const char *getSpeedName();
const char *getScrollTextColorName();
const char *getLogoEffectName();
const char *getLogoColorName();
int16_t getTextPixelWidth(const String &text);

void setMode(DisplayMode newMode, bool saveSetting = true);
