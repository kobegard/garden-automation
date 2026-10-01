# Garden Automation Mark 2: firmware and Home Assistant logic

Each grow room has six ESPHome nodes, and Home Assistant closes every control
loop. No board talks to another board directly.

```
01 reservoir ──pH, EC, temp, level, flood──► HA ──timed doses──► 02a + 02b ──► pumps 1-8
04 environment ──T, RH, CO2, VPD──────────► HA ──speed %──────► 05 ──► 2x Cloudline
                                            HA ──schedule/ramp─► 06 + Kasa plugs ──► 4x SE7000
                                            HA ──CO2 plug──────► Kasa (CO2 regulator)
```

## Layout

| Path | What it is |
|---|---|
| `esphome/common/base.yaml` | WiFi, API, OTA and diagnostics, shared by every node |
| `esphome/common/d1_mini.yaml` | Wemos D1 mini (ESP8266) platform for boards 02/04/05/06 |
| `esphome/packages/0X_*.yaml` | One package per board design: pin map, sensors, safety logic |
| `esphome/roomNN-*.yaml` | Per-device files: name, room and per-unit settings |
| `homeassistant/blueprints/automation/grow/` | Control loops as blueprints, one automation per room per loop |
| `tools/new_room.py` | Generates the six device files for another room |

## Getting started

```bash
cp esphome/secrets.yaml.example esphome/secrets.yaml   # fill in; it is git-ignored
python3 tools/new_room.py 2 3 4                        # rooms 02-04 (room 01 is committed)
cd esphome && esphome run room01-dosing-a.yaml         # first flash over USB, then OTA
```

To install the blueprints, copy `homeassistant/blueprints/automation/grow/` into
`<config>/blueprints/automation/`. Then go to **Settings → Automations →
Blueprints** and create one automation per room from each blueprint.

## Pin maps used (and where they come from)

| Board | MCU | Pins | Source |
|---|---|---|---|
| 01 Reservoir | FeatherS3[D] (ESP32-S3) | I2C IO8/IO9 · A02YYUW UART rx IO43 / tx IO44 · DS18B20 IO5 · flood IO6 (S1), IO14 (S2) · LDO2 enable IO39 | Netlist 2026-09-19 + FeatherS3[D] pinout; H6.4=GND, H6.16=LDO2, H5.10=5V fix header orientation |
| 02 Dosing | D1 mini | Pump 1 GPIO5 · Pump 2 GPIO13 · **Pump 3 GPIO14** · Pump 4 GPIO4 | Netlist 2026-08-30 + silkscreen (review F2). The old doc's GPIO12 is wrong |
| 04 Environment | D1 mini | SDA GPIO4 · SCL GPIO5 | Review 2026-09-18 |
| 05 Fan | D1 mini | Fan 1 GPIO12 · Fan 2 GPIO14 (inverted PWM, 5 kHz) | Netlist 2026-09-18 + review |
| 06 Light | D1 mini | DIM GPIO14 (inverted PWM, 1 kHz) | Board-06 audit addendum |

`01_reservoir_sensor_hub.md` (GPIO21/22/17/16/4/19/23) describes the old
ESP32-DevKitC design and is stale. The firmware follows the 2026-09-19 netlist.

**Meter-check every board's GPIOs at commissioning,** especially Board 02. Its
pump map has been corrected twice.

## Safety built into the firmware

- **Board 02:** HA asks for "N mL". The ESP converts that using the per-pump
  flow calibration and runs the dose as a timed on-device script, so a WiFi
  drop mid-dose can't leave a pump running. Other safeguards:
  - each dose is capped at 60 s;
  - every pump (manual switch included) has a 90 s hard watchdog;
  - only one pump runs at a time;
  - "Dosing enabled" OFF stops everything and rejects new doses.
- **Board 01:** LDO2 (the sensor rail) is held on. The flood pull-ups live on
  that rail, so flood readings are ignored whenever it is off. Without that,
  both inputs would read "wet".
- **HA blueprints:**
  - pH, EC and CO2 skip any reading older than a few minutes;
  - dosing skips while any blocker is on;
  - the interlock blueprint turns off "Dosing enabled" on flood, low level, or
    if the reservoir hub goes offline for 5 minutes, and dosing stays off until
    you turn it back on;
  - CO2 runs only with the lights on, under a temperature limit and a
    hard ppm ceiling.

## Commissioning checklist

1. **Board 01:** switch each Atlas EZO-EC to I2C mode, once per unit.
   Calibrate pH with the "capture pH 7 / pH 4" buttons in buffer solutions.
   Set `tank_empty_mm` and `tank_full_mm` in `room01-reservoir.yaml`.
   Continuity-check H2.4 to the Feather TX pad (review F6).
2. **Board 02:** calibrate each pump's flow rate (mL/min) by timing a measured
   volume. Kamoer's listing gives anything from 5.2 to 90 mL/min.
3. **Board 05:** on the first assembled 09-18 board, diode-test D3/D4 before
   plugging in a fan (see "Hardware" below). Check fan steps 1, 5 and 10
   against the fan's display.
4. **Board 06:** measure the DIM+ floor and the SE7000's default brightness when
   DIM+ is undriven. Confirm the RJ11 pinout with an ohmmeter.

## Open items

- **AS7343 (0x39) and MLX90632 (0x3A) on Board 04 have no ESPHome driver**
  (checked against ESPHome 2026.6). They need external components. Leaf VPD
  depends on the MLX90632. The I2C scan at boot will still show whether
  either chip answers.
- **DFR0997 display (Board 01):** no ESPHome driver either.
- **Pump chemical assignments:** pumps 1 and 2 are pH Up and pH Down (from the
  old YAML). Pumps 3-8 are still TBD and named "Pump N".
- **WH52 soil probe:** no firmware yet. The moisture receive path (D2, review
  F3) and the ADC2 thermistor (F4) should be bench-tested first; see
  `WH52_D2_ADC2_Research_2026-09-18`. Nothing in the system acts on soil data yet.
- **Compile:** every config passes `esphome config` (ESPHome 2026.6.5). A full
  `esphome compile` hasn't been run because the build sandbox couldn't reach
  the PlatformIO registry. Run one compile per board type before the first flash.

## Hardware

- **Board 05 F1 (reversed power diodes):** fixed in the 2026-09-18 netlist.
  D1/D2 have the cathode on H1.4, and D3/D4 have pin 1 on the fused node with
  pin 2 on GND. This assumes pin 1 is the cathode on the D3/D4 footprint,
  which differs from the D1/D2 footprint.
- **Board 01 F5 (ADS1115 logic level):** fixed in the 2026-09-19 netlist. U2
  VDD moved to VCC (3.3 V).
