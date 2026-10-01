# Board 06 — Light Controller

Source of truth: `SCH_06_light_controller_2026-08-30.pdf`, `Netlist_06_light_controller_2026-08-30.tel`, `BOM_06_light_controller_06_light_controller_2026-08-30.xlsx`, `PickAndPlace_06_light_controller_2026_08_30.xlsx`, `Gerber_06_light_controller_2026-08-30.zip`.

## MCU
Wemos D1 mini (ESP8266; the photographed unit is a D1 mini V4.0.0, USB-C, 4 MB. A D1 Mini Pro also fits; same pinout). Single MOSFET-switched channel generating a PWM dimming signal for all 4 SE7000 LED drivers, daisy-chained together and dimmed as one group.

**MCU-socket architecture matches boards 02/04/05 — U1/U2's own footprint is a bare mechanical outline with zero pins** (`WEMOS_D1_MINI_footprint`; the schematic shows both "U1" and "U2" text stacked on the same box and the netlist/BOM call it U2 — a labeling leftover, not two parts). Confirmed via netlist: this footprint appears in no net. The actual GPIO wiring runs through two real placed BOM lines, **P1 and P2 — 1×8 female headers, BOOMELE, LCSC C27438** — the identical part already used for boards 02, 04, and 05's MCU sockets, marked Extended in the BOM/PCBA export. Whether JLCPCB is meant to assemble these headers (vs. the user hand-sourcing gold-plated headers as originally planned) is an open decision, not yet resolved — see `System_Overview.md`.

**GPIO pin identity — CONFIRMED 2026-09-02, via the PCB export's own designer-labeled reference layer.** Page 4 of `PCB_06_light_controller_2026-08-30.pdf` carries a full Wemos D1 Mini Pro pinout drawn directly on the documentation layer, with every P1/P2 pin position labeled by function (5V/GND/2/0/SDA/SCL/RX/TX on P1; 3V3/15/13/12/14/16/A0/RST on P2) — the same technique that resolved board 02's GPIO mapping. Cross-checked pin-1 identity on both headers via their footprint's square pad (the standard pin-1 marker): P1's square pad sits at the physical bottom of the header, next to "TX" — counting TX(1)/RX(2)/SCL(3)/SDA(4)/0(5)/2(6)/GND(7)/5V(8) upward from there lands pin 7 exactly on "GND," matching the netlist-confirmed P1.7→GND fact and validating the method. P2's square pad sits at the physical **top**, next to "3V3" — the two headers are mirror-mounted, not simply symmetric, so P2's pin count runs the opposite direction from P1's. Counting 3V3(1)/15(2)/13(3)/12(4)/**14(5)**/16(6)/A0(7)/RST(8) downward from P2's pin 1 lands pin 5 exactly on **"14"**.

**Result: P2 pin 5 (the DIM gate-resistor net) is GPIO14 (D5)** — confirming the doc's original long-carried assumption. **This corrects the 2026-08-28 PCB engineering audit's Finding #2**, whose positional-symmetry argument assumed both headers shared the same pin-1 end and concluded GPIO12(D6) — that assumption didn't hold once the actual mirrored mounting was checked against the designer's own labels. See the audit report's 2026-09-02 addendum for the correction and the full pad-shape evidence. No physical continuity check is required before firmware — this is now source-verified, not inferred.

## Wiring

| Signal | Header pin | Gate resistor | MOSFET | Gate pulldown | Filter resistor | Filter cap | Connector |
|---|---|---|---|---|---|---|---|
| DIM (**GPIO14/D5, confirmed** — see MCU section above) | P2 pin 5 | R3 (100Ω) | M1 (**AO3400A**, LCSC C20917) | R1 (10kΩ) | R2 (100Ω, LCSC C25076) | C1 (10µF, LCSC C112497) | S1 (KF128 2-pin screw terminal, LCSC C474920) |

Path: P2 pin 5 → R3 → M1 gate (R1 pulldown to GND) → M1 drain → R2 → node shared with C1 (to GND), the D1/D2 protection diodes (below), and S1 pin 1 (DIM+); S1 pin 2 = GND/DIM-.

**Wiring — RJ11 daisy-chain:** S1's screw terminal feeds unit 1's RJ11 IN jack (pin 2 = DIM+, pin 3 = DIM-/GND); unit 1 OUT/THRU → unit 2 IN → unit 2 OUT → unit 3 IN → unit 3 OUT → unit 4 IN. All 4 fixtures share the same DIM+/DIM- bus and dim together — one GPIO drives all 4, no independent control.

## MOSFET: AO3400A

**M1 = AO3400A (AOS, LCSC C20917, SOT-23-3 SMD)** — a JLCPCB **Basic** part, replacing the board's earlier 2N7000 (TO-92). Confirmed unchanged across the 2026-08-21 and 2026-08-30 exports. 30V/5.8A, RDS(on) 28mΩ@10V, Vgs(th) 1.4V — comfortably driven by a 3.3V GPIO through the existing 100Ω gate resistor.

