# LifeBar

A standalone device that turns how you spend your time into an
identity, and puts it on your desk.

The idea came from resource bars in strategy games: a narrow display
that is always visible and shows where you stand at a glance. There are
hundreds of habit trackers in software — the difference here is the
object that sits there and cannot be swiped away.

All data stays on the device. No account, no cloud.

> [Deutsche Fassung](README.de.md) · Source comments are in German; the
> device UI and web interface are available in English and German.

---

## Features

- **Up to 6 skills** from 18 presets, logged in half-hour steps
- **Up to 6 vices** from 12 categories, counted as occurrences
- **EXP equal minutes.** Self-calibrating: nobody can log more than a
  day holds
- **Level curve without a cap** — `20 + 2.6·n²` minutes per level
- **Earned titles** instead of chosen classes
- **Weekly rhythm** starting Monday 4 a.m. with review, weekly goal,
  seals and bonus EXP for overtime
- **Lock screen** showing time, level and title
- **Night mode** with warmer colours, four brightness steps
- **English and German**, 12- and 24-hour format
- **Web interface** for stats, logging, setup, import and export
- **Event log** with timestamps, available as CSV

## The title system

Level unlocks the parts, the week fills them.

| Level | Structure | Example |
|---|---|---|
| 1–2 | — | NOVICE |
| 3–9 | noun | WARRIOR |
| 10–19 | + qualifier | WILD BERSERKER |
| 20+ | + lead word | NIGHTLY WILD WARLORD |

Noun and qualifier come from **total minutes** — they describe you, not
your week. The noun escalates with each tier: warrior → berserker →
warlord, scholar → magister → archscholar. The qualifier comes from the
second strongest skill, provided it holds at least 25 % of the leader's
minutes.

The lead word is reassigned **at every week rollover**, by salience:

1. **MINDFUL** at ten or more vice entries in the week
2. **DEVOTED** or **VERSATILE**, measured relative to the number of
   active modules
3. **NIGHTLY / MORNING / MIDDAY / EVENING** from the dominant time of
   day

At tier three that allows over 2000 distinct titles. The last 26 weeks
are kept as a chronicle and shown in the dashboard.

## Design principles

- **Time is the only currency.** Spreading yourself thin shows up
  immediately.
- **Vices cost no EXP.** Honesty has to stay cheap, otherwise people
  stop logging it — and then the data is worthless. What gets rewarded
  is consistent logging itself.
- **Level never drops.** Deselected modules keep their values and still
  count toward the level, they just disappear from the display.
- **No caps on backfilling.** Logging everything in the evening is the
  normal case, not the exception.
- **Select first, then log.** At 12 mm column width a mis-tap costs one
  tap instead of a wrong entry.
- **No flash writes from the UI task.** Every write runs through flags
  out of `loop()`.

---

## Hardware

**Waveshare ESP32-S3-Touch-LCD-3.49**, revision V2

- ESP32-S3R8, 16 MB flash, 8 MB PSRAM
- 3.49" IPS, 172 × 640, capacitive touch (AXS15231B, QSPI + I2C)
- PCF85063 RTC, QMI8658 IMU, ES8311 audio, TF slot
- Powered over USB-C; the 18650 holder on the board is optional and the
  firmware does not read it

### Mind the board revision

This board exists as V1 and V2. On V2, **LCD_BL and EXIO_INT as well as
LCD_TE and LCD_RESET sit on swapped IOs**. With the wrong example code
the screen stays black. Identifiable by the Rev1.1 silkscreen.

Because of that the **backlight PWM is disabled here**:
`EXAMPLE_PIN_NUM_BK_LIGHT` from the demo points at the V1 pin, so
`setUpduty()` has no effect. Brightness is handled through colours
instead.

---

## Installation

### 1. Get the Waveshare demo

Some files are not in this repo (see *Third-party files*). Start by
downloading the demo package:

```
github.com/waveshareteam/ESP32-S3-Touch-LCD-3.49
```

Extract it to a path **without spaces or non-ASCII characters**.

### 2. Set up Arduino

- Board package **esp32 by Espressif Systems ≥ 3.1.0**
- **LVGL 9 offline** from `Arduino_Libraries` of the demo package,
  copied into your sketchbook `libraries/` folder. If an `lvgl` folder
  is already there, delete it. The bundled `lv_conf.h` is customised
  and must not be overwritten by the Library Manager version
- The folder is named `lvgl9` in the package — rename it to `lvgl` and
  remove `lvgl8` from `libraries/`
- **SensorLib 0.3.1**
- Quit Arduino completely and restart it afterwards

Additionally enable in `lv_conf.h`:

```c
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_48 1
```

### 3. Add the third-party files

Copy from `10_LVGL_V9_Test` of the demo package into the sketch folder:

```
lvgl_port.c   lvgl_port.h   i2c_bsp.c   i2c_bsp.h   user_config.h
src/axs15231b/   src/touch/   src/lcd_bl_bsp/
```

Then apply the changes from `docs/waveshare-patches.md`. Without them
the firmware does not boot or the touch does not respond.

### 4. Tools settings

| Setting | Value |
|---|---|
| Board | ESP32S3 Dev Module |
| USB CDC On Boot | **Enabled** |
| Flash Size | 16MB (128Mb) |
| PSRAM | **OPI PSRAM** |
| Partition Scheme | any scheme including **FATFS** |

The partition scheme matters: the event log lives in a FAT partition.
Without a FATFS share the boot log reports
`[log] FEHLER: FFat nicht verfuegbar`.

