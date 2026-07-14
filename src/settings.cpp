#include "settings.h"
#include <Preferences.h>
#include "config.h"
#include "app_state.h"

static Preferences prefs;

void loadSettings() {
  prefs.begin(PREF_NAMESPACE, false);

  matrixBrightness = prefs.getUChar("bright", DEFAULT_BRIGHTNESS);

  panelCount = prefs.getUChar("panels", MAX_PANEL_COUNT);
  if (panelCount < MIN_PANEL_COUNT || panelCount > MAX_PANEL_COUNT) {
    panelCount = MAX_PANEL_COUNT;
  }

  uiLanguage = prefs.getString("uiLang", "de");
  uiLanguage.toLowerCase();
  if (uiLanguage != "de" && uiLanguage != "en") {
    uiLanguage = "de";
  }

  scrollTextColorMode = prefs.getUChar("txtColor", 0);
  if (scrollTextColorMode > 4) {
    scrollTextColorMode = 0;
  }

  scrollTextEffectMode = prefs.getUChar("scrollFx", SCROLL_EFFECT_NORMAL);
  if (scrollTextEffectMode > SCROLL_EFFECT_DUAL_SLIDE) {
    scrollTextEffectMode = SCROLL_EFFECT_NORMAL;
  }

  logoEffectMode = prefs.getUChar("logoFx", LOGO_EFFECT_STATIC);
  if (logoEffectMode > LOGO_EFFECT_DUAL_SLIDE) {
    logoEffectMode = LOGO_EFFECT_STATIC;
  }

  logoColorMode = prefs.getUChar("logoColor", LOGO_COLOR_BRAND);
  if (logoColorMode > LOGO_COLOR_RAINBOW) {
    logoColorMode = LOGO_COLOR_BRAND;
  }

  scrollInterval = prefs.getUShort("speed", DEFAULT_SCROLL_INTERVAL);
  if (scrollInterval < 5 || scrollInterval > 200) {
    scrollInterval = DEFAULT_SCROLL_INTERVAL;
  }

  logoInterval = prefs.getUShort("logoSpeed", scrollInterval);
  if (logoInterval < 5 || logoInterval > 200) {
    logoInterval = DEFAULT_LOGO_INTERVAL;
  }

  scrollFontSize = prefs.getUChar("scrollFontSz", 1);
  if (scrollFontSize < 1 || scrollFontSize > 2) {
    scrollFontSize = 1;
  }

  logoFontSize = prefs.getUChar("logoFontSz", 1);
  if (logoFontSize < 1 || logoFontSize > 2) {
    logoFontSize = 1;
  }

  scrollFontStyle = prefs.getUChar("scrollFont", FONT_STYLE_CLASSIC);
  if (scrollFontStyle > FONT_STYLE_BLOCK) {
    scrollFontStyle = FONT_STYLE_CLASSIC;
  }

  logoFontStyle = prefs.getUChar("logoFont", FONT_STYLE_CLASSIC);
  if (logoFontStyle > FONT_STYLE_BLOCK) {
    logoFontStyle = FONT_STYLE_CLASSIC;
  }

  scrollText = prefs.getString("text", scrollText);
  if (scrollText.length() == 0) {
    scrollText = "DIY LED MATRIX  -  PIXEL ART  -  ESP32  -  HUB75  ";
  }
  if (scrollText.length() > MAX_SCROLL_TEXT_LEN) {
    scrollText = scrollText.substring(0, MAX_SCROLL_TEXT_LEN);
  }

  logoText = prefs.getString("logoText", logoText);
  logoText.trim();
  if (logoText.length() == 0) {
    logoText = "DIY LED Matrix";
  }
  if (logoText.length() > MAX_LOGO_TEXT_LEN) {
    logoText = logoText.substring(0, MAX_LOGO_TEXT_LEN);
  }

  int savedMode = prefs.getInt("mode", MODE_SCROLL_TEXT);
  if (savedMode < MODE_SCROLL_TEXT || savedMode > MODE_RANDOM_FX) {
    savedMode = MODE_SCROLL_TEXT;
  }
  currentMode = (DisplayMode)savedMode;

  autoModeDemo = prefs.getBool("auto", true);

  homeWifiEnabled = prefs.getBool("wifiEn", false);
  homeWifiSsid = prefs.getString("wifiSsid", "");
  homeWifiPassword = prefs.getString("wifiPass", "");

  otaUrl = prefs.getString("otaUrl", DEFAULT_OTA_URL);
  if (otaUrl.length() == 0) {
    otaUrl = DEFAULT_OTA_URL;
  }

  Serial.println("Settings loaded:");
  Serial.print("Brightness: ");
  Serial.println(matrixBrightness);
  Serial.print("Panel layout: ");
  Serial.println(getPanelLayoutName());
  Serial.print("UI Language: ");
  Serial.println(uiLanguage);
  Serial.print("Text Color: ");
  Serial.println(getScrollTextColorName());
  Serial.print("Scroll Effect: ");
  Serial.println(getScrollTextEffectName());
  Serial.print("Logo Effect: ");
  Serial.println(getLogoEffectName());
  Serial.print("Logo Color: ");
  Serial.println(getLogoColorName());
  Serial.print("Scroll Speed: ");
  Serial.print(scrollInterval);
  Serial.println(" ms");
  Serial.print("Logo Speed: ");
  Serial.print(logoInterval);
  Serial.println(" ms");
  Serial.print("Scroll Font: ");
  Serial.print(getScrollFontSizeName());
  Serial.print(" / ");
  Serial.println(getScrollFontStyleName());
  Serial.print("Logo Font: ");
  Serial.print(getLogoFontSizeName());
  Serial.print(" / ");
  Serial.println(getLogoFontStyleName());
  Serial.print("Scroll Text: ");
  Serial.println(scrollText);
  Serial.print("Logo Text: ");
  Serial.println(logoText);
  Serial.print("Mode: ");
  Serial.println(getModeName(currentMode));
  Serial.print("Auto Demo: ");
  Serial.println(autoModeDemo ? "ON" : "OFF");
  Serial.print("Home WiFi: ");
  Serial.println(homeWifiEnabled ? homeWifiSsid : "disabled");
}

