# SmartFix Matrix Modular Firmware

Diese Version ersetzt die große `src/main.cpp` durch mehrere Dateien.

## Enthaltene Funktionen

- HUB75 Matrix auf Waveshare ESP32-S3-RGB-Matrix
- AP `SmartFix-Matrix`
- optionales Heim-WLAN
- Webinterface
- Laufschrift Text, Geschwindigkeit, Farbe
- Logo Text
- Logo Effekte: Statisch, Buchstabe-für-Buchstabe, Fade In/Out
- gespeicherte Einstellungen per Preferences/NVS
- GitHub OTA URL + OTA Start
- 3D Logo Modus entfernt
- Custom SmartFix Pixel-Font für bessere Lesbarkeit

## Import

1. Den Inhalt des `src` Ordners in deinen lokalen `src` Ordner kopieren.
2. Alte `src/main.cpp` ersetzen.
3. Optional `platformio.ini` übernehmen, falls deine aktuelle abweicht.
4. Build:

```powershell
pio run
pio run -t upload
pio device monitor
```

## Panel-Erweiterung

Aktuell ist es für 1x 64x32 eingestellt.

- 2 Panels nebeneinander: meistens 128x32 (`PANEL_CHAIN = 2`)
- 2 Panels übereinander: 64x64, braucht meist Virtual-Matrix-Mapping
- 4 Panels 2x2: 128x64, braucht Virtual-Matrix-Mapping
