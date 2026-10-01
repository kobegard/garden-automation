# WH52 bench test

Checks the current WH52-Rival board (netlist `PCB1_2026-09-09`) against the
2026-09-18 review and the D2/ADC2 research doc **before** paying for a respin.
It needs no signal generator: the ESP32-C3 makes its own excitation.

## Flash it

1. Arduino IDE with the **esp32 core 3.x**, board **ESP32C3 Dev Module**.
2. **Tools → USB CDC On Boot → Enabled.** The sketch refuses to compile
   without it, because the WH52 only has USB.
3. Fit a charged cell (above 3.6 V), plug in USB-C, upload `bench_test.ino`,
   and open the Serial Monitor at 115200 baud.
4. Hold the tip in each medium for about 20 s (5 reports): **air**, **tap
   water**, **moist soil**, then **dry soil**. Copy the output for each.
5. Optional: put a DMM (DC volts) from C7 (the IO3 node) to GND while the tip
   is in moist soil.

Each report takes about 4 s.

## What the results mean

| Line | If you see | It means |
|---|---|---|
| `IO3` (moisture receive) | ≈ 0 mV in every medium | **F3 confirmed.** D2 makes a negative-only detector. Respin: swap D2 pin 1 / pin 2 nets. |
| DMM on C7 | negative DC in moist soil | F3 confirmed, and the node goes past the −0.3 V absolute max |
| `IO3` | rises clearly from air to water | F3 is **not** real; tell me and I'll re-check the model |
| `IO1 p-p` / `hi` | big in air (~3.3 V, saturating ~2.5 V), smaller in dry soil, much smaller in water | The **firmware fallback works**: research model predicts wet 0.17 V, moist 1.0 V, dry 2.5 V p-p |
| `IO10 hi-Z` columns vs as-built | IO1 / IO3 swing grows with IO10 hi-Z | R8/C6 loading confirmed. Respin: make R8's ground return switchable |
| `EC IO6 high %` | changes between tap water and soil | EC path responds (it's digital only, so it's a coarse threshold) |
| `TH1` | flat 0 mV, "ADC2 unavailable" | **F4 confirmed:** ESP-IDF 5 won't run ADC2 oneshot on the C3. Respin: swap the IO4/IO5 roles so TH1 lands on ADC1_CH4 |
| `TH1` | reads, but `spread` is tens of mV or the temperature jumps | F4 confirmed (Espressif: "results are not stable") |
| `TH1` | steady, sensible room temperature | ADC2 works on this core version; a respin for F4 is optional |
| `battery` | 3.0-4.2 V and tracks a DMM across BT1 within ~0.05 V | Battery divider OK |

**Also check while the board is powered:** use a DMM on **IO2 and IO0 with
the sketch idle** (between reports, IO2 is released). Both sit near battery
voltage, up to 4.2 V, through 100-200 kΩ. That's above the C3's 3.6 V
absolute maximum (review F9). It's current-limited, so it's a long-term
reliability issue, not an instant failure, but it belongs in the respin.

## Respin list (if the bench confirms)

1. D2: swap pin 1 and pin 2 nets (pin 1 → GND, pin 2 → IO3 node).
2. Swap the IO4/IO5 roles so TH1 is on ADC1_CH4 and the moisture drive moves to IO5.
3. Make R8's ground return switchable (N-FET or a spare GPIO).
4. Series resistors on IO1 and IO3 against negative excursions.
5. Rework the battery sense so IO0/IO2 can't exceed 3.3 V, e.g. a high-side P-FET switch.
6. USB ESD: USBLC6-2SC6 (already used on Board 04).
7. BT1 reverse-polarity protection.

Send me the serial output for all four media and I'll interpret it and,
if needed, check the respin netlist against this list.
