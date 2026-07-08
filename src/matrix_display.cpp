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

uint16_t nextUtf8Codepoint(const String &text, uint16_t &index) {
  if (index >= text.length()) {
    return 0;
  }

  uint8_t c = (uint8_t)text[index++];

  if ((c & 0x80) == 0) {
    return c;
  }

  if ((c & 0xE0) == 0xC0 && index < text.length()) {
    uint8_t c2 = (uint8_t)text[index++];
    return ((uint16_t)(c & 0x1F) << 6) | (c2 & 0x3F);
  }

  if ((c & 0xF0) == 0xE0 && (index + 1) < text.length()) {
    uint8_t c2 = (uint8_t)text[index++];
    uint8_t c3 = (uint8_t)text[index++];
    return ((uint16_t)(c & 0x0F) << 12) | ((uint16_t)(c2 & 0x3F) << 6) | (c3 & 0x3F);
  }

  return '?';
}

static bool isUmlautCodepoint(uint16_t cp) {
  return cp == 0x00E4 || cp == 0x00F6 || cp == 0x00FC ||
         cp == 0x00C4 || cp == 0x00D6 || cp == 0x00DC;
}

static char umlautBaseChar(uint16_t cp) {
  switch (cp) {
    case 0x00E4: return 'a';
    case 0x00F6: return 'o';
    case 0x00FC: return 'u';
    case 0x00C4: return 'A';
    case 0x00D6: return 'O';
    case 0x00DC: return 'U';
    default: return '?';
  }
}

int16_t getMatrixCodepointPixelWidth(uint16_t codepoint) {
  if (codepoint == 0x00DF) { // ß -> ss
    return 12;
  }
  return 6;
}

int16_t getMatrixTextPixelWidth(const String &text) {
  int16_t width = 0;
  uint16_t i = 0;

  while (i < text.length()) {
    uint16_t cp = nextUtf8Codepoint(text, i);
    if (cp == 0) break;
    width += getMatrixCodepointPixelWidth(cp);
  }

  return width;
}

void drawMatrixCodepoint(uint16_t codepoint, int16_t x, int16_t y, uint16_t color) {
  display->setTextWrap(false);
  display->setTextSize(1);
  display->setTextColor(color);

  if (isUmlautCodepoint(codepoint)) {
    char base = umlautBaseChar(codepoint);
    display->setCursor(x, y);
    display->print(base);

    int16_t dotY = y - 2;
    if (dotY < 0) dotY = y;

    display->drawPixel(x + 1, dotY, color);
    display->drawPixel(x + 3, dotY, color);
    return;
  }

  if (codepoint == 0x00DF) { // ß
    display->setCursor(x, y);
    display->print("ss");
    return;
  }

  if (codepoint >= 32 && codepoint <= 126) {
    display->setCursor(x, y);
    display->print((char)codepoint);
    return;
  }

  display->setCursor(x, y);
  display->print('?');
}

void drawMatrixText(const String &text, int16_t x, int16_t y, uint16_t color) {
  int16_t cursorX = x;
  uint16_t i = 0;

  while (i < text.length()) {
    uint16_t cp = nextUtf8Codepoint(text, i);
    if (cp == 0) break;
    drawMatrixCodepoint(cp, cursorX, y, color);
    cursorX += getMatrixCodepointPixelWidth(cp);
  }
}

static uint8_t countUtf8Codepoints(const String &text) {
  uint8_t count = 0;
  uint16_t i = 0;

  while (i < text.length()) {
    uint16_t cp = nextUtf8Codepoint(text, i);
    if (cp == 0) break;
    count++;
  }

  return count;
}

static String utf8PrefixByCodepoints(const String &text, uint8_t maxCodepoints) {
  String output;
  uint16_t i = 0;
  uint8_t count = 0;

  while (i < text.length() && count < maxCodepoints) {
    uint16_t start = i;
    uint16_t cp = nextUtf8Codepoint(text, i);
    if (cp == 0) break;

    for (uint16_t j = start; j < i; j++) {
      output += text[j];
    }
    count++;
  }

  return output;
}

static bool logoIsSmartFix() {
  return logoText.equalsIgnoreCase("SmartFix");
}

static uint8_t clampScale(uint16_t value) {
  if (value > 255) return 255;
  return (uint8_t)value;
}

