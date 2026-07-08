#include "animations.h"
#include "config.h"
#include "app_state.h"
#include "matrix_display.h"
#include "settings.h"

static int16_t scrollX = PANEL_RES_X;
static unsigned long lastScrollUpdate = 0;
static unsigned long lastFxUpdate = 0;
static unsigned long lastFullRedraw = 0;
static bool scrollDualReturn = false;

void resetAnimationState() {
  scrollX = PANEL_RES_X;
  lastScrollUpdate = 0;
  lastFxUpdate = 0;
  lastFullRedraw = 0;
  scrollDualReturn = false;
}

static uint16_t scaledMatrixColor(uint8_t r, uint8_t g, uint8_t b, uint8_t scale) {
  return makeColor((uint16_t)r * scale / 255,
                   (uint16_t)g * scale / 255,
                   (uint16_t)b * scale / 255);
}

static uint16_t scrollPaletteColor(uint8_t index, uint8_t scale = 255) {
  switch (index % 5) {
    case 0: return scaledMatrixColor(0, 255, 80, scale);
    case 1: return scaledMatrixColor(0, 120, 255, scale);
    case 2: return scaledMatrixColor(255, 220, 0, scale);
    case 3: return scaledMatrixColor(255, 40, 35, scale);
    default: return scaledMatrixColor(255, 255, 255, scale);
  }
}

static uint16_t selectedScrollColorScaled(uint8_t scale) {
  switch (scrollTextColorMode) {
    case 1: return scaledMatrixColor(0, 255, 80, scale);
    case 2: return scaledMatrixColor(0, 120, 255, scale);
    case 3: return scaledMatrixColor(255, 220, 0, scale);
    case 4: return scaledMatrixColor(255, 40, 35, scale);
    case 0:
    default: return scaledMatrixColor(255, 255, 255, scale);
  }
}

static int8_t scrollWaveOffset(uint16_t phase, int8_t amplitude) {
  phase %= 24;
  if (phase > 12) {
    phase = 24 - phase;
  }
  return ((int16_t)phase * amplitude * 2 / 12) - amplitude;
}

static void printScrollCodepoint(uint16_t codepoint, int16_t x, int16_t y, uint16_t color) {
  drawMatrixCodepoint(codepoint, x, y, color);
}

static void drawScrollSparkles(unsigned long now) {
  for (uint8_t i = 0; i < 5; i++) {
    int16_t px = (now / 55 + i * 17) % PANEL_RES_X;
    int16_t py = 15 + ((now / 90 + i * 7) % 15);
    uint16_t color = scrollPaletteColor(i + now / 160, 180);
    display->drawPixel(px, py, color);
  }
}

static void drawScrollingText() {
  unsigned long now = millis();

  if (now - lastScrollUpdate >= scrollInterval) {
    lastScrollUpdate = now;

    display->fillScreen(black);
    drawHeader();

    display->setTextWrap(false);
    display->setTextSize(1);

    if (scrollTextEffectMode == SCROLL_EFFECT_NORMAL || scrollTextEffectMode == SCROLL_EFFECT_DUAL_SLIDE) {
      drawMatrixText(scrollText, scrollX, 20, getScrollTextColor());
    } else {
      int16_t cursorX = scrollX;
      uint16_t byteIndex = 0;
      uint16_t glyphIndex = 0;

      while (byteIndex < scrollText.length()) {
        uint16_t cp = nextUtf8Codepoint(scrollText, byteIndex);
        int16_t glyphWidth = getMatrixCodepointPixelWidth(cp);

        if (cursorX > -12 && cursorX < PANEL_RES_X) {
          int16_t y = 20;
          uint16_t color = getScrollTextColor();

          if (scrollTextEffectMode == SCROLL_EFFECT_RAINBOW) {
            color = scrollPaletteColor(glyphIndex + now / 120);
          } else if (scrollTextEffectMode == SCROLL_EFFECT_WAVE) {
            y += scrollWaveOffset(now / 70 + glyphIndex * 3, 2);
          } else if (scrollTextEffectMode == SCROLL_EFFECT_SPARKLE) {
            color = (((glyphIndex + now / 90) % 9) == 0) ? white : getScrollTextColor();
          } else if (scrollTextEffectMode == SCROLL_EFFECT_COMET) {
            printScrollCodepoint(cp, cursorX + 3, y, selectedScrollColorScaled(45));
            printScrollCodepoint(cp, cursorX + 2, y, selectedScrollColorScaled(75));
            printScrollCodepoint(cp, cursorX + 1, y, selectedScrollColorScaled(110));
            color = selectedScrollColorScaled(255);
          } else if (scrollTextEffectMode == SCROLL_EFFECT_FLASH) {
            color = ((now / 350) % 2 == 0) ? white : getScrollTextColor();
          }

          printScrollCodepoint(cp, cursorX, y, color);
        }

        cursorX += glyphWidth;
        glyphIndex++;
      }

      if (scrollTextEffectMode == SCROLL_EFFECT_SPARKLE) {
        drawScrollSparkles(now);
      }
    }

    int16_t textWidth = getTextPixelWidth(scrollText);

    if (scrollTextEffectMode == SCROLL_EFFECT_DUAL_SLIDE) {
      if (scrollDualReturn) {
        scrollX++;
        if (scrollX > PANEL_RES_X) {
          scrollX = PANEL_RES_X;
          scrollDualReturn = false;
        }
      } else {
        scrollX--;
        if (scrollX < -textWidth) {
          scrollX = -textWidth;
          scrollDualReturn = true;
        }
      }
    } else {
      scrollX--;
      if (scrollX < -textWidth) {
        scrollX = PANEL_RES_X;
      }
    }
  }
}

