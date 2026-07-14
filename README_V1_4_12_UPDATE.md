# DIY LED Matrix v1.4.12 Update

This update restores the font controls from the recovered project ZIP and keeps the v1.4.11 brightness slider.

Changed files:

```text
src/config.h
src/app_state.h
src/app_state.cpp
src/settings.h
src/settings.cpp
src/matrix_display.h
src/matrix_display.cpp
src/animations.cpp
src/web_interface.cpp
release-notes/v1.4.12.md
```

Build:

```powershell
pio run
pio run -t upload
```

Commit:

```powershell
git add src release-notes/v1.4.12.md README_V1_4_12_UPDATE.md
git commit -m "Restore font controls and keep brightness slider"
git push
git tag v1.4.12
git push origin v1.4.12
```
