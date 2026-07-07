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

static uint16_t scaleSmartColor(uint8_t brightnessScale) {
  uint8_t g = (uint16_t)255 * brightnessScale / 255;
  uint8_t b = (uint16_t)80 * brightnessScale / 255;
  return makeColor(0, g, b);
}

static uint16_t scaleFixColor(uint8_t brightnessScale) {
  uint8_t g = (uint16_t)120 * brightnessScale / 255;
  uint8_t b = (uint16_t)255 * brightnessScale / 255;
  return makeColor(0, g, b);
}

static void drawPatternChar(int16_t x, int16_t y, const char *rows[], uint8_t h, uint16_t color) {
  for (uint8_t yy = 0; yy < h; yy++) {
    const char *row = rows[yy];
    for (uint8_t xx = 0; row[xx] != '\0'; xx++) {
      if (row[xx] != ' ') {
        display->drawPixel(x + xx, y + yy, color);
      }
    }
  }
}

// Custom compact 64x32-friendly logo font.
// Bigger and more readable than the default 5x7 font, but still fits 64px width.
static uint8_t drawLogoChar(char c, int16_t posX, int16_t posY, uint16_t color) {
  static const char *S[] = {
    " #### ",
    "##  ##",
    "##    ",
    " ###  ",
    "  ### ",
    "    ##",
    "##  ##",
    " #### ",
    "      "
  };

  static const char *m[] = {
    "        ",
    "## ##   ",
    "### ##  ",
    "## # ## ",
    "## # ## ",
    "## # ## ",
    "##   ## ",
    "##   ## ",
    "        "
  };

  static const char *a[] = {
    "      ",
    " ###  ",
    "   ## ",
    " #### ",
    "## ## ",
    "## ## ",
    "## ## ",
    " #### ",
    "      "
  };

  static const char *r[] = {
    "      ",
    "## ## ",
    "###   ",
    "##    ",
    "##    ",
    "##    ",
    "##    ",
    "##    ",
    "      "
  };

  static const char *t[] = {
    "  ##  ",
    " #### ",
    "  ##  ",
    "  ##  ",
    "  ##  ",
    "  ##  ",
    "  ### ",
    "   ## ",
    "      "
  };

  static const char *F[] = {
    "######",
    "##    ",
    "##    ",
    "##### ",
    "##    ",
    "##    ",
    "##    ",
    "##    ",
    "      "
  };

  static const char *i[] = {
    "##",
    "  ",
    "##",
    "##",
    "##",
    "##",
    "##",
    "##",
    "  "
  };

  static const char *xGlyph[] = {
    "      ",
    "##  ##",
    " #### ",
    "  ##  ",
    "  ##  ",
    " #### ",
    "##  ##",
    "##  ##",
    "      "
  };

  switch (c) {
    case 'S': drawPatternChar(posX, posY, S, 9, color); return 6;
    case 'm': drawPatternChar(posX, posY, m, 9, color); return 8;
    case 'a': drawPatternChar(posX, posY, a, 9, color); return 6;
    case 'r': drawPatternChar(posX, posY, r, 9, color); return 6;
    case 't': drawPatternChar(posX, posY, t, 9, color); return 6;
    case 'F': drawPatternChar(posX, posY, F, 9, color); return 6;
    case 'i': drawPatternChar(posX, posY, i, 9, color); return 2;
    case 'x': drawPatternChar(posX, posY, xGlyph, 9, color); return 6;
    default:
      display->setTextSize(1);
      display->setTextWrap(false);
      display->setTextColor(color);
      display->setCursor(posX, posY + 1);
      display->print(c);
      return 6;
  }
}

static uint8_t smartFixWordmarkWidth(uint8_t revealChars) {
  String text = "SmartFix";
  uint8_t w = 0;

  for (uint8_t idx = 0; idx < text.length() && idx < revealChars; idx++) {
    switch (text[idx]) {
      case 'S': w += 6; break;
      case 'm': w += 8; break;
      case 'a': w += 6; break;
      case 'r': w += 6; break;
      case 't': w += 6; break;
      case 'F': w += 6; break;
      case 'i': w += 2; break;
      case 'x': w += 6; break;
      default:  w += 6; break;
    }

    // Slightly bigger gap before "Fix"
    if (idx == 4) w += 2;
    else w += 1;
  }

  return w;
}

void drawSmartFixWordmark(int16_t x, int16_t y, uint8_t revealChars, uint8_t brightnessScale) {
  String text = "SmartFix";

  if (revealChars > text.length()) {
    revealChars = text.length();
  }

  int16_t cursorX = x;
  uint16_t smartColor = scaleSmartColor(brightnessScale);
  uint16_t fixColor = scaleFixColor(brightnessScale);

  for (uint8_t idx = 0; idx < text.length() && idx < revealChars; idx++) {
    uint16_t color = (idx >= 5) ? fixColor : smartColor;

    uint8_t charW = drawLogoChar(text[idx], cursorX, y, color);
    cursorX += charW;

    if (idx == 4) {
      cursorX += 2;
    } else {
      cursorX += 1;
    }
  }
}

void drawHeader() {
  display->setTextWrap(false);
  display->setTextSize(1);

  uint8_t revealChars = logoText.length();
  uint8_t fadeScale = 255;

  if (logoEffectMode == LOGO_EFFECT_TYPEWRITER) {
    uint8_t phase = (millis() / 230) % (logoText.length() + 7);
    revealChars = phase;
    if (revealChars > logoText.length()) {
      revealChars = logoText.length();
    }
  } else if (logoEffectMode == LOGO_EFFECT_FADE) {
    uint16_t phase = (millis() / 20) % 512;
    if (phase > 255) {
      phase = 511 - phase;
    }
    fadeScale = 45 + ((uint16_t)phase * 210 / 255);
  }

  if (logoIsSmartFix()) {
    uint8_t w = smartFixWordmarkWidth(revealChars);
    int16_t x = (PANEL_RES_X - w) / 2;
    if (x < 0) x = 0;

    drawSmartFixWordmark(x, 2, revealChars, fadeScale);
  } else {
    String visible = logoText;
    if (revealChars < visible.length()) {
      visible = visible.substring(0, revealChars);
    }

    int16_t logoWidth = getTextPixelWidth(visible);
    int16_t logoX = (PANEL_RES_X - logoWidth) / 2;
    if (logoX < 0) logoX = 0;

    uint16_t color = makeColor(0, (uint16_t)255 * fadeScale / 255, (uint16_t)80 * fadeScale / 255);

    display->setTextColor(color);
    display->setCursor(logoX, 3);
    display->print(visible);
  }

  // Blue separator line intentionally removed.
}
