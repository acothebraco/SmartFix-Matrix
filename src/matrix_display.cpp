#include "matrix_display.h"
#include "config.h"
#include "app_state.h"

MatrixPanel_I2S_DMA *display = nullptr;

uint16_t black;
uint16_t white;
uint16_t green;
uint16_t blue;
uint16_t red;
uint16_t yellow;

uint16_t makeColor(uint8_t r, uint8_t g, uint8_t b) {
  // This panel has green/blue channel swap.
  // Desired RGB(r,g,b) -> library RGB(r,b,g)
  return display->color565(r, b, g);
}

void initMatrix() {
  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,
    PANEL_RES_Y,
    PANEL_CHAIN
  );

  // Waveshare ESP32-S3-RGB-Matrix settings
  mxconfig.gpio.e = 9;
  mxconfig.clkphase = false;
  mxconfig.driver = HUB75_I2S_CFG::SHIFTREG;

  display = new MatrixPanel_I2S_DMA(mxconfig);

  if (!display->begin()) {
    Serial.println("ERROR: display->begin() failed!");
    while (true) {
      delay(1000);
    }
  }

  display->setBrightness8(matrixBrightness);
  display->clearScreen();

  black  = makeColor(0, 0, 0);
  white  = makeColor(255, 255, 255);
  green  = makeColor(0, 255, 80);
  blue   = makeColor(0, 120, 255);
  red    = makeColor(255, 0, 0);
  yellow = makeColor(255, 180, 0);
}

void clearDisplay() {
  if (display) {
    display->clearScreen();
  }
}

uint16_t getScrollTextColor() {
  switch (scrollTextColorMode) {
    case 0: return white;
    case 1: return green;
    case 2: return blue;
    case 3: return yellow;
    case 4: return red;
    default: return white;
  }
}

static bool logoIsSmartFix() {
  return logoText.equalsIgnoreCase("SmartFix");
}

static uint8_t clampScale(uint16_t value) {
  if (value > 255) return 255;
  return (uint8_t)value;
}

static uint16_t scaledColor(uint8_t r, uint8_t g, uint8_t b, uint8_t scale) {
  return makeColor((uint16_t)r * scale / 255,
                   (uint16_t)g * scale / 255,
                   (uint16_t)b * scale / 255);
}

static uint16_t smartColor(uint8_t scale = 255) {
  return scaledColor(0, 255, 80, scale);
}

static uint16_t fixColor(uint8_t scale = 255) {
  return scaledColor(0, 120, 255, scale);
}

static uint16_t smartShadowColor(uint8_t scale = 255) {
  return scaledColor(0, 40, 18, scale);
}

static uint16_t fixShadowColor(uint8_t scale = 255) {
  return scaledColor(0, 22, 55, scale);
}

static uint16_t smartHighlightColor(uint8_t scale = 255) {
  return scaledColor(120, 255, 160, scale);
}

static uint16_t fixHighlightColor(uint8_t scale = 255) {
  return scaledColor(110, 210, 255, scale);
}

static void printText(const String &text, int16_t x, int16_t y, uint16_t color) {
  display->setTextWrap(false);
  display->setTextSize(1);
  display->setTextColor(color);
  display->setCursor(x, y);
  display->print(text);
}

// Draw a readable wordmark with the same base font as the scrolling text,
// but with a subtle shadow/highlight so it looks more like a logo and not like
// plain thin text. This keeps the m readable on 64x32.
static void drawBrandWordmark(int16_t x, int16_t y, uint8_t revealChars, uint8_t brightnessScale, int8_t shimmerIndex = -1) {
  const String smart = "Smart";
  const String fix = "Fix";
  const String full = "SmartFix";

  if (revealChars > full.length()) {
    revealChars = full.length();
  }

  uint8_t smartReveal = revealChars;
  if (smartReveal > smart.length()) smartReveal = smart.length();

  uint8_t fixReveal = 0;
  if (revealChars > smart.length()) {
    fixReveal = revealChars - smart.length();
    if (fixReveal > fix.length()) fixReveal = fix.length();
  }

  String smartVisible = smart.substring(0, smartReveal);
  String fixVisible = fix.substring(0, fixReveal);

  int16_t fixX = x + 32;  // 5 chars * 6 px + small 2 px brand gap

  // Soft logo-like depth. Only dim, so it does not destroy the m.
  if (smartVisible.length() > 0) {
    printText(smartVisible, x + 1, y + 1, smartShadowColor(brightnessScale));
  }
  if (fixVisible.length() > 0) {
    printText(fixVisible, fixX + 1, y + 1, fixShadowColor(brightnessScale));
  }

  // Main colored letters, exactly the same readable font as the scrolling text.
  if (smartVisible.length() > 0) {
    printText(smartVisible, x, y, smartColor(brightnessScale));
  }
  if (fixVisible.length() > 0) {
    printText(fixVisible, fixX, y, fixColor(brightnessScale));
  }

  // Small highlight stripe, inspired by the PNG gloss, but minimal for 64x32.
  if (revealChars >= full.length() && brightnessScale > 90) {
    display->drawPixel(x + 2, y, smartHighlightColor(brightnessScale));
    display->drawPixel(x + 3, y, smartHighlightColor(brightnessScale));
    display->drawPixel(fixX + 1, y, fixHighlightColor(brightnessScale));
    display->drawPixel(fixX + 2, y, fixHighlightColor(brightnessScale));
  }

  // Shimmer effect: one bright moving character.
  if (shimmerIndex >= 0 && shimmerIndex < (int)full.length() && revealChars >= full.length()) {
    if (shimmerIndex < 5) {
      String c = String(full[shimmerIndex]);
      printText(c, x + shimmerIndex * 6, y, smartHighlightColor(brightnessScale));
    } else {
      String c = String(full[shimmerIndex]);
      printText(c, fixX + (shimmerIndex - 5) * 6, y, fixHighlightColor(brightnessScale));
    }
  }
}