## Protection diodes on DIM+

Two components clamp the DIM+ node (the same net as C1, R2, and S1 pin 1) to GND:
- **D1 — SMAJ12CA-13-F** (Diodes Inc., LCSC C134948, SMA package): bidirectional TVS diode, 12V standoff. Confirmed via netlist (D1 pin 1 on the DIM+ net, pin 2 to GND) and schematic (drawn as a TVS symbol directly across the DIM+/GND node).
- **D2 — BAT54S KL4** (CJ/Jiangsu Changjing, LCSC C8592, SOT-23-3): dual small-signal Schottky diode pair, series-connected (confirmed via the 2026-08-30 BOM's LCSC part description: "Diode Configuration: 1 Pair Series Connection"), both cathodes tied to DIM+, common node to GND. Confirmed via netlist (D2 pins 2/3 on the DIM+ net, pin 1 to GND). **Corrected 2026-10-01: the netlist doesn't support "both cathodes on DIM+".** On a BAT54S, pin 1 = anode of diode A, pin 2 = cathode of diode B, pin 3 = common (A's cathode and B's anode). With pin 1 on GND and pins 2/3 on DIM+, diode A clamps DIM+ against going negative, and diode B has both ends on DIM+, so it does nothing. That's one active diode, as the 2026-09-18 review also says. It's harmless, just half the part unused.

Together these add transient/ESD protection to the DIM+ line exiting the board via the RJ11 daisy-chain — a sensible addition given that bus runs off-board to 4 external fixtures, similar in spirit to board 05's per-channel TVS diodes (D3/D4), though board 06 doesn't have an equivalent series fuse. Both parts are JLCPCB **Extended** and neither has been run through the Basic/cheaper-Extended swap search done for the rest of the design.

## Dimming
The SE7000 takes a 0–10V PWM dimming input; the board's open-drain MOSFET pulldown generates PWM, low-pass filtered by R2+C1 (100Ω/10µF, corner ≈159Hz) into a quasi-DC 0–10V-equivalent voltage. **Firmware:** the MOSFET only pulls DIM+ toward 0V (the driver sources/pulls the line high internally), so duty must be inverted (`duty = 100 − desired_brightness`). Run ESPHome PWM at ≥1kHz. **Open-circuit behavior:** if DIM+/DIM- are disconnected, the SE7000's internal pull-up defaults to 100% brightness, not off — relevant if the D1 mini loses power or the connector comes loose.

With 4 drivers ganged on one channel, their pull-up currents sum on the shared bus; R2 at 100Ω keeps the resulting floor voltage under roughly 8% worst-case (VERIFIED against Mean Well's own datasheet in the 2026-08-28 audit). Exact floor depends on the SE7000's actual source current, not yet measured — confirm with a multimeter at 100% MOSFET duty once wired up.

**Suspected dimming-curve problem — added 2026-10-01 [I], bench-check before trusting.** The paragraph above treats R2/C1 as a PWM averaging filter, which assumes DIM+ is pulled up by a *voltage* source. It isn't. Mean Well AB-type drivers *source* ~100 µA each (this doc's own floor reasoning uses that figure), so 4 drivers give ~0.4 mA into C1 = 10 µF:
- **Charging:** the line rises only ~40 mV per 1 ms PWM period while M1 is off.
- **Discharging:** each on-pulse drains C1 through R2 with τ = R2·C1 = 1 ms.
- **Result:** the steady-state level is roughly **V ≈ I·R2 / duty = 0.04 V / duty**.

| MOSFET duty | Brightness commanded | Expected DIM+ |
|---|---|---|
| 0.4 % | ~99.6 % | ~10 V (full) |
| 1 % | 99 % | ~4 V |
| 10 % | 90 % | ~0.4 V |
| 50 % | 50 % | ~0.04 V (floor) |

In other words, almost every setting below ~99 % would sit at the driver's minimum.
- **Test:** command 50 % and meter DIM+ to DIM−. About 5 V means the averaging model holds; about 0.05 V means this finding holds.
- **Fix if confirmed:** leave **C1 unfitted**. Mean Well 3-in-1 inputs accept 10 V PWM (100 Hz–3 kHz) directly, and the open-drain MOSFET plus the drivers' own pull-up then form a clean PWM signal. The firmware (1 kHz, inverted) works unchanged. D1's capacitance (SMAJ12CA, ~nF) still slows the rising edge to tens of µs at 0.4 mA, which is acceptable at 1 kHz.
- **Firmware-only alternative:** run the PWM at ~100 Hz and map brightness to on-time with V ≈ I·R2·T/t_on. This depends on the drivers' real source current, so it needs measuring; removing C1 is simpler.

**Discharge safety:** worst case the 10µF cap is charged to ~10V; MOSFET turn-on discharges it through R2 at a peak of 100mA (10V/100Ω), a brief 1ms transient — comfortably within AO3400A's 5.8A continuous rating.

## Firmware (ESPHome YAML)

**Committed 2026-10-01** as `garden-automation/esphome/packages/06_light_controller.yaml`, replacing the draft that used to live here. The config validates; it hasn't been flashed. Differences from the old draft:
- **`board: d1_mini`** (4 MB) instead of `d1_mini_pro`. It runs on both, and the D1 mini V4 (USB-C) photographed for the project is the 4 MB part.
- **`restore_mode: RESTORE_DEFAULT_ON`** instead of `RESTORE_DEFAULT_OFF`. Undriven DIM+ already means ~100 % (assumed, unconfirmed), so defaulting to on avoids a dim-then-bright flash at boot. Lights going fully off is the Kasa plugs' job, and the light-schedule blueprint drives both.
- **`min_power: 0.10`** plus `zero_means_zero: true`, to keep commands out of Mean Well's undefined region below ~8 %.
- **`gamma_correct: 1.0`**, so brightness % maps linearly to DIM+ duty.

The 1 kHz and `inverted: true` settings from the draft are unchanged. **Read the C1 finding in the Dimming section before trusting any brightness value.**

## Power
No dedicated regulator or power connector — powered via the D1 mini's own onboard USB port (USB-C on the V4.0.0 photographed 2026-10-01; earlier text said micro-USB, review conflict C10). Deployment needs a USB wall adapter at the light driver's install location (this board can't tap power from the SE7000s, unlike board 05's fans).

## Board

**Board outline: 60.0mm × 35.0mm** (confirmed directly from `Gerber_BoardOutlineLayer.GKO` in the 2026-08-30 export — 3mm corner radius). This is a **new, smaller outline** — the 2026-08-21 export was 70.0×37.0mm; this revision shrank the board by ~14% in area and relaid out every component. Mounting-hole span (SCREW1–4) is now **~51.2mm × 26.1mm** (down from ~61.1×28.2mm) — the entire component layout shifted to fit: the P1/U2/P2 MCU-socket group rotated 180° as a unit and now hugs the board's left edge, while S1/C1/R1/R2/R3/M1/D1/D2 all shifted in X only (Y position and rotation unchanged) to fit the narrower board. Schematic and netlist are unchanged from 2026-08-21 — this was a layout-only revision, no circuit change.

**P1 edge clearance (new, 2026-08-30 layout) — checked and cleared.** P1's header body sits close to the board's left edge: copper pad edge to board edge is 0.80mm (well clear of JLCPCB's ~0.3mm minimum), and the header's drawn body outline is 0.455mm from the edge. Confirmed against BOOMELE's own C27438 datasheet (user-supplied PDF, 2026-09-02): housing width is exactly 2.4mm, matching this board's footprint exactly — even at the datasheet's worst-case tolerance (±0.20mm), the housing stays ≥0.354mm clear of the edge. No overhang, no design change needed. See the audit report's 2026-09-02 addenda for the full measurement.

Single channel only — S2–S4, M2–M4, and their filter caps/resistors are not present on this board.

**MCU not placed by JLCPCB** — U1/U2's bare-outline footprint and the 4 mounting screws show as unselected in the PCBA order; the actual GPIO/power interface (P1, P2) is a real placed, Extended BOM line — see the MCU-socket note above. BOM has 10 placed lines total: C1, D1, D2, M1, P1, P2, R1, R2, R3, S1.

## Outstanding
- **Bench-test the dimming curve (blocking, added 2026-10-01).** Command 50 % and meter DIM+. If it reads ~0.05 V rather than ~5 V, leave C1 unfitted (see Dimming). This sits on top of the original CRITICAL firmware item from the 2026-08-28 audit. The firmware now exists and is committed (see Firmware) but hasn't been flashed. That item stays open until PWM polarity and the floor voltage are measured on real hardware.
- ~~MCU-socket architecture~~ — **DECIDED 2026-09-02: JLCPCB assembles P1/P2** (Extended headers), same as boards 02/04/05. No separate header sourcing/soldering needed for this board's MCU socket. See `System_Overview.md`. C27438 plating already confirmed Gold via jlcpcb.com/partdetail.
- D1 (SMAJ12CA-13-F) and D2 (BAT54S KL4) — both new, both Extended, neither run through the Basic/cheaper-Extended swap search yet.
- Measure the actual DIM+ voltage at 100% MOSFET duty cycle once all 4 lights are wired, to confirm the real floor voltage.
- RJ11 pinout (DIM+ = pin 2, DIM- = pin 3) is not physically confirmed — open since the original 2026-08-14 spec, never resolved.
- Board not currently in the JLCPCB cart (cart was cleared) — and the board-05/06 PCBA pricing in `System_Overview.md`'s ordering table predates this session's M1/D1/D2/P1/P2 changes and the 2026-08-30 layout revision, needs re-running before ordering.
