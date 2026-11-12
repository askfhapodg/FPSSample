# Antenna rotator

A DIY pan/tilt rotator that puts an **ACASOM ROC-4** booster antenna
(0.53 kg, 230 × 146 × 39 mm) on a **K&M 20800** overhead stand (3/8" thread,
up to 3.1 m) and lets you aim it from the base of the stand or from a phone.

All electronics sit in a box at the base. Only two servos go up the mast.

**Status:** the firmware compiles for ESP32 and the motion/command logic has
passing unit tests. It has **not been run on real hardware yet** — treat pin
choices, servo calibration and the parts list as a first build to be checked
on the bench before the antenna goes up.

## How it is controlled

| Input | Use |
|---|---|
| Phone, over the rotator's own Wi-Fi | Hold-to-move arrows, go-to azimuth/elevation, STOP |
| USB serial (115200 baud) | Same commands as text lines; this is the hook for a future tracker |
| Wired joystick at the base (optional) | Works with no radio at all |

Every input uses one small text protocol, so anything that can compute an
azimuth and elevation (a laptop, a Raspberry Pi, a tracking script) can drive
the antenna without firmware changes.

## Parts

| Part | Notes |
|---|---|
| ESP32 DevKit (ESP32-WROOM-32, 30 or 38 pin) | Any `esp32dev`-compatible board |
| 2 × 270° digital metal-gear servo, 30–35 kg·cm class | e.g. DS3235SG-270. Check the listing says 270° and 500–2500 µs |
| Metal 2-axis pan/tilt bracket **with a pan bearing**, for standard-size servos | The bearing must carry the antenna's weight, not the servo shaft |
| UBEC / buck converter, 6 V output, 8 A or more | Powers both servos and the ESP32 |
| 12 V source: 3S Li-ion pack or a USB-C PD 12 V trigger + power bank | |
| 2 × electrolytic capacitor 1000–2200 µF, 10 V or more | One at the servos, one at the ESP32 |
| 2 × 330 Ω resistor | In series with each servo signal wire |
| 4 m shielded 4-core cable, power pair 0.75 mm² (18 AWG) or thicker | Base box to mast head |
| Clip-on ferrite, 1–2 pcs | On the cable just below the servos |
| Plate or flange with a 3/8" female thread | Joins the pan/tilt base to the stand |
| 1/4"-20 screw, about 10 mm, plus washer | Fixes the ROC-4's ball-head mount to the tilt plate (check the thread in its base first) |
| Optional: KY-023 joystick module | Wired control at the base |
| Enclosure, fuse (5 A), switch, sandbag for the stand | |

## Wiring

```
12 V ──fuse──switch──► UBEC 6 V ──┬──────────────► servo + (both)      ┐
                                  ├──► ESP32 VIN                       │ up the
GND ──────────────────────────────┴──► ESP32 GND ─► servo − (both)     │ mast,
ESP32 GPIO18 ──330 Ω────────────────────────────► pan servo signal     │ shielded
ESP32 GPIO19 ──330 Ω────────────────────────────► tilt servo signal    ┘

Optional joystick:  VRx → GPIO34   VRy → GPIO35   SW → GPIO32   + → 3V3   GND → GND
```

- Put one capacitor across + and − at the mast head and one at the ESP32 VIN
  pin. Without them a servo starting up can reset the ESP32.
- Connect the cable shield to GND at the base end only.
- Pins and Wi-Fi settings live in `src/config.h`.
- Set `HAS_JOYSTICK` to `1` only when a joystick is really connected;
  unconnected analog pins would move the antenna on their own.

## Mechanical assembly

1. Screw the 3/8" plate onto the stand's thread and bolt the pan/tilt base to it.
2. The ROC-4 ships with a mini ball head whose locking shoe grips the antenna's
   bottom edge, between the two N connectors. Screw the ball head's base to the
   tilt plate with the 1/4" screw, set the antenna upright and lock the ball hard.
   The antenna then stands about 15 cm above the tilt plate, so keep tilt angles
   moderate; the default limits (−15° to +60°) are a starting point to test.
