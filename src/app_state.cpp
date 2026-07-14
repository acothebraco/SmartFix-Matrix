#include "app_state.h"
#include "matrix_display.h"
#include "settings.h"
#include "animations.h"

DisplayMode currentMode = MODE_SCROLL_TEXT;

bool autoModeDemo = true;
unsigned long lastModeChange = 0;
const unsigned long modeInterval = 10000;

String scrollText = "DIY LED MATRIX  -  PIXEL ART  -  ESP32  -  HUB75  ";
String logoText = "DIY LED Matrix";

uint8_t panelCount = MAX_PANEL_COUNT;
uint8_t matrixBrightness = DEFAULT_BRIGHTNESS;
uint16_t scrollInterval = DEFAULT_SCROLL_INTERVAL;
uint16_t logoInterval = DEFAULT_LOGO_INTERVAL;
uint8_t scrollFontSize = 1;
uint8_t logoFontSize = 1;
uint8_t scrollFontStyle = FONT_STYLE_CLASSIC;
uint8_t logoFontStyle = FONT_STYLE_CLASSIC;
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

int16_t getMatrixWidth() {
  uint8_t count = panelCount;
  if (count < MIN_PANEL_COUNT) count = MIN_PANEL_COUNT;
  if (count > MAX_PANEL_COUNT) count = MAX_PANEL_COUNT;
  return PANEL_RES_X * count;
}

int16_t getMatrixHeight() {
  return PANEL_RES_Y;
}

const char *getPanelLayoutName() {
  switch (panelCount) {
    case 1: return "64x32";
    case 2: return "128x32";
    default: return "UNKNOWN";
  }
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


const char *getLogoSpeedName() {
  if (logoInterval >= 60) {
    return "LANGSAM";
  }
  if (logoInterval <= 20) {
    return "SCHNELL";
  }
  return "MITTEL";
}

static const char *fontSizeName(uint8_t size) {
  return size >= 2 ? "GROSS" : "KLEIN";
}

const char *getScrollFontSizeName() {
  return fontSizeName(scrollFontSize);
}

const char *getLogoFontSizeName() {
  return fontSizeName(logoFontSize);
}

static const char *fontStyleName(uint8_t style) {
  switch (style) {
    case FONT_STYLE_BOLD:  return "FETT";
    case FONT_STYLE_WIDE:  return "BREIT";
    case FONT_STYLE_BLOCK: return "BLOCK";
    case FONT_STYLE_CLASSIC:
    default:               return "KLASSISCH";
  }
}

const char *getScrollFontStyleName() {
  return fontStyleName(scrollFontStyle);
}

const char *getLogoFontStyleName() {
  return fontStyleName(logoFontStyle);
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

void setPanelCount(uint8_t newPanelCount, bool saveSetting) {
  if (newPanelCount < MIN_PANEL_COUNT) newPanelCount = MIN_PANEL_COUNT;
  if (newPanelCount > MAX_PANEL_COUNT) newPanelCount = MAX_PANEL_COUNT;

  panelCount = newPanelCount;
  clearDisplay();
  resetAnimationState();

  Serial.print("Panel layout changed to: ");
  Serial.println(getPanelLayoutName());

  if (saveSetting) {
    savePanelSettings();
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
