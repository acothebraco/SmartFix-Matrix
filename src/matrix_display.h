#pragma once

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

extern MatrixPanel_I2S_DMA *display;

// Corrected colors for this panel
extern uint16_t black;
extern uint16_t white;
extern uint16_t green;
extern uint16_t blue;
extern uint16_t red;
extern uint16_t yellow;

void initMatrix();
void clearDisplay();

uint16_t makeColor(uint8_t r, uint8_t g, uint8_t b);
uint16_t getScrollTextColor();

void drawHeader();
void drawSmartFixWordmark(int16_t x, int16_t y, uint8_t revealChars, uint8_t brightnessScale);
