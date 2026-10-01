# Board 01 — Reservoir Sensor Hub

**Source of truth:** `Netlist_01_reservoir_sensor_hub_2026-09-19.tel`, `BOM_…_2026-09-19.xlsx`, `SCH_…_2026-09-19.pdf`, `PCB_…_2026-09-19.pdf`, `Gerber_…_2026-09-19.zip`, `PickAndPlace_…_2026_09_19.xlsx`, plus the FeatherS3[D] pinout and the external datasheets named below.

> **Rewritten 2026-10-01.** The previous copy of this doc described the abandoned ESP32-DevKitC design (GPIO21/22/17/16/4/19/23, an H3 display header, C1 100 µF / C2 47 µF, R7/R8 LED resistors). None of that matches the 09-19 CAD. The 2026-09-18 full-system review flagged it as stale (conflict C5). Everything below is re-derived from the 09-19 files.
>
> Evidence: **[V]** = checked directly in the 09-19 CAD or a named datasheet in this pass. **[V-doc]** = carried from an earlier audit, not re-derived. **[I]** = inferred. **[U]** = not established.

## MCU

**Unexpected Maker FeatherS3[D]** (ESP32-S3, 16 MB flash, 8 MB PSRAM, native USB-C), plugged into two female headers [V BOM]:

- **H5:** 12-pin, Bossie BX-PM2.54-1-12PY (LCSC C18078134).
- **H6:** 16-pin, HOAUC 2685Y-116CNG1SNA01 (LCSC C350305).

Both headers are real BOM lines with LCSC numbers, so **JLCPCB places them**. That replaces the "hand-source 2× 1×19 headers" plan in `System_Overview.md`, which applied to the DevKitC footprint. The Feather module itself (U1, footprint `COMM-TH_L50.8-W22.9…`, no nets) is hand-populated [V BOM/net]. Contact plating of C18078134 and C350305 is **[U]**; check on jlcpcb.com/partdetail.

**Header orientation — derived from the power nets [V]:**

- **H6** counts from the Feather's RST end: H6.4 = GND and H6.16 = LDO2 3V3 both match the Feather pinout only in that direction.
- **H5** counts from the SDA end: H5.10 = 5V and H5.1/H5.2 = SDA/SCL.

The two headers count in opposite directions, so check the pin-1 marks against the Feather silkscreen when you first fit it.

## GPIO map [V netlist 09-19 + FeatherS3[D] pinout]

| Function | Board net / pin | Feather pin | GPIO |
|---|---|---|---|
| I2C SDA (ADS1115, EZO-EC, DFR0997, Feather fuel gauge) | `I2C_SDA` H5.1 | SDA | **IO8** |
| I2C SCL | `I2C_SCL` H5.2 | SCL | **IO9** |
| Ultrasonic: sensor TX (output) → ESP RX | net `RX` H2.4 → H6.15 | TX pad | **IO43** (used as RX via the GPIO matrix) |
| Ultrasonic: ESP TX → sensor RX/mode | net `TX` H2.3 → H6.14 | RX pad | **IO44** (used as TX) |
| DS18B20 1-Wire | H7.3 → H6.10 | 5 | **IO5** |
| Flood S1 | S1.1 → H6.9 | 6 | **IO6** |
| Flood S2 | S2.1 → H6.7 | 14 | **IO14** |
| Sensor rail (LDO2) enable | on the Feather | — | **IO39** |
| pH analogue | H1.3 → U2 AIN0 (ADS1115, 0x48) | — | via I2C |

**Ultrasonic UART — corrected 2026-10-01 (review F6).** The net names `TX`/`RX` follow the *Feather's* pads, not the sensor's. DFRobot's A02YYUW pinout is 3 = RX (mode input) and 4 = TX (output). So the sensor's output lands on IO43 (the Feather "TX" pad). The firmware sets `rx_pin: GPIO43, tx_pin: GPIO44`, which the ESP32-S3 GPIO matrix allows. Holding IO44 idle-high puts the sensor in its processed-output mode (~100 ms per reading) [V DFRobot wiki]. At boot, the ROM log on U0TXD (IO43) briefly contends with the sensor's TX output [I]. That's harmless in practice, but worth knowing. **Continuity-check H2.4 → Feather TX pad once before first power-up.**

## Connectors [V netlist 09-19]