static uint16_t logoSpeedStep(uint16_t multiplier = 1) {
  uint16_t step = scrollInterval * multiplier;
  if (step < 8) step = 8;
  if (step > 240) step = 240;
  return step;
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

static uint16_t singleLogoColor(uint8_t colorIndex, uint8_t scale) {
  switch (colorIndex) {
    case 0: return smartColor(scale);
    case 1: return fixColor(scale);
    case 2: return scaledColor(255, 220, 0, scale);
    case 3: return scaledColor(255, 40, 35, scale);
    case 4: return scaledColor(255, 255, 255, scale);
    default: return smartColor(scale);
  }
}

static uint16_t singleLogoShadowColor(uint8_t colorIndex, uint8_t scale) {
  switch (colorIndex) {
    case 0: return scaledColor(0, 40, 18, scale);
    case 1: return scaledColor(0, 22, 55, scale);
    case 2: return scaledColor(55, 38, 0, scale);
    case 3: return scaledColor(55, 0, 0, scale);
    case 4: return scaledColor(35, 35, 35, scale);
    default: return scaledColor(0, 40, 18, scale);
  }
}

static uint16_t singleLogoHighlightColor(uint8_t colorIndex, uint8_t scale) {
  switch (colorIndex) {
    case 0: return smartHighlightColor(scale);
    case 1: return fixHighlightColor(scale);
    case 2: return scaledColor(255, 245, 140, scale);
    case 3: return scaledColor(255, 110, 110, scale);
    case 4: return scaledColor(255, 255, 255, scale);
    default: return smartHighlightColor(scale);
  }
}

static uint8_t colorIndexForLogoPart(uint8_t partIndex) {
  switch (logoColorMode) {
    case LOGO_COLOR_GREEN:  return 0;
    case LOGO_COLOR_BLUE:   return 1;
    case LOGO_COLOR_YELLOW: return 2;
    case LOGO_COLOR_RED:    return 3;
    case LOGO_COLOR_WHITE:  return 4;

    case LOGO_COLOR_RAINBOW:
      return partIndex % 5;

    case LOGO_COLOR_TWO_WORDS:
    case LOGO_COLOR_BRAND:
    default:
      return partIndex % 2;
  }
}

static uint16_t logoMainColor(uint8_t partIndex, uint8_t scale = 255) {
  return singleLogoColor(colorIndexForLogoPart(partIndex), scale);
}

static uint16_t logoShadowColor(uint8_t partIndex, uint8_t scale = 255) {
  return singleLogoShadowColor(colorIndexForLogoPart(partIndex), scale);
}

static uint16_t logoHighlightColor(uint8_t partIndex, uint8_t scale = 255) {
  return singleLogoHighlightColor(colorIndexForLogoPart(partIndex), scale);
}

static void printText(const String &text, int16_t x, int16_t y, uint16_t color) {
  drawMatrixText(text, x, y, color);
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
    printText(smartVisible, x + 1, y + 1, logoShadowColor(0, brightnessScale));
  }
  if (fixVisible.length() > 0) {
    printText(fixVisible, fixX + 1, y + 1, logoShadowColor(1, brightnessScale));
  }

  // Main colored letters, exactly the same readable font as the scrolling text.
  if (smartVisible.length() > 0) {
    printText(smartVisible, x, y, logoMainColor(0, brightnessScale));
  }
  if (fixVisible.length() > 0) {
    printText(fixVisible, fixX, y, logoMainColor(1, brightnessScale));
  }

  // Small highlight stripe, inspired by the PNG gloss, but minimal for 64x32.
  if (revealChars >= full.length() && brightnessScale > 90) {
    display->drawPixel(x + 2, y, logoHighlightColor(0, brightnessScale));
    display->drawPixel(x + 3, y, logoHighlightColor(0, brightnessScale));
    display->drawPixel(fixX + 1, y, logoHighlightColor(1, brightnessScale));
    display->drawPixel(fixX + 2, y, logoHighlightColor(1, brightnessScale));
  }

  // Shimmer effect: one bright moving character.
  if (shimmerIndex >= 0 && shimmerIndex < (int)full.length() && revealChars >= full.length()) {
    if (shimmerIndex < 5) {
      String c = String(full[shimmerIndex]);
      printText(c, x + shimmerIndex * 6, y, logoHighlightColor(0, brightnessScale));
    } else {
      String c = String(full[shimmerIndex]);
      printText(c, fixX + (shimmerIndex - 5) * 6, y, logoHighlightColor(1, brightnessScale));
    }
  }
}

void drawSmartFixWordmark(int16_t x, int16_t y, uint8_t revealChars, uint8_t brightnessScale) {
  drawBrandWordmark(x, y, revealChars, brightnessScale);
}

