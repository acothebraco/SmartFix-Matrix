#include "settings.h"
#include <Preferences.h>
#include "config.h"
#include "app_state.h"

static Preferences prefs;

void loadSettings() {
  prefs.begin(PREF_NAMESPACE, false);

  matrixBrightness = prefs.getUChar("bright", DEFAULT_BRIGHTNESS);

  scrollTextColorMode = prefs.getUChar("txtColor", 0);
  if (scrollTextColorMode > 4) {
    scrollTextColorMode = 0;
  }

  logoEffectMode = prefs.getUChar("logoFx", LOGO_EFFECT_STATIC);
  if (logoEffectMode > LOGO_EFFECT_FADE) {
    logoEffectMode = LOGO_EFFECT_STATIC;
  }

  scrollInterval = prefs.getUShort("speed", DEFAULT_SCROLL_INTERVAL);
  if (scrollInterval < 5 || scrollInterval > 200) {
    scrollInterval = DEFAULT_SCROLL_INTERVAL;
  }

  scrollText = prefs.getString("text", scrollText);
  if (scrollText.length() == 0) {
    scrollText = "ELEKTRONIKSERVICE  -  REPARATUR  -  KONSOLEN  -  SMARTFIX  ";
  }
  if (scrollText.length() > MAX_SCROLL_TEXT_LEN) {
    scrollText = scrollText.substring(0, MAX_SCROLL_TEXT_LEN);
  }

  logoText = prefs.getString("logoText", logoText);
  logoText.trim();
  if (logoText.length() == 0) {
    logoText = "SmartFix";
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
  Serial.print("Text Color: ");
  Serial.println(getScrollTextColorName());
  Serial.print("Logo Effect: ");
  Serial.println(getLogoEffectName());
  Serial.print("Scroll Speed: ");
  Serial.print(scrollInterval);
  Serial.println(" ms");
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

void saveBrightnessSetting() {
  prefs.putUChar("bright", matrixBrightness);
  Serial.print("Brightness saved: ");
  Serial.println(matrixBrightness);
}

void saveSpeedSetting() {
  prefs.putUShort("speed", scrollInterval);
  Serial.print("Scroll speed saved: ");
  Serial.print(scrollInterval);
  Serial.println(" ms");
}

void saveTextColorSetting() {
  prefs.putUChar("txtColor", scrollTextColorMode);
  Serial.print("Text color saved: ");
  Serial.println(getScrollTextColorName());
}

void saveLogoEffectSetting() {
  prefs.putUChar("logoFx", logoEffectMode);
  Serial.print("Logo effect saved: ");
  Serial.println(getLogoEffectName());
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