void saveModeSettings() {
  prefs.putInt("mode", (int)currentMode);
  prefs.putBool("auto", autoModeDemo);
  Serial.println("Mode settings saved");
}

void saveLanguageSetting() {
  prefs.putString("uiLang", uiLanguage);
  Serial.print("UI language saved: ");
  Serial.println(uiLanguage);
}

void saveBrightnessSetting() {
  prefs.putUChar("bright", matrixBrightness);
  Serial.print("Brightness saved: ");
  Serial.println(matrixBrightness);
}

void savePanelSettings() {
  prefs.putUChar("panels", panelCount);
  Serial.print("Panel layout saved: ");
  Serial.println(getPanelLayoutName());
}

void saveSpeedSetting() {
  prefs.putUShort("speed", scrollInterval);
  Serial.print("Scroll speed saved: ");
  Serial.print(scrollInterval);
  Serial.println(" ms");
}

void saveLogoSpeedSetting() {
  prefs.putUShort("logoSpeed", logoInterval);
  Serial.print("Logo speed saved: ");
  Serial.print(logoInterval);
  Serial.println(" ms");
}

void saveScrollFontSetting() {
  prefs.putUChar("scrollFontSz", scrollFontSize);
  prefs.putUChar("scrollFont", scrollFontStyle);
  Serial.print("Scroll font saved: ");
  Serial.print(getScrollFontSizeName());
  Serial.print(" / ");
  Serial.println(getScrollFontStyleName());
}

void saveLogoFontSetting() {
  prefs.putUChar("logoFontSz", logoFontSize);
  prefs.putUChar("logoFont", logoFontStyle);
  Serial.print("Logo font saved: ");
  Serial.print(getLogoFontSizeName());
  Serial.print(" / ");
  Serial.println(getLogoFontStyleName());
}

void saveTextColorSetting() {
  prefs.putUChar("txtColor", scrollTextColorMode);
  Serial.print("Text color saved: ");
  Serial.println(getScrollTextColorName());
}

void saveScrollEffectSetting() {
  prefs.putUChar("scrollFx", scrollTextEffectMode);
  Serial.print("Scroll effect saved: ");
  Serial.println(getScrollTextEffectName());
}

void saveLogoEffectSetting() {
  prefs.putUChar("logoFx", logoEffectMode);
  Serial.print("Logo effect saved: ");
  Serial.println(getLogoEffectName());
}

void saveLogoColorSetting() {
  prefs.putUChar("logoColor", logoColorMode);
  Serial.print("Logo color saved: ");
  Serial.println(getLogoColorName());
}

void saveScrollTextSetting() {
  prefs.putString("text", scrollText);
  Serial.print("Scroll text saved: ");
  Serial.println(scrollText);
}

void saveLogoTextSetting() {
  prefs.putString("logoText", logoText);
  Serial.print("Logo text saved: ");
  Serial.println(logoText);
}

void saveWiFiSettings() {
  prefs.putBool("wifiEn", homeWifiEnabled);
  prefs.putString("wifiSsid", homeWifiSsid);
  prefs.putString("wifiPass", homeWifiPassword);
  Serial.println("WiFi settings saved");
}

void saveOtaSettings() {
  prefs.putString("otaUrl", otaUrl);
  Serial.println("OTA settings saved");
}

void factoryResetSettings() {
  prefs.clear();
  Serial.println("Preferences cleared");
}
