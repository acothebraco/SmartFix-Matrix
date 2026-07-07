#include "animations.h"
#include "config.h"
#include "app_state.h"
#include "matrix_display.h"
#include "settings.h"

static int16_t scrollX = PANEL_RES_X;
static unsigned long lastScrollUpdate = 0;
static unsigned long lastFxUpdate = 0;
static unsigned long lastFullRedraw = 0;

void resetAnimationState() {
  scrollX = PANEL_RES_X;
  lastScrollUpdate = 0;
  lastFxUpdate = 0;
  lastFullRedraw = 0;
}

static void drawScrollingText() {
  unsigned long now = millis();

  if (now - lastScrollUpdate >= scrollInterval) {
    lastScrollUpdate = now;

    display->fillScreen(black);
    drawHeader();

    display->setTextWrap(false);
    display->setTextSize(1);
    display->setTextColor(getScrollTextColor());
    display->setCursor(scrollX, 20);
    display->print(scrollText);

    scrollX--;

    int16_t textWidth = getTextPixelWidth(scrollText);

    if (scrollX < -textWidth) {
      scrollX = PANEL_RES_X;
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