A visual reference ships with the demo package as
`Tools Configuration.png`.

### 5. Upload and set up

With no stored WiFi credentials the device opens a hotspot named
`LifeBar-XXXX`. Connect to it, the portal opens by itself, then either
enter your WiFi there or go straight to `/setup`.

**Mind the order:** set up your skills via `/setup` first, then enter
the WiFi credentials. The other way round the hotspot closes before you
got to the setup page.

**Factory reset:** hold the BOOT button, plug the device in, keep
holding for two seconds.

---

## Using the device

The main screen sits in the middle, three directions around it:

| Gesture | Target |
|---|---|
| left / right, anywhere | switch between skills and vices |
| swipe down from the top | weekly review |
| down again | settings |
| swipe up from the bottom | lock screen |

On subpages the opposite direction goes back one level, starting from
the edge that shows the grip.

**Logging:** tap a column to select it, then the large areas at the
bottom do the logging — minus on the left, plus on the right. Tapping
the column again clears the selection. The selection persists after
logging, so 1.5 h of gym is one tap to select and three to add.

The lock screen appears by itself after three minutes of inactivity.

---

## Web interface

Reachable at `http://lifebar.local` or the IP from the boot log. On
Windows `lifebar.local` needs the Bonjour service installed.

| Path | Purpose |
|---|---|
| `/` | Stats: calendar, time of day, distribution, history, logging |
| `/setup` | Skills, vices, language |
| `/add` | Backfill time with date and clock time |
| `/import` | Edit and apply the data set |
| `/export` | Download the data set as a text file |
| `/events` | Event log as CSV, `?since=UNIXTIME` for increments |
| `/info` | Status as JSON |
| `/book` | Logging, used by the stats page |

The page is plain HTML, CSS and SVG with no framework and no CDN, about
19 KB in PROGMEM. It therefore works even when the router has no
internet connection.

### Import and export format

Plain text, readable and editable by hand. Lines starting with `#` are
ignored. These lines **set** values:

```
lang=en
week_goal=600
bonus=90
skill,GYM,1230
vice,SOCIAL,12
```

Event lines **add** and carry a timestamp:

```
event,2026-08-17,19:30,skill,GYM,90
event,2026-08-17,21:00,vice,STREAMING,1
```

Amount is minutes for skills and a count for vices. The week is derived
from the date, so an entry lands correctly in the current or previous
week and shows up in the calendar.

---

## Files

### Own code (GPLv3)

| File | Contents |
|---|---|
| `Lifebar.ino` | Startup, main loop, compile switches |
| `lifebar_i18n.h` | String table, languages, lead words, weekdays |
| `lifebar_model.h` | Data model, level curve, titles, week logic, NVS |
| `lifebar_log.h` | Event log on FAT, 8 bytes per entry |
| `lifebar_net.h` | WiFi, captive portal, web server, all endpoints |
| `lifebar_page.h` | Stats page as a PROGMEM string |
| `ui_lifebar.h` | All screens, gestures, colours |

Include order is `i18n → model → log → net → ui`. Anything two layers
need belongs in the lower of the two — otherwise it will not compile.
With header-only code and `static` that is the most common mistake.

### Bundled

`lb_emoji_28.c` — the icons, generated as an LVGL bitmap font from
**Noto Emoji** using the font converter at `lvgl.io`. Ships with the
repo. Licence notice in `OFL.txt`.

### Third-party files

The Waveshare files from step 3 are **not** in this repo because the
original states no licence. The changes they need are documented in
`docs/waveshare-patches.md`.

---

## Compile switches

At the top of `Lifebar.ino`:

| Switch | Effect |
|---|---|
| `USE_NET` | 0 = radio stays off entirely |
| `USE_WEB` | 0 = WiFi without web server, DNS and mDNS |
| `USE_MDNS` | 0 = no mDNS, device reachable by IP only |
| `BOOT_DEBUG` | Boot markers on the serial monitor |
| `HEAP_DEBUG` | Log memory usage |

Plus `UI_DEBUG` in `ui_lifebar.h` and `TOUCH_DEBUG` in `lvgl_port.c`.

The first three are the tool for narrowing down the touch issue
described below.

---

## Known issues

### Touch and network services

On this board the I2C reads of the touch controller return corrupted
data under certain network conditions: all 32 bytes hold the same value
while the transfer reports `ESP_OK`. The fill value changes between
sessions.

Established: with `USE_NET 0` the touch works reliably, with network
services it does not always. The touch path itself is unmodified
Waveshare original, and the vendor's LVGL 8 example uses identical code
— switching LVGL versions changes nothing about it.

Mitigations in the code:

- **Sanity check** in the touch callback: reads where the first eight
  bytes are identical get discarded. At one poll every 30 ms that goes
  unnoticed
- Touch bus at 100 kHz instead of 300 kHz
- WiFi in power save mode

### Others

- **Backlight PWM** has no effect, see board revision above
- **Several weeks without power** are detected as a gap when rolling
  forward: zero weeks go into the history, the seal streak breaks, and
  the review names the gap
- **Without a valid clock** the week logic rests, night mode does not
  switch, and log entries get a placeholder timestamp. They are redated
  once NTP succeeds, but only within the same session. The PCF85063 on
  the board is not used by the firmware

---

## Licence

Own code under **GPLv3**.

Third-party components keep their own licences:

- `lb_emoji_28.c` — SIL Open Font License 1.1, see `OFL.txt`
- `src/touch/esp_lcd_touch.*` — Apache-2.0 (Espressif)
