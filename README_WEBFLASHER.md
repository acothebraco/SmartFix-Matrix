# DIY LED Matrix Web Flasher

This update adds a GitHub Pages based browser flasher for DIY LED Matrix.

## Files

```text
web/flash/index.html
web/flash/manifest.json
.github/workflows/webflasher.yml
```

## One-time GitHub setup

1. Open the DIY-LED-Matrix repository on GitHub.
2. Go to Settings -> Pages.
3. Set Source to GitHub Actions.
4. Push this update to main.
5. Wait until the workflow `Build firmware and deploy web flasher` has completed.

The web flasher will be available at:

```text
https://acothebraco.github.io/DIY-LED-Matrix/
```

## Important

The browser flasher uses the full USB binary:

```text
DIY-LED-Matrix-usb.bin
```

The OTA binary is still used only for OTA updates from the device web interface:

```text
DIY-LED-Matrix-ota.bin
```


## Firmware version display

The GitHub Pages workflow reads the firmware version directly from:

```cpp
#define FIRMWARE_VERSION "x.y.z"
```

in `src/config.h` and replaces the placeholders in `web/flash/index.html` and `web/flash/manifest.json` during deployment.

So when you update the firmware version in `src/config.h`, the Web Flasher page will show the same version after the next GitHub Actions run.
