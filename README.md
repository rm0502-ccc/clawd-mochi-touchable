# Clawd Mochi Touchable 🦀✋

A modified version of [Clawd Mochi](https://github.com/yousifamanuel/clawd-mochi) by [@yousifamanuel](https://github.com/yousifamanuel), adding touch interaction, time-aware behavior, and 15 new animated expressions.

The original Clawd Mochi is a physical desk companion driven by an ESP32-C3 and a 1.54" TFT display. This fork keeps all the original hardware and wiring unchanged, but significantly expands the firmware to turn it from a remote-controlled display into an autonomous desk pet with its own personality.

> ⚠️ This is an independent fan project. It is not affiliated with, sponsored by, or endorsed by Anthropic. "Claude" and "Clawd" are trademarks of Anthropic.

---

## What's different from the original

### 15 new expressions (4 → 19 total)

| Expression | Description |
|---|---|
| 😠 Angry | Furrowed brows + shake animation |
| 😢 Sad | Drooping eyes + falling tear drops |
| 😪 Tired | Eyelids drooping shut |
| 😴 Sleep | Closed eyes + floating z Z Z |
| 🤔 Think | Upward gaze + thought bubbles appear and drift |
| 😊 Happy | Inverted-V eyes + bounce |
| 😒 Annoyed | One eye squinting + side glance |
| 😘 Kiss | Winking eye + heart that grows, pulses, and explodes into particles |
| 😉 Wink | Quick blink pattern |
| 🫧 Bubble | Normal eyes + floating bubbles |
| 😑 Bored | Half-closed eyes + slow side-to-side drift with tilt |
| 😕 Confused | Asymmetric eyes |
| 😵 Dizzy | Spiral eyes (triggered by touch overload) |
| 💀 Dead | X-shaped eyes |
| 👀 Lookaround | Eyes darting left and right |

Each expression has its own draw function (static frame) and anim function (full animation sequence).

### Touch interaction (TTP223 sensor)

Added a TTP223 capacitive touch sensor on **GPIO 5**.

- **Single touch** → random positive expression (Happy, Kiss, Squish, or Wink)
- **5+ touches within 25 seconds** → touch overload → Dizzy expression
- **20 seconds after last touch** → returns to automatic mode
- **Touch during animation** → interrupts current animation and responds immediately (via `animDelay()` interruptible delay system)

### Time-aware behavior (NTP)

Connects to your home WiFi to sync time via NTP, enabling schedule-based expressions:

| Time | Behavior |
|---|---|
| 23:00 – 08:00 | Sleep mode (continuous) |
| 12:00 – 13:00 | Tired mode (continuous) |
| Other hours | Random expression rotation with weighted probability |

### Auto-cycling system

The `loop()` is now a full state machine:

1. Handle web requests
2. Process touch input
3. Check manual override timeout (20s)
4. Apply time-based rules
5. Repeat current expression (10 times before switching)
6. Weighted random selection for next expression

Expression probabilities: Normal 19%, Happy 9%, Bored 8%, Squish 8%, Think 8%, Annoyed 8%, Angry 7%, Sad 7%, Wink 5%, Kiss 5%, Lookaround 3%, Sleep 3%, Tired 2%, Confused 2%, Dizzy 2%, Dead 2%.

### WiFi mode change

Changed from AP-only (hotspot) to **STA mode** (connects to your home WiFi) with AP fallback. When connected, the display shows the assigned IP address. If WiFi connection fails, it falls back to the original hotspot mode (`ClaWD-Mochi` / `clawd1234`).

---

## Additional hardware

Only one part added on top of the [original parts list](https://github.com/yousifamanuel/clawd-mochi#parts-list):

| Part | Spec | ~Price |
|---|---|---|
| TTP223 touch sensor | Capacitive touch module | ~$0.30 |

Connect: **SIG → GPIO 5**, **VCC → 3V3**, **GND → GND**.

Everything else (ESP32-C3, ST7789 display, wiring, 3D case) is identical to the original project.

---

## Setup

Follow the [original setup guide](https://github.com/yousifamanuel/clawd-mochi#software-setup) for Arduino IDE, board support, and libraries.

Before uploading, edit `clawd_mochi_touchable.ino` and replace the WiFi credentials with your own:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
```

> **Important:** Set **Tools → USB CDC On Boot → Enabled** before flashing, or the board won't be recognized.

---

## File structure

| File | Description |
|---|---|
| `clawd_mochi_touchable.ino` | Modified firmware with touch, expressions, and time logic |
| `clawd_mochi.ino` | Original firmware by [@yousifamanuel](https://github.com/yousifamanuel) (unchanged) |

---

## Credits

This project is a fork of [yousifamanuel/clawd-mochi](https://github.com/yousifamanuel/clawd-mochi). All hardware design, 3D models, wiring, web controller, and the original firmware are by [@yousifamanuel](https://github.com/yousifamanuel). This fork only modifies the Arduino firmware.

## License

Same as the original project — [MIT License](LICENSE) for code, **CC BY-NC-SA 4.0** for 3D models and media assets.
