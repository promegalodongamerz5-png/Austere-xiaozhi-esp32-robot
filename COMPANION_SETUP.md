# Xiaozhi Companion: animated OLED face + 3 servos

This fork adds a new board, **`bread-compact-esp32-companion`** (ESP32 v1 + SSD1306 OLED + 3 servos).
The original `bread-compact-esp32` board is untouched.

## What you get

- **Full-screen face on the OLED** with all 21 emotions the Xiaozhi server sends:
  neutral, happy, laughing, funny, sad, angry, crying, loving, embarrassed, surprised, shocked,
  thinking, winking, cool, relaxed, delicious, kissy, confident, sleepy, silly, confused.
  Transitions are animated; the eyes blink and wander, the mouth moves while it is speaking,
  and some emotions add effects (tears, hearts, blush, "zZz", sparkles, thought dots, "?").
- **3 servos**: head turns left/right, two hands raise/wave. Every emotion triggers a matching gesture and
  the body makes small movements while the assistant speaks.
- **Voice control of the body** through MCP tools the AI can call
  (`self.companion.look`, `.arms`, `.gesture`, `.emotion`), e.g. "wave at me", "look to your left".
- System messages (Wi-Fi setup hotspot name/URL, errors, activation code) still appear as a text screen.

## Wiring (defaults in `main/boards/bread-compact-esp32-companion/config.h`)

| Part | ESP32 GPIO |
| --- | --- |
| OLED SDA / SCL | 4 / 15 |
| Mic (I2S) WS / SCK / DIN | 25 / 26 / 32 |
| Speaker amp (I2S) DOUT / BCLK / LRCK | 33 / 14 / 27 |
| Boot button / touch button / ASR button | 0 / 5 / 19 |
| **Servo: head** | **23** |
| **Servo: left hand** | **13** |
| **Servo: right hand** | **12** |

Servo power: use a separate 5 V supply (about 1-2 A), join its GND to the ESP32 GND, and put a
470-1000 uF capacitor across the servo supply. Powering three servos from the ESP32 board usually causes
resets (brown-outs) and noisy audio. GPIO12 is a boot-strapping pin: do not let anything pull it high at
power-up (a servo signal wire does not). If boot ever fails, move that servo to another free pin in `config.h`.

## Flash from VS Code

1. Install **VS Code** and the **ESP-IDF** extension (`espressif.esp-idf-extension`; VS Code will offer it
   because of `.vscode/extensions.json`).
2. Run the extension's setup (`ESP-IDF: Configure ESP-IDF Extension`) and install **ESP-IDF v6.0.1 or newer
   (6.1 recommended)**. This firmware does not support ESP-IDF 5.x.
3. `File > Open Folder...` and open this project folder (the one that contains `CMakeLists.txt`).
   The included `.vscode/settings.json` already sets the target to `esp32` and selects the companion
   board through `sdkconfig.companion`.
4. In the bottom status bar: choose your **serial port** (plug the ESP32 in first) and make sure the target is `esp32`.
5. Click the **Build, Flash and Monitor** flame button (or run `ESP-IDF: Build, Flash and Monitor`
   from the command palette). The first build downloads components and takes a while.

If you ever change target or board and the build behaves strangely, run `ESP-IDF: Full Clean` and
build again.

Check that the board is selected: `ESP-IDF: SDK Configuration Editor`, search for *Target Board*; it should say
*Bread Compact ESP32 DevKit + Face + Servos (Companion)*.

Terminal alternative (inside an ESP-IDF terminal): `python3 scripts/build.py bread-compact-esp32-companion`
then `idf.py flash monitor`.

## Calibrating the servos (edit `config.h`, then rebuild)

- Head: `SERVO_HEAD_CENTER_DEG` (straight ahead), `SERVO_HEAD_RANGE_DEG` (how far it turns each side),
  `SERVO_HEAD_REVERSED` (swap left and right).
- Hands: `*_DOWN_DEG` and `*_UP_DEG` per hand. The hands are normally mirrored, so one pair counts up and the
  other down. Start with a small angle difference and widen it until the hand reaches the position you want
  without hitting the body.
- `SERVO_MIN_PULSE_US` / `SERVO_MAX_PULSE_US`: 500-2400 suits SG90/MG90S.
- `SERVO_RELAX_MS`: the servos stop being driven this long after a movement ends (silent, no buzzing). Set to 0 to hold forever.

## Display settings (`config.h`)

- `DISPLAY_HEIGHT`: `64` for 128x64 OLEDs, `32` for 128x32 (the face is scaled down automatically).
- Mirrored or upside-down face: toggle `DISPLAY_MIRROR_X` / `DISPLAY_MIRROR_Y`.
- White-on-black swapped: toggle `DISPLAY_INVERT_COLOR`.
- SH1106 modules: the same driver is used as the original board; if the image is shifted 2 pixels the module is
  an SH1106 and needs a column offset (not included).
- Only ASCII text is shown on the text screen (non-ASCII characters appear as `?`).

## Add or change emotions

All faces are defined in `MakePreset()` in `face_renderer.cc` (eye shape, size, lids, mouth, extras).
Gestures are keyframe tables at the top of `servo_controller.cc`; the emotion to gesture mapping is
`ServoController::PlayEmotion()`.

## Status of this change

- The face engine was compiled and rendered on a PC (every emotion, 128x64 and 128x32) and looks as intended.
- The repo's own test-suite passes (`python3 -m unittest discover -s scripts/tests`) and the new board is listed by
  `scripts/build.py --list-boards`.
- It has **not** been compiled with ESP-IDF or run on hardware (no ESP-IDF toolchain was available while writing it).
  Expect to fix small build errors or tweak servo angles on first use.

## Get a ready-to-flash .bin without installing ESP-IDF (GitHub Actions)

1. Create a new GitHub repository and push this project folder to it.
2. Open the repo's **Actions** tab, choose **Build Companion Firmware**, click **Run workflow**.
3. When it finishes (about 10-20 minutes), download the **companion-firmware** artifact and unzip it to get `merged-binary.bin`.
4. Flash it to the ESP32 DevKit v1 at address `0x0`:
   - Browser: open https://espressif.github.io/esp-launchpad/ , connect, add `merged-binary.bin` at `0x0`, Program.
   - Or: `pip install esptool` then `esptool.py --chip esp32 --port COM5 --baud 460800 write_flash 0x0 merged-binary.bin`
     (replace `COM5` with your port, e.g. `/dev/ttyUSB0` on Linux).
   If the board does not enter download mode, hold the BOOT button while it connects.