void drawSmartFixWordmark(int16_t x, int16_t y, uint8_t revealChars, uint8_t brightnessScale) {
  drawBrandWordmark(x, y, revealChars, brightnessScale);
}

static void drawGenericLogoText(const String &text, int16_t x, int16_t y, uint8_t revealChars, uint8_t brightnessScale) {
  String visible = text;
  if (revealChars < visible.length()) {
    visible = visible.substring(0, revealChars);
  }

  uint16_t shadow = scaledColor(0, 35, 18, brightnessScale);
  uint16_t main   = smartColor(brightnessScale);

  printText(visible, x + 1, y + 1, shadow);
  printText(visible, x, y, main);
}

static void drawHeaderSparkles(int16_t x, int16_t y) {
  // Deterministic small sparkle field around the header. No true randomness here,
  // so the animation stays smooth without flicker chaos.
  uint8_t frame = (millis() / 95) % 16;

  const int8_t pts[][2] = {
    {0, 0}, {8, 1}, {15, -1}, {25, 0}, {35, -1}, {46, 1}, {52, 0},
    {4, 9}, {12, 10}, {29, 9}, {39, 10}, {48, 9}, {57, 10}
  };

  for (uint8_t i = 0; i < sizeof(pts) / sizeof(pts[0]); i++) {
    if (((i + frame) % 5) == 0) {
      uint16_t c = (i % 2 == 0) ? smartHighlightColor(180) : fixHighlightColor(180);
      int16_t px = x + pts[i][0];
      int16_t py = y + pts[i][1];
      if (px >= 0 && px < PANEL_RES_X && py >= 0 && py < PANEL_RES_Y) {
        display->drawPixel(px, py, c);
      }
    }
  }
}

void drawHeader() {
  display->setTextWrap(false);
  display->setTextSize(1);

  String text = logoText;
  text.trim();
  if (text.length() == 0) {
    text = "SmartFix";
  }

  const bool isBrand = text.equalsIgnoreCase("SmartFix");
  const uint8_t totalChars = isBrand ? 8 : text.length();

  uint8_t revealChars = totalChars;
  uint8_t fadeScale = 255;
  int16_t baseX = isBrand ? 7 : (PANEL_RES_X - getTextPixelWidth(text)) / 2;
  int16_t baseY = 3;
  int8_t shimmerIndex = -1;

  if (baseX < 0) baseX = 0;

  if (logoEffectMode == LOGO_EFFECT_TYPEWRITER) {
    uint8_t phase = (millis() / 220) % (totalChars + 8);
    revealChars = phase;
    if (revealChars > totalChars) revealChars = totalChars;
  } else if (logoEffectMode == LOGO_EFFECT_FADE) {
    uint16_t phase = (millis() / 18) % 512;
    if (phase > 255) phase = 511 - phase;
    fadeScale = clampScale(50 + ((uint16_t)phase * 205 / 255));
  } else if (logoEffectMode == LOGO_EFFECT_SLIDE) {
    uint16_t phase = (millis() / 22) % 170;
    if (phase < 55) {
      int16_t targetX = baseX;
      baseX = PANEL_RES_X - ((PANEL_RES_X - targetX) * phase / 55);
    } else if (phase > 125) {
      int16_t targetX = baseX;
      baseX = targetX - ((phase - 125) * (targetX + 56) / 45);
    }
  } else if (logoEffectMode == LOGO_EFFECT_SHIMMER) {
    shimmerIndex = (millis() / 115) % (totalChars + 4);
    if (shimmerIndex >= totalChars) shimmerIndex = -1;
  } else if (logoEffectMode == LOGO_EFFECT_SPARKLE) {
    // Main draw stays static; sparkles are added below.
  } else if (logoEffectMode == LOGO_EFFECT_PULSE) {
    uint16_t phase = (millis() / 16) % 512;
    if (phase > 255) phase = 511 - phase;
    fadeScale = clampScale(150 + ((uint16_t)phase * 105 / 255));
  }

  if (isBrand) {
    drawBrandWordmark(baseX, baseY, revealChars, fadeScale, shimmerIndex);
  } else {
    drawGenericLogoText(text, baseX, baseY, revealChars, fadeScale);
  }

  if (logoEffectMode == LOGO_EFFECT_SPARKLE || logoEffectMode == LOGO_EFFECT_PULSE) {
    drawHeaderSparkles(baseX, baseY);
  }

  // Blue separator line intentionally removed.
}