3. Power the electronics **before** attaching the servo horns, so the servos
   sit at their saved position (centre on first boot). Attach the horns with the
   antenna facing straight ahead and level.
4. Tie the coax and the servo cable to the mast with a loose loop at the top
   that allows the full ±130° sweep without pulling on the N-type connectors.
5. Weigh the base down. The head adds roughly 0.5 kg on top of the antenna,
   and the stand was designed for a microphone.

## Build and flash

```sh
pip install platformio
pio run -t upload        # compile and flash over USB
pio device monitor       # serial console, 115200 baud
make test                # unit tests on the PC, no hardware needed
```

Change `WIFI_PASSWORD` in `src/config.h` before the first flash.

## Using it

Join the Wi-Fi network `antenna-rotator` and open `http://192.168.4.1/`.

- **Arrows** move while held and stop on release. If the connection drops,
  the antenna stops by itself after 0.6 s.
- **Go** moves to an azimuth and elevation.
- **Set** tells the rotator which compass bearing the antenna is pointing at
  now. After that, azimuths are real compass bearings. The value is saved.

### Commands

Send as a serial line, or as `POST /api/cmd` with form field `c`.
Replies start with `OK` followed by the status, or `ERR` and a reason.

| Command | Effect |
|---|---|
| `GOTO <az> <el>` | Point at azimuth/elevation in degrees |
| `PAN <deg>` / `TILT <deg>` | Move one axis to a mechanical angle (0 = centre) |
| `JOG <pan °/s> <tilt °/s>` | Move at a rate; must be repeated at least every 0.6 s |
| `STOP` | Decelerate and hold |
| `HOME` | Pan 0, tilt 0 |
| `NORTH <bearing>` | Declare the current pointing direction as this compass bearing |
| `STATUS` | `OK az=… el=… pan=… tilt=… north=… moving=0/1` |

```sh
curl -d 'c=GOTO 45 10' http://192.168.4.1/api/cmd
```

## Limits and things to know

- **Pan covers ±130°, not a full circle.** Hobby servos cannot turn
  continuously, and the coax could not follow anyway. Face the stand towards
  the middle of the sector you work in. A request outside the range returns
  an error instead of moving somewhere else.
- **No position feedback.** The firmware knows where it told the servos to go,
  not where they are. A stalled or overloaded servo goes unnoticed.
- **Wi-Fi shares the 2.4 GHz band with the drone link.** The ROC-4 transmits
  10 W there, so the phone connection may drop while flying on 2.4 GHz, and the
  access point adds a little noise of its own. If that happens, change
  `WIFI_CHANNEL`, fly on 5.8 GHz, or use the joystick or USB serial, which need
  no radio.
- **RF pickup.** The servo cable runs next to a 10 W transmitter. If the servos
  twitch while the booster transmits, the shielded cable, ferrite and series
  resistors above are the fix; a 3.3 V → 5 V level shifter on the signal lines
  is the next step.
- The antenna position is saved after each move, so power-up does not swing
  the antenna. If it was turned by hand while off, it will snap back.

## Code layout

| Path | Contents |
|---|---|
| `lib/rotator/` | Portable core: axis motion profile, limits, command protocol |
| `src/main.cpp` | ESP32 glue: servo PWM, Wi-Fi, web server, serial, joystick, storage |
| `src/config.h` | Pins, Wi-Fi, servo calibration, speeds and limits |
| `src/web_ui.h` | The phone control page |
| `test/host/` | Unit tests for the core |

## Roadmap: drone tracking

Tracking reduces to sending `GOTO <az> <el>` a few times per second. What it
still needs:

1. **Where the mast is and which way it faces** — a GPS module and a compass
   (or the manual `NORTH` setting used today).
2. **Where the drone is** — live latitude, longitude and altitude from the
   drone's telemetry, delivered to the rotator over serial or Wi-Fi.
3. **The maths** — bearing and elevation from the two positions, added either
   as a `TRACK <lat> <lon> <alt>` command in the core or in a companion script.

A full-circle pan axis (stepper motor with a belt drive and a homing sensor)
would replace the pan servo; the `Axis` class is the only part that changes.
