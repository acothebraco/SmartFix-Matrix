#pragma once

#include <Arduino.h>
#include <WiFi.h>

void setupWiFi();
void handleWiFiReconnect();
String getWiFiStatusText();
String getStaIpText();
String getMdnsAddressText();
bool isMdnsStarted();
void disconnectHomeWiFi();
