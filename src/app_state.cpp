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
uint8_t scrollTextEffectMode = SCROLL_EFFECT_NORMAL;
uint8_t logoEffectMode = LOGO_EFFECT_STATIC;
uint8_t logoColorMode = LOGO_COLOR_BRAND;

bool homeWifiEnabled = false;
String homeWifiSsid = "";
String homeWifiPassword = "";
String otaUrl = DEFAULT_OTA_URL;
String uiLanguage = "de";
String lastOtaStatus = "Noch kein OTA Update gestartet.";
String latestFirmwareVersion = "-";
String latestFirmwareUrl = "";
String lastUpdateCheckText = "Noch nie";
bool firmwareUpdateAvailable = false;

int16_t getTextPixelWidth(const String &text) {
  return getMatrixTextPixelWidth(text);
}

bool isGermanUi() {
  return !uiLanguage.equalsIgnoreCase("en");
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
    case 0: return "WEIß";
    case 1: return "GRÜN";
    case 2: return "BLAU";
    case 3: return "GELB";
    case 4: return "ROT";
    default: return "WEIß";
  }
}

const char *getScrollTextEffectName() {
  switch (scrollTextEffectMode) {
    case SCROLL_EFFECT_NORMAL:  return "NORMAL";
    case SCROLL_EFFECT_RAINBOW: return "RAINBOW";
    case SCROLL_EFFECT_WAVE:    return "WAVE";
    case SCROLL_EFFECT_SPARKLE: return "SPARKLE";
    case SCROLL_EFFECT_COMET:   return "COMET";
    case SCROLL_EFFECT_FLASH:      return "FLASH";
    case SCROLL_EFFECT_DUAL_SLIDE: return "2-WAY SLIDE";
    default:                       return "NORMAL";
  }
}

const char *getLogoColorName() {
  switch (logoColorMode) {
    case LOGO_COLOR_BRAND:     return "AUTO/BRAND";
    case LOGO_COLOR_GREEN:     return "GRÜN";
    case LOGO_COLOR_BLUE:      return "BLAU";
    case LOGO_COLOR_WHITE:     return "WEIß";
    case LOGO_COLOR_YELLOW:    return "GELB";
    case LOGO_COLOR_RED:       return "ROT";
    case LOGO_COLOR_TWO_WORDS: return "2-FARBIG";
    case LOGO_COLOR_RAINBOW:   return "MEHRFARBIG";
    default:                   return "AUTO/BRAND";
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
    case LOGO_EFFECT_WAVE:       return "WAVE";
    case LOGO_EFFECT_BOUNCE:     return "BOUNCE";
    case LOGO_EFFECT_GLITCH:     return "GLITCH";
    case LOGO_EFFECT_SCANLINE:   return "SCANLINE";
    case LOGO_EFFECT_DUAL_SLIDE: return "2-WAY SLIDE";
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