| Ref | Function | Part | Pinout | Plating |
|---|---|---|---|---|
| H1 | DFR0504 pH isolator, MCU side | Bossie BX-PH2.0-3PZZ (C18077745) | 1 = +5V, 2 = GND, 3 = AIN0 | [U] |
| H2 | A02YYUW ultrasonic | CAX PH2.0-4P (C722763) | 1 = VCC, 2 = GND, 3 = net `TX` (sensor RX), 4 = net `RX` (sensor TX) | [U] |
| H4 | Atlas EZO-EC on Isolated Carrier | CJT A2541WV-5P (C225480) | 1 = VCC, 2 = NC (carrier OFF), 3 = GND, 4 = SDA, 5 = SCL | Gold [V-doc] |
| H5 / H6 | FeatherS3[D] sockets | see MCU | — | [U] |
| H7 | DS18B20 (via the probe's adapter board) | HanElectricity 2541WV-03P (C5383112) | 1 = GND, 2 = VCC, 3 = DATA | [U] |
| S1 / S2 | TKOWTB flood probes | JXTCONN C254128V-2P0G36 (C49291872) | 1 = signal, 2 = GND | Tin |

There is **no H3**. The DFR0997 display connects to the Feather's own STEMMA QT port, which is on the same IO8/IO9 bus [V netlist + pinout].

### H1 cable — must be re-pinned (new finding 2026-10-01) [V DFRobot wiki]

The DFR0504's MCU-side connector is ordered **A (signal), + (5 V), − (GND)**. H1 is ordered **+5V, GND, signal**. A stock Gravity PH2.0 cable is wrong in *both* orientations:

- Straight through, it puts +5V on the isolator's analogue output.
- Reversed, it reverses the isolator's supply.

**Re-pin the cable: H1.1 → '+', H1.2 → '−', H1.3 → 'A'.** This closes the review's open item "[U] H1 pin order vs the DFR0504 cable" with a negative result. A future PCB revision could reorder H1 to A/+/− so stock cables fit.

Also: the DFR0504 wants **5.0 ± 0.1 V** on its MCU side [V DFRobot]. The board's `+5V` is the Feather 5V pin through F1 (PTC), so measure it under load.

### H4 — matches the Atlas carrier [V carrier datasheet]

The Isolated Carrier header is VCC, OFF, GND, TX, RX. In I2C mode TX becomes SDA and RX becomes SCL, which matches H4. OFF is left unconnected, which keeps the carrier on. The EZO-EC ships in UART mode and must be switched to I2C (address 0x64) once per unit.

### H7 / DS18B20 — pull-up comes from the probe kit [V DROK manual]

The 1-Wire net (`$1N234`: D3, H6.10, H7.3) has **no pull-up on this board**. The DROK probe's adapter board carries a 4.7 kΩ ("472") pull-up and a DAT/VCC/GND header. That header order is the reverse of H7, so a straight pin-1-to-pin-1 Dupont lead swaps GND and DATA. **Wire by label.** Probe wires: red = VCC, yellow = DATA, black = GND.

## Power [V netlist/BOM 09-19]

- **`+5V`:** Feather 5V pin (H5.10) → **F1** SMD1206P075TF/16 (750 mA hold) → H1 (DFR0504) and LED1. D1 (PESD5V0S1UB-N) clamps it. C2 22 µF and C3 1 µF decouple it.
- **`VCC` (3.3 V, LDO2):** Feather H6.16, switched by IO39. Feeds H2, H4, H7, **U2 (ADS1115)**, the I2C pull-ups R1/R2 (4.7 kΩ), the flood pull-ups R3/R4 (10 kΩ) and LED2. C1 22 µF and C4–C7 1 µF decouple it.
- **Supply:** SHNITPWR 5 V/3 A USB-C adapter → ANMBEST IP67 USB-C feedthrough → JBTOP right-angle USB-C cable → Feather [V-doc].

**ADS1115 logic level — fixed in the 09-19 netlist (review F5).** U2.8 (VDD) moved from `+5V` to `VCC`, so the bus's 3.3 V highs now meet the ADS1115's V_IH (0.7 × VDD). The 0–3.0 V pH signal still fits within VDD.

**LDO2 is kept on permanently in firmware.** Earlier drafts gated the sensor rail for power saving. That's not worth it on a USB-powered board, and it's unsafe here: R3/R4 pull up to `VCC`, so with LDO2 off both flood inputs read LOW, which means "wet". The firmware ignores flood readings whenever LDO2 is off, and only switches it off for the manual "Power-cycle sensors" button.

## Protection and indicators [V BOM 09-19]

- **D1** PESD5V0S1UB-N on `+5V`.
- **D2** PESD5V0L2BT on I2C SDA/SCL.
- **D3** PESD5V0L2BT on the DS18B20 data line.
- **D4 / D5** PESD5V0L2BT on S2 / S1.
- **LED1:** KT-0805Y with R5 560 Ω, on `+5V`.
- **LED2:** NCD0805R1 with R6 330 Ω, on `VCC`.

## pH / EC chains

- **pH:** SEN0169-V2 industrial probe → transmitter V2 (0–3.0 V out [V DFRobot]) → DFR0504 isolator → H1 → **U2 ADS1115IDGSR** AIN0, address 0x48 (ADDR tied to GND) [V]. **The firmware must set gain ±4.096 V**; the default ±2.048 V clips. Calibrate pH with buffers using the firmware's "capture pH 7 / pH 4" buttons.
- **EC:** Mini K1.0 probe → EZO-EC on the Electrically Isolated Carrier → H4, address 0x64. Temperature compensation comes from the DS18B20 [V firmware].

## Firmware

`garden-automation/esphome/packages/01_reservoir_sensor_hub.yaml`, using the pins above. It includes the DFR0997 dashboard (`esphome/components/dfr0997`). The config validates; it hasn't been flashed yet.

## Board

100.0 × 100.0 mm outline [V-doc review, Gerber 09-01; 09-19 outline not re-measured]. 18 mounting holes: 16× M3 (4 enclosure corners, SEN0169, DFR0504, Atlas carrier) and 2× M2 (DS18B20 adapter) [V BOM].

## Outstanding

**Blocking before first power-up:**
- Re-pin the H1 → DFR0504 cable (see H1 above).
- Continuity-check H2.4 → Feather TX pad (UART sense, F6).
- Wire the DS18B20 adapter to H7 by label, not pin order.
- Switch the EZO-EC to I2C mode, once per unit.

**Optional, not blocking:**
- Confirm H5/H6 (C18078134, C350305) and H1/H2/H7 contact plating on jlcpcb.com/partdetail.
- Measure `+5V` at H1 under load against the DFR0504's 5.0 ± 0.1 V spec.
- Re-measure the 09-19 board outline from the Gerber.
- Enclosure: the 150 × 150 IP67 hinged box (drawing received 2026-10-01) vs the earlier Otdorpatio 270 × 130 × 110 instruction is still unresolved (review C6).
- Consider reordering H1 to A/+/− in a future revision so stock Gravity cables fit.