static void drawGenericLogoText(const String &text, int16_t x, int16_t y, uint8_t revealChars, uint8_t brightnessScale, int8_t shimmerIndex = -1) {
  String visible = text;
  if (revealChars < countUtf8Codepoints(visible)) {
    visible = utf8PrefixByCodepoints(visible, revealChars);
  }

  int16_t cursorX = x;
  uint8_t wordIndex = 0;
  String word = "";

  for (uint16_t i = 0; i <= visible.length(); i++) {
    char c = (i < visible.length()) ? visible[i] : ' ';
    bool separator = (c == ' ' || c == '-' || c == '_' || i == visible.length());

    if (!separator) {
      word += c;
      continue;
    }

    if (word.length() > 0) {
      printText(word, cursorX + 1, y + 1, logoShadowColor(wordIndex, brightnessScale));
      printText(word, cursorX, y, logoMainColor(wordIndex, brightnessScale));
      cursorX += getMatrixTextPixelWidth(word);
      wordIndex++;
      word = "";
    }

    if (i < visible.length()) {
      String sep = String(c);
      printText(sep, cursorX, y, logoMainColor(wordIndex, brightnessScale));
      cursorX += getMatrixTextPixelWidth(sep);
    }
  }

  if (shimmerIndex >= 0 && shimmerIndex < (int)visible.length()) {
    char c = visible[shimmerIndex];
    if (c != ' ') {
      printText(String(c), x + shimmerIndex * 6, y, scaledColor(255, 255, 255, brightnessScale));
    }
  }
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
      uint16_t c = logoHighlightColor(i, 180);
      int16_t px = x + pts[i][0];
      int16_t py = y + pts[i][1];
      if (px >= 0 && px < PANEL_RES_X && py >= 0 && py < PANEL_RES_Y) {
        display->drawPixel(px, py, c);
      }
    }
  }
}

static int8_t triangleWaveOffset(uint16_t phase, int8_t amplitude) {
  phase %= 24;
  if (phase > 12) {
    phase = 24 - phase;
  }
  return ((int16_t)phase * amplitude * 2 / 12) - amplitude;
}

static void drawLogoWaveText(const String &text, int16_t x, int16_t y, bool isBrand, uint8_t brightnessScale, bool bounceOnly) {
  int16_t cursorX = x;
  uint8_t wordIndex = 0;
  uint16_t t = millis() / logoSpeedStep(2);
  uint16_t byteIndex = 0;
  uint8_t glyphIndex = 0;
  int8_t globalBounce = bounceOnly ? triangleWaveOffset(millis() / logoSpeedStep(2), 2) : 0;

  while (byteIndex < text.length()) {
    uint16_t cp = nextUtf8Codepoint(text, byteIndex);

    if (cp == ' ') {
      cursorX += 6;
      wordIndex++;
      glyphIndex++;
      continue;
    }

    uint8_t partIndex = isBrand ? (glyphIndex < 5 ? 0 : 1) : wordIndex;
    int8_t yOffset = globalBounce;

    if (!bounceOnly) {
      yOffset = triangleWaveOffset(t + glyphIndex * 3, 2);
    }

    drawMatrixCodepoint(cp, cursorX + 1, y + 1 + yOffset, logoShadowColor(partIndex, brightnessScale));
    drawMatrixCodepoint(cp, cursorX, y + yOffset, logoMainColor(partIndex, brightnessScale));

    cursorX += getMatrixCodepointPixelWidth(cp);

    if (isBrand && glyphIndex == 4) {
      cursorX += 2;
    }

    if (!isBrand && (cp == '-' || cp == '_')) {
      wordIndex++;
    }

    glyphIndex++;
  }
}

static void drawLogoGlitchOverlay(const String &text, int16_t x, int16_t y, bool isBrand) {
  uint8_t phase = (millis() / logoSpeedStep(2)) % 24;

  if (phase > 5) {
    return;
  }

  // Short red/blue offsets. It looks like a glitch but stays readable.
  uint16_t glitchRed = scaledColor(255, 35, 35, 120);
  uint16_t glitchBlue = scaledColor(50, 150, 255, 120);

  if (isBrand) {
    printText("SmartFix", x + 1, y, glitchRed);
    printText("SmartFix", x - 1, y + 1, glitchBlue);
  } else {
    printText(text, x + 1, y, glitchRed);
    printText(text, x - 1, y + 1, glitchBlue);
  }

  int16_t lineY = y + 2 + (phase % 6);
  display->drawFastHLine(0, lineY, PANEL_RES_X, glitchBlue);
}

