# DIY LED Matrix - Header Font & Logo Effects Update

This update improves only the modular source structure.

Changed files:
- `src/config.h` - firmware version bumped to 1.2.0
- `src/app_state.h` - added logo effect enum values
- `src/app_state.cpp` - added logo effect names
- `src/settings.cpp` - stores/validates the new logo effects
- `src/web_interface.cpp` - adds new header/logo effect buttons
- `src/matrix_display.cpp` - replaces the old custom logo font with a readable wordmark based on the same GFX font style as the scrolling text, plus subtle shadow/highlight and additional effects

New Logo Effects:
- Statisch
- Buchstabe fuer Buchstabe
- Fade In / Out
- Slide In
- Shimmer
- Sparkle
- Pulse Glow

The 3D logo mode remains removed.
