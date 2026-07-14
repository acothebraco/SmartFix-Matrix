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

uint16_t nextUtf8Codepoint(const String &text, uint16_t &index);
int16_t getMatrixCodepointPixelWidth(uint16_t codepoint);
int16_t getMatrixTextPixelWidth(const String &text);
int16_t getMatrixCodepointPixelWidthStyled(uint16_t codepoint, uint8_t size, uint8_t style);
int16_t getMatrixTextPixelWidthStyled(const String &text, uint8_t size, uint8_t style);
void drawMatrixCodepoint(uint16_t codepoint, int16_t x, int16_t y, uint16_t color);
void drawMatrixText(const String &text, int16_t x, int16_t y, uint16_t color);
void drawMatrixCodepointStyled(uint16_t codepoint, int16_t x, int16_t y, uint16_t color, uint8_t size, uint8_t style);
void drawMatrixTextStyled(const String &text, int16_t x, int16_t y, uint16_t color, uint8_t size, uint8_t style);

void drawHeader();
void drawDiyLedMatrixWordmark(int16_t x, int16_t y, uint8_t revealChars, uint8_t brightnessScale);