static void drawLogoScanline(int16_t x, int16_t y, int16_t textWidth) {
  uint16_t phase = (millis() / logoSpeedStep()) % (textWidth + 18);
  int16_t scanX = x - 8 + phase;

  for (int8_t dx = 0; dx < 2; dx++) {
    int16_t px = scanX + dx;
    if (px >= 0 && px < PANEL_RES_X) {
      display->drawFastVLine(px, y - 1, 10, scaledColor(255, 255, 255, 170));
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
  const uint8_t totalChars = isBrand ? 8 : countUtf8Codepoints(text);

  uint8_t revealChars = totalChars;
  uint8_t fadeScale = 255;
  int16_t baseX = isBrand ? 7 : (PANEL_RES_X - getMatrixTextPixelWidth(text)) / 2;
  int16_t baseY = 3;
  int8_t shimmerIndex = -1;

  if (baseX < 0) baseX = 0;

  if (logoEffectMode == LOGO_EFFECT_TYPEWRITER) {
    uint8_t phase = (millis() / logoSpeedStep(5)) % (totalChars + 8);
    revealChars = phase;
    if (revealChars > totalChars) revealChars = totalChars;
  } else if (logoEffectMode == LOGO_EFFECT_FADE) {
    uint16_t phase = (millis() / logoSpeedStep()) % 512;
    if (phase > 255) phase = 511 - phase;
    fadeScale = clampScale(50 + ((uint16_t)phase * 205 / 255));
  } else if (logoEffectMode == LOGO_EFFECT_SLIDE) {
    uint16_t phase = (millis() / logoSpeedStep()) % 170;
    if (phase < 55) {
      int16_t targetX = baseX;
      baseX = PANEL_RES_X - ((PANEL_RES_X - targetX) * phase / 55);
    } else if (phase > 125) {
      int16_t targetX = baseX;
      baseX = targetX - ((phase - 125) * (targetX + 56) / 45);
    }
  } else if (logoEffectMode == LOGO_EFFECT_DUAL_SLIDE) {
    uint16_t phase = (millis() / logoSpeedStep()) % 220;
    int16_t targetX = baseX;
    int16_t textWidth = isBrand ? 50 : getMatrixTextPixelWidth(text);

    if (phase < 55) {
      // Slide in from the right.
      baseX = PANEL_RES_X - ((PANEL_RES_X - targetX) * phase / 55);
    } else if (phase < 110) {
      baseX = targetX;
    } else if (phase < 165) {
      // Slide out to the right.
      baseX = targetX + ((phase - 110) * (PANEL_RES_X - targetX + 2) / 55);
    } else {
      // Slide in from the left.
      baseX = -textWidth + (((int32_t)(targetX + textWidth) * (phase - 165)) / 55);
    }
  } else if (logoEffectMode == LOGO_EFFECT_SHIMMER) {
    shimmerIndex = (millis() / logoSpeedStep(3)) % (totalChars + 4);
    if (shimmerIndex >= totalChars) shimmerIndex = -1;
  } else if (logoEffectMode == LOGO_EFFECT_SPARKLE) {
    // Main draw stays static; sparkles are added below.
  } else if (logoEffectMode == LOGO_EFFECT_PULSE) {
    uint16_t phase = (millis() / logoSpeedStep()) % 512;
    if (phase > 255) phase = 511 - phase;
    fadeScale = clampScale(150 + ((uint16_t)phase * 105 / 255));
  }

  if (logoEffectMode == LOGO_EFFECT_WAVE) {
    drawLogoWaveText(isBrand ? String("SmartFix") : text, baseX, baseY, isBrand, fadeScale, false);
  } else if (logoEffectMode == LOGO_EFFECT_BOUNCE) {
    drawLogoWaveText(isBrand ? String("SmartFix") : text, baseX, baseY, isBrand, fadeScale, true);
  } else {
    if (isBrand) {
      drawBrandWordmark(baseX, baseY, revealChars, fadeScale, shimmerIndex);
    } else {
      drawGenericLogoText(text, baseX, baseY, revealChars, fadeScale, shimmerIndex);
    }
  }

  if (logoEffectMode == LOGO_EFFECT_SPARKLE || logoEffectMode == LOGO_EFFECT_PULSE) {
    drawHeaderSparkles(baseX, baseY);
  } else if (logoEffectMode == LOGO_EFFECT_GLITCH) {
    drawLogoGlitchOverlay(isBrand ? String("SmartFix") : text, baseX, baseY, isBrand);
  } else if (logoEffectMode == LOGO_EFFECT_SCANLINE) {
    drawLogoScanline(baseX, baseY, isBrand ? 50 : getMatrixTextPixelWidth(text));
  }

  // Blue separator line intentionally removed.
}
