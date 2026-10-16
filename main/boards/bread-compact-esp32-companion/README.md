# Bread Compact ESP32 Companion

ESP32 DevKit + I2S mic + I2S amp + SSD1306 OLED (128x64) + 3 hobby servos.
Same audio/OLED/button wiring as `bread-compact-esp32`; adds a full-screen animated face and a servo body.

Files:

| File | Purpose |
| --- | --- |
| `config.h` | Pins, OLED size/orientation, servo angles and limits (edit this to calibrate) |
| `face_renderer.*` | Procedural face engine (pure C++, no ESP-IDF; 21 emotions, blink, gaze, talking mouth, extras) |
| `face_display.*` | `Display` implementation that pushes the face to the SSD1306 (no LVGL) |
| `servo_controller.*` | 3 servos on LEDC PWM, smooth keyframe gestures, relax-when-idle |
| `companion_board.cc` | Board class, buttons, MCP tools (`self.companion.*`) |

See `/COMPANION_SETUP.md` in the project root for wiring and the VS Code flashing steps.
