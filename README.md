# Antenna rotator

A DIY rotator that turns an **ACASOM ROC-4** booster antenna (0.53 kg,
230 × 146 × 39 mm) on top of a **K&M 20800** stand (3/8" thread, up to 3.1 m).
You aim it from a phone or from the base of the stand, and the next step is
automatic tracking of one DJI Mavic (see [Tracking plan](#tracking-plan)).

The recommended build turns the antenna left and right only (**pan-only**).
Tilt is set once by hand on the ball head that comes with the ROC-4: the
antenna's beam is tens of degrees tall, and a drone at 120 m altitude is less
than 14° above the horizon once it is 500 m away, so motorised tilt adds a
servo, weight and wind load for little gain. A second servo can be added
later (`HAS_TILT` in `src/config.h`).

All electronics sit in a box at the base. Only the servo goes up the mast.

**Status:** the firmware compiles for ESP32 and the motion/command logic has
passing unit tests. It has **not been run on real hardware yet**, and the
tracking part is planned, not written. Check the servo and mechanics on the
bench before the antenna goes up.

## How it is controlled

| Input | Use |
|---|---|
| Phone, over the rotator's own Wi-Fi | Hold-to-move arrows, go-to azimuth, STOP |
| USB serial (115200 baud) | Same commands as text lines |
| Wired joystick at the base (optional) | Works with no radio at all |
| Remote ID receiver (planned) | Automatic tracking of one drone |

Every input uses one small text protocol, so anything that can compute an
azimuth can drive the antenna.

## Parts

Prices and stock were checked on **anodas.lt** on 12 November 2026 (shops in
Vilnius, Vytenio g. 20, and Kaunas, Pramonės pr. 4D; courier 4.90 €, free
over 150 €). Items marked "4–7 days" are ordered in by the shop.

### Electronics (anodas.lt)

| Part | Code | Price | Notes |
|---|---|---|---|
| ESP32 DevKit, ESP-WROOM-32, 30 pin | AN-08893 | 11.50 € | Rotator controller. In stock (Kaunas, central warehouse) |
| Servo PowerHD LF-20MG-270 | AN-08404 | 34.80 € | 270°, 20 kg·cm at 6 V (16.5 at 4.8 V), 4.8–6 V, ball bearings. 4–7 days. Not waterproof: cover it in rain |
| FireBeetle 2 ESP32-UE (DFRobot DFR1140) | AN-25401 | 26.90 € | Remote ID receiver for tracking; has an external-antenna connector. 4–7 days. Skip it until you want tracking |
| Antenna cable SMA socket – IPEX u.FL, 15 cm | — | 1.90 € | For the FireBeetle |
| Antenna 2.4 GHz, SMA plug, 110 mm | — | 3.00 € | Screws onto the cable above, through the box wall |
| Shielded cable 4 × 0.65 mm², per metre | — | 3.50 €/m | 4 m = 14.00 €. Base box to servo |
| Box Kradex Z124B, IP65, 185 × 145 × 39 mm | — | 7.00 € | |
| 2 × capacitor 2200 µF 50 V | — | ~2 € each | One at the servo, one at the ESP32 |
| Servo extension lead 15 cm | — | 2.40 € | Cut in half for a servo plug at the mast end |
| 330 Ω resistor, heat-shrink, hook-up wire | — | ~1 € | |
| USB power bank, 5 V ≥ 3 A, two ports (e.g. Kruger&Matz 10000 mAh PD) | — | 22.40 € | Or any power bank you already have |

Electronics total: about **107 €** without the power bank, about **75 €**
without the tracking receiver.

### Mechanics (not found at anodas.lt)

The pan axis needs a bearing so the servo only turns the antenna and does not
carry it. No ready-made heavy pan base with a bearing turned up in the
Lithuanian shops I searched, so this part is DIY. The parts below are common,
but I have not checked specific shops for them:

| Part | Notes |
|---|---|
| Turntable ("lazy susan") bearing, about 100 mm, steel | Furniture fitting from a DIY store. Its centre opening must clear the servo (41 × 21 mm) |
| 2 plates, about 120 × 120 mm: 3–4 mm aluminium or 12 mm plywood | Bottom plate on the stand, top plate on the bearing |
| 3/8"-16 UNC nut or threaded insert | Glued or clamped under the bottom plate so it screws onto the stand. Imperial threads are rarely stocked in Lithuanian DIY stores; photo and music shops sell them, e.g. Thomann |
| 1/4"-20 screw, about 10 mm, plus washer | Fixes the ROC-4's ball head to the top plate. Check the thread in the ball head's base first |
| M3/M4 screws, nuts and spacers | Servo to bottom plate, servo horn to top plate |

### Optional

| Part | Notes |
|---|---|
| Second LF-20MG-270 servo and a tilt bracket | Motorised tilt (`HAS_TILT 1`) |
| GPS module, e.g. DFRobot TEL0157 (29.80 € at anodas.lt) | Only if the stand will be far from where you fly from; see the tracking plan |
| Step-down converter XL4015 (6.40 € at anodas.lt) with a 9–12 V source | If the servo is weak or the ESP32 resets on 5 V (see Power) |
| KY-023 joystick module | Wired control at the base |
| Sandbag | For the stand's base |

## Power

The servo is rated 4.8–6 V, so the simplest supply is a 5 V USB power bank
with two ports:

- **Port 1:** a USB cable cut open, feeding the ESP32 DevKit's VIN pin and the
  servo.
- **Port 2:** the FireBeetle on its own USB-C cable.

Over 4 m of 0.65 mm² cable the servo sees roughly 4.7 V while moving. That is
slightly under its rated 4.8 V, so if it turns weakly or the ESP32 resets when
it starts, switch to 6 V: an XL4015 step-down set to 6.0 V and fed from 9–12 V
(a 3S battery, or a USB-C PD power bank with a 12 V trigger cable).

## Wiring

```
Power bank port 1 (5 V) ──┬──► ESP32 DevKit VIN       ┐
                          └──► cable core 1+2 ───────►│ servo +      up the mast,
GND ──────────────────────┬──► ESP32 DevKit GND       │              4-core
                          └──► cable core 3 ─────────►│ servo −      shielded
ESP32 GPIO18 ──330 Ω──────────► cable core 4 ────────►┘ servo signal

Power bank port 2 ──USB-C──► FireBeetle (Remote ID receiver, planned)
FireBeetle TX ──► ESP32 GPIO16 (RX2), plus a common GND

Optional joystick:  VRx → GPIO34   VRy → GPIO35   SW → GPIO32   + → 3V3   GND → GND
Optional tilt servo signal: GPIO19 (needs its own power cores)
```

- Put one 2200 µF capacitor across servo + and − at the mast end and one at
  the ESP32's VIN pin. Without them the servo starting up can reset the ESP32.
- Connect the cable shield to GND at the base end only, and clip a ferrite
  onto the cable just below the servo.
- Pins and Wi-Fi settings live in `src/config.h`.
- Set `HAS_JOYSTICK` to `1` only when a joystick is really connected;
  unconnected analog pins would move the antenna on their own.

## Mechanical assembly

```
          ROC-4
            │  ball head (comes with the ROC-4), 1/4" screw
   ┌────────┴────────┐  top plate ── servo horn screwed to its centre
   ╞═════════════════╡  turntable bearing (carries the load)
   │   ┌─servo─┐     │
   └───┴───────┴─────┘  bottom plate ── servo bolted on top, shaft up
            │  3/8" nut underneath
         K&M 20800
```

1. Fix the 3/8" nut to the centre of the bottom plate's underside and bolt the
   servo on top of the plate, shaft pointing up and centred.
2. Screw the turntable bearing's lower ring to the bottom plate around the
   servo, and the top plate to its upper ring. Shim with washers until the
   servo horn touches the top plate without lifting it, then screw the horn to
   the top plate's centre.
3. Power the electronics **before** screwing the horn on, so the servo sits at
   centre (or its saved position). Fit the horn with the antenna's mounting
   direction facing straight ahead.
4. Screw the ROC-4's ball head to the top plate, set the antenna upright and
   tilted up about 5–10°, and lock the ball firmly.
5. Tie the coax and the servo cable to the mast with a loose loop at the top
   that allows the full ±130° sweep without pulling on the N-type connectors.
6. Weigh the base down; with the antenna on top at 3 m the stand is
   top-heavy.

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
- **Go** moves to an azimuth (and elevation, if a tilt servo is fitted).
- **Set** tells the rotator which compass bearing the antenna is pointing at
  now. After that, azimuths are real compass bearings. The value is saved.

### Commands

Send as a serial line, or as `POST /api/cmd` with form field `c`.
Replies start with `OK` followed by the status, or `ERR` and a reason.

| Command | Effect |
|---|---|
| `GOTO <az> <el>` | Point at azimuth/elevation in degrees; in a pan-only build the elevation is ignored |
| `PAN <deg>` / `TILT <deg>` | Move one axis to a mechanical angle (0 = centre); `TILT` needs a tilt servo |
| `JOG <pan °/s> <tilt °/s>` | Move at a rate; must be repeated at least every 0.6 s |
| `STOP` | Decelerate and hold |
| `HOME` | Pan 0, tilt 0 |
| `NORTH <bearing>` | Declare the current pointing direction as this compass bearing |
| `STATUS` | `OK az=… el=… pan=… tilt=… north=… moving=0/1 tiltaxis=0/1` |

```sh
curl -d 'c=GOTO 45 10' http://192.168.4.1/api/cmd
```

## Limits and things to know

- **Pan covers ±130°, not a full circle.** Hobby servos cannot turn
  continuously, and the coax could not follow anyway. Face the stand towards
  the middle of the sector you work in. A request outside the range returns
  an error instead of moving somewhere else.
- **No position feedback.** The firmware knows where it told the servo to go,
  not where it is. A stalled or overloaded servo goes unnoticed.
- **Wi-Fi shares the 2.4 GHz band with the drone link.** The ROC-4 transmits
  10 W there, so the phone connection may drop while flying on 2.4 GHz, and the
  access point adds a little noise of its own. If that happens, change
  `WIFI_CHANNEL`, fly on 5.8 GHz, or use the joystick or USB serial, which need
  no radio.
- **RF pickup.** The servo cable runs next to a 10 W transmitter. If the servos
  twitch while the booster transmits, the shielded cable and series resistor
  above, plus a clip-on ferrite, are the fix; a 3.3 V → 5 V level shifter on the signal lines
  is the next step.
- The antenna position is saved after each move, so power-up does not swing
  the antenna. If it was turned by hand while off, it will snap back.

## Code layout

| Path | Contents |
|---|---|
| `lib/rotator/` | Portable core: axis motion profile, limits, command protocol |
| `src/main.cpp` | ESP32 glue: servo PWM, Wi-Fi, web server, serial, joystick, storage |
| `src/config.h` | Pins, Wi-Fi, pan-only/tilt choice, servo calibration, speeds and limits |
| `src/web_ui.h` | The phone control page |
| `test/host/` | Unit tests for the core |

## Tracking plan

Goal: the antenna follows **one DJI Mavic** automatically, and you can always
take over from the phone.

### Where the drone's position comes from

DJI's developer kit does not support the consumer Mavic 3 / Classic / Pro, so
an app on the controller is not an option for those models (the Enterprise
3E/3T/3M would work that way). What every recent Mavic sold in the EU does is
broadcast **Remote ID**: a Wi-Fi beacon with the drone's position, altitude
and the pilot's position, which an ESP32 can receive.

**Test this before buying the receiver.** Install the free *Drone Scanner*
app (Android/iOS), fly, and check that your Mavic appears and how far away it
stays visible. If it never appears, the drone does not broadcast Remote ID
(older models, or no EU class label), and tracking this way is not possible;
manual aiming still works.

### Range: the important limit

Phone reports for the Mavic 3's Remote ID are roughly 200–600 m. An ESP32 with
an external antenna may do better, but I have not been able to verify by how
much. So:

- **Close and middle range:** the antenna tracks automatically.
- **Beyond Remote ID range:** the rotator holds the last bearing. Far away
  the bearing changes slowly, so an occasional nudge from the phone is enough.
- Tracking at the booster's full 15–20 km would need a different method (for
  example finding the strongest drone signal by sweeping the antenna). That is
  a possible phase 2, not part of this plan.

### Other pieces

- **Where the stand is:** Remote ID also carries the pilot's position. With
  the stand within about 20 m of you, the error at 500 m is about 2°, well
  inside the beam, so no GPS module is needed. Add one only if the stand
  stands far from where you fly from.
- **Which way is north:** use the existing `NORTH` setting, either pointing
  the antenna at a landmark with a known bearing, or (planned) a "sync" button:
  point the antenna at the drone by eye and press it.
- **Radio interference:** the ROC-4 transmitting 10 W on 2.4 GHz can deafen
  the 2.4 GHz receiver. Keep the receiver and its antenna in the box at the
  base, behind the panel, and if reception drops while flying, set the drone
  link to 5.8 GHz.

### Software still to write

1. **Receiver firmware (FireBeetle):** decode Remote ID beacons, lock onto
   your drone by its serial number, and send its position and the pilot's
   position to the rotator over the serial link.
2. **Tracking in the rotator:** bearing from stand to drone, smoothing, a
   "hold last bearing" mode when the signal is lost, manual override from the
   phone, and the sync button.

A full-circle pan axis (stepper motor with a belt drive and a homing sensor)
could later replace the servo; the `Axis` class is the only part that changes.
