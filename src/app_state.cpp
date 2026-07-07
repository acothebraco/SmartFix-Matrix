#include "app_state.h"
#include "matrix_display.h"
#include "settings.h"
#include "animations.h"

DisplayMode currentMode = MODE_SCROLL_TEXT;

bool autoModeDemo = true;
unsigned long lastModeChange = 0;
const unsigned long modeInterval = 10000;

String scrollText = "ELEKTRONIKSERVICE  -  REPARATUR  -  KONSOLEN  -  SMARTFIX  ";
String logoText = "SmartFix";

uint8_t matrixBrightness = DEFAULT_BRIGHTNESS;
uint16_t scrollInterval = DEFAULT_SCROLL_INTERVAL;
uint8_t scrollTextColorMode = 0; // 0=White, 1=Green, 2=Blue, 3=Yellow, 4=Red
uint8_t logoEffectMode = LOGO_EFFECT_STATIC;

bool homeWifiEnabled = false;
String homeWifiSsid = "";
String homeWifiPassword = "";
String otaUrl = DEFAULT_OTA_URL;
String lastOtaStatus = "Noch kein OTA Update gestartet.";

int16_t getTextPixelWidth(const String &text) {
  return text.length() * 6;
}

const char *getModeName(DisplayMode mode) {
  switch (mode) {
    case MODE_SCROLL_TEXT: return "SCROLL_TEXT";
    case MODE_LOGO_STATIC: return "LOGO_STATIC";
    case MODE_PIXEL_ART:   return "PIXEL_ART";
    case MODE_RANDOM_FX:   return "RANDOM_FX";
    default:               return "UNKNOWN";
  }
}

const char *getSpeedName() {
  if (scrollInterval >= 60) {
    return "LANGSAM";
  }
  if (scrollInterval <= 20) {
    return "SCHNELL";
  }
  return "MITTEL";
}

const char *getScrollTextColorName() {
  switch (scrollTextColorMode) {
    case 0: return "WEISS";
    case 1: return "GRUEN";
    case 2: return "BLAU";
    case 3: return "GELB";
    case 4: return "ROT";
    default: return "WEISS";
  }
}

const char *getLogoEffectName() {
  switch (logoEffectMode) {
    case LOGO_EFFECT_STATIC:     return "STATISCH";
    case LOGO_EFFECT_TYPEWRITER: return "BUCHSTABE";
    case LOGO_EFFECT_FADE:       return "FADE";
    case LOGO_EFFECT_SLIDE:      return "SLIDE";
    case LOGO_EFFECT_SHIMMER:    return "SHIMMER";
    case LOGO_EFFECT_SPARKLE:    return "SPARKLE";
    case LOGO_EFFECT_PULSE:      return "PULSE";
    default:                     return "STATISCH";
  }
}

void setMode(DisplayMode newMode, bool saveSetting) {
  currentMode = newMode;

  clearDisplay();
  resetAnimationState();

  Serial.print("Mode changed to: ");
  Serial.println(getModeName(currentMode));

  if (saveSetting) {
    saveModeSettings();
  }
}
