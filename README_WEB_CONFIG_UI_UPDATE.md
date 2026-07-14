# DIY LED Matrix Web Config UI Update

This update fixes the dropdown behavior on the web configuration page and gives the page a new DIY LED Matrix web flasher inspired design.

## Changed files

```text
src/web_interface.cpp
src/config.h
release-notes/v1.3.1.md
```

## New behavior

- Open dropdown sections stay open after pressing buttons.
- Scroll position is restored after saving/changing settings.
- Returning from WiFi scan opens the Home WiFi section automatically.
- The web config page now uses a modern dark gradient design with DIY LED Matrix DIY badge.

## Build

```powershell
pio run
pio run -t upload
```

## Commit

```powershell
git add src/web_interface.cpp src/config.h release-notes/v1.3.1.md
git commit -m "Improve web config UI and keep dropdowns open"
git push
```