static void drawLogoStatic() {
  unsigned long now = millis();

  if (now - lastFullRedraw < 500) {
    return;
  }

  lastFullRedraw = now;
  display->fillScreen(black);

  drawHeader();

  display->setTextSize(1);
  display->setTextColor(white);
  display->setCursor(12, 20);
  display->print("MATRIX");

  display->drawRect(0, 0, 64, 32, green);
  display->drawPixel(1, 1, blue);
  display->drawPixel(62, 1, blue);
  display->drawPixel(1, 30, blue);
  display->drawPixel(62, 30, blue);
}

static void drawPixelArt() {
  unsigned long now = millis();

  if (now - lastFullRedraw < 500) {
    return;
  }

  lastFullRedraw = now;
  display->fillScreen(black);

  display->fillRect(8, 8, 4, 4, red);
  display->fillRect(16, 8, 4, 4, red);
  display->fillRect(4, 12, 20, 4, red);
  display->fillRect(8, 16, 12, 4, red);
  display->fillRect(12, 20, 4, 4, red);

  display->setTextWrap(false);
  display->setTextSize(1);
  display->setTextColor(green);
  display->setCursor(31, 7);
  display->print("PIX");

  display->setTextColor(blue);
  display->setCursor(31, 18);
  display->print("ART");
}

static void drawRandomFx() {
  unsigned long now = millis();

  if (now - lastFullRedraw > 3000) {
    lastFullRedraw = now;
    display->fillScreen(black);

    display->setTextWrap(false);
    display->setTextSize(1);
    display->setTextColor(green);
    display->setCursor(3, 3);
    display->print("RANDOM");

    display->setTextColor(blue);
    display->setCursor(18, 14);
    display->print("FX");
  }

  if (now - lastFxUpdate >= 20) {
    lastFxUpdate = now;

    int x = random(0, PANEL_RES_X);
    int y = random(0, PANEL_RES_Y);

    uint16_t color;

    int r = random(0, 4);
    if (r == 0) color = green;
    else if (r == 1) color = blue;
    else if (r == 2) color = white;
    else color = yellow;

    display->drawPixel(x, y, color);
  }
}

void handleAutoModeDemo() {
  if (!autoModeDemo) {
    return;
  }

  unsigned long now = millis();

  if (now - lastModeChange >= modeInterval) {
    lastModeChange = now;

    int nextMode = (int)currentMode + 1;

    if (nextMode > MODE_RANDOM_FX) {
      nextMode = MODE_SCROLL_TEXT;
    }

    setMode((DisplayMode)nextMode);
  }
}

void handleSerialCommands() {
  if (!Serial.available()) {
    return;
  }

  char cmd = Serial.read();

  if (cmd == '0') {
    autoModeDemo = false;
    setMode(MODE_SCROLL_TEXT);
  } else if (cmd == '1') {
    autoModeDemo = false;
    setMode(MODE_LOGO_STATIC);
  } else if (cmd == '2') {
    autoModeDemo = false;
    setMode(MODE_PIXEL_ART);
  } else if (cmd == '3') {
    autoModeDemo = false;
    setMode(MODE_RANDOM_FX);
  } else if (cmd == 'a' || cmd == 'A') {
    autoModeDemo = true;
    lastModeChange = millis();
    saveModeSettings();
    Serial.println("Auto mode demo enabled");
  }
}

void handleCurrentAnimation() {
  handleSerialCommands();
  handleAutoModeDemo();

  switch (currentMode) {
    case MODE_SCROLL_TEXT:
      drawScrollingText();
      break;

    case MODE_LOGO_STATIC:
      drawLogoStatic();
      break;

    case MODE_PIXEL_ART:
      drawPixelArt();
      break;

    case MODE_RANDOM_FX:
      drawRandomFx();
      break;
  }
}
