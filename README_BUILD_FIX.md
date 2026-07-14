# DIY LED Matrix v1.3.1 Build Fix

This package fixes GitHub Actions build errors caused by `web_interface.cpp` referencing the new text/logo effect declarations while the matching state/settings/display files were not committed.

Copy the files into the repository root and commit them.

Changed files:

```text
src/animations.cpp
src/animations.h
src/app_state.cpp
src/app_state.h
src/config.h
src/matrix_display.cpp
src/matrix_display.h
src/settings.cpp
src/settings.h
src/web_interface.cpp
src/web_interface.h
release-notes/v1.3.1.md
```

After copying:

```powershell
pio run
git add src release-notes/v1.3.1.md
git commit -m "Fix v1.3.1 text effect build"
git push
```
