#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 1

#define FIRMWARE_VERSION "0.2.0"

MatrixPanel_I2S_DMA *display = nullptr;

// Farben
uint16_t black;
uint16_t white;
uint16_t green;
uint16_t blue;

// Laufschrift
const char *scrollText = "ELEKTRONIKSERVICE  -  REPARATUR  -  KONSOLEN  -  SMARTFIX  ";
int16_t scrollX = PANEL_RES_X;
unsigned long lastScrollUpdate = 0;
const unsigned long scrollInterval = 35; // kleiner = schneller

int16_t getTextPixelWidth(const char *text) {
  // Standard Adafruit GFX Font: ca. 6 Pixel pro Zeichen
  return strlen(text) * 6;
}

void initMatrix() {
  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,
    PANEL_RES_Y,
    PANEL_CHAIN
  );

  // Waveshare ESP32-S3-RGB-Matrix Settings
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

  display->setBrightness8(70);
  display->clearScreen();

  black = display->color565(0, 0, 0);
  white = display->color565(255, 255, 255);
  green = display->color565(0, 255, 80);
  blue  = display->color565(0, 120, 255);
}

void drawHeader() {
  display->setTextWrap(false);
  display->setTextSize(1);

  // Smart
  display->setCursor(2, 3);
  display->setTextColor(green);
  display->print("Smart");

  // Fix
  display->setCursor(34, 3);
  display->setTextColor(blue);
  display->print("Fix");

  // kleine Trennlinie
  display->drawLine(0, 13, 63, 13, blue);
}

void drawScrollingText() {
  unsigned long now = millis();

  if (now - lastScrollUpdate >= scrollInterval) {
    lastScrollUpdate = now;

    display->fillScreen(black);

    drawHeader();

    display->setTextWrap(false);
    display->setTextSize(1);
    display->setTextColor(white);
    display->setCursor(scrollX, 20);
    display->print(scrollText);

    scrollX--;

    int16_t textWidth = getTextPixelWidth(scrollText);

    if (scrollX < -textWidth) {
      scrollX = PANEL_RES_X;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println("SMARTFIX MATRIX");
  Serial.print("Firmware v");
  Serial.println(FIRMWARE_VERSION);
  Serial.println("Mode: Scrolling Text");
  Serial.println("================================");

  initMatrix();

  Serial.println("Matrix initialized OK");
}

void loop() {
  drawScrollingText();
}