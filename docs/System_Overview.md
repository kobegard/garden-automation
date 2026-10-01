# Garden Automation Mark 2 — System Overview

> **Corrections pass 2026-10-01** (marked "corrected 2026-10-01" inline). Sources: the 2026-09-18 full-system review, the 09-19 Board 01 / 09-18 Board 05 CAD exports, and the firmware now in `kobegard/garden-automation`. The JLCPCB pricing sections are **not** re-run here; they still predate the redesigns, as their own 08-25 note says.

Five board designs — board 03 was consolidated into board 02. Each board's own `.md` file in this folder has full wiring detail; this is the map between them.

**Deployment scale: 10 rooms.** Each room gets its own complete set: 1× board 01, 2× board 02 (for 8 pump channels), 1× board 04, 1× board 05, 1× board 06 — all boards independent per room, each running its own ESPHome config. Total units: 10× board 01, 20× board 02, 10× board 04, 10× board 05, 10× board 06.

## The five boards

1. **01_reservoir_sensor_hub** — **Unexpected Maker FeatherS3[D] (ESP32-S3)** (corrected 2026-10-01; was "ESP32", the abandoned DevKitC design). DS18B20 water temp, A02YYUW ultrasonic water level, 2× flood probes, and a local **DFR0997 2" IPS display on the Feather's STEMMA QT port** (corrected 2026-10-01: there is no H3 header; the display isn't on this PCB). pH/EC sensing: hybrid design — DFRobot Gravity analog pH (SEN0169-V2 industrial probe + DFR0504 isolator + DFR0553/ADS1115 16-bit I2C ADC) and Atlas Scientific isolated EZO EC circuit (M-EC-KIT-1.0). See `01_reservoir_sensor_hub.md` for full wiring, power, and connector detail.
2. **02_dosing_controller** — ESP8266 (D1 mini / D1 Mini Pro, same pinout). 4 MOSFET-driven pump channels, forward-only. **Pump GPIOs: 1 = GPIO5, 2 = GPIO13, 3 = GPIO14, 4 = GPIO4** (Pump 3 corrected 2026-09-18, was GPIO12). 2 physical units per room (Pumps 1-4 / Pumps 5-8) for 8 total pump channels — 20 units total.
3. **04_environment_sensor** — ESP8266 (D1 mini / D1 Mini Pro). BME280 + **MLX90632** (corrected 2026-10-01; was MLX90614. The MLX90632 is chained off the AS7343 breakout by STEMMA QT, I2C 0x3A) + SCD41 + **AS7343** (spectral/PAR — corrected 2026-08-25; was previously AS7341) on one shared I2C bus: BME280/MLX90614/SCD41 each get their own 4-pin elevated header (**SK1–SK3**, not SK1–SK4 — corrected 2026-08-25, the design now has only 3 of these headers), and AS7343 connects via a separate 4-pin JST-SH Qwiic connector (CN1) with its own ESD-protection diode (D1). Three distinct header pin orders across SK1–SK3/CN1, each matched to its sensor's native breakout — see board doc.
4. **05_fan_controller** — ESP8266 (D1 mini / D1 Mini Pro). 2 MOSFET-switched PWM channels, one per AC Infinity Cloudline fan: Fan 1 = GPIO12, Fan 2 = GPIO14. **The 09-02 layout had all four power-path diodes reversed (review F1, CRITICAL); the 09-18 netlist has them flipped** (corrected 2026-10-01). Diode-test D3/D4 on the first assembled board.
5. **06_light_controller** — ESP8266 (D1 mini / D1 Mini Pro). Single MOSFET-switched channel generating a PWM dimming signal for all 4 SE7000 LED drivers, daisy-chained and dimmed as one group off GPIO14 (D5).

## Power sourcing
- **01_reservoir_sensor_hub**: USB-C into the FeatherS3[D] (SHNITPWR 5 V/3 A via an IP67 feedthrough). Two board rails (corrected 2026-10-01): `+5V` = Feather 5V pin through F1 (750 mA PTC) → H1 (DFR0504) and LED1; `VCC` = Feather LDO2 3.3 V, switched by IO39 → H2, H4, H7, the ADS1115 (moved off +5V in the 09-19 netlist, review F5) and all pull-ups. Each sensor chain isolates/regenerates its own downstream power (DFR0504 for pH, Atlas carrier's internal isolated DC/DC for EC).
- **02_dosing_controller**: external 12V, one dedicated adapter per physical unit. **Conflict (review C4):** this doc said 12V/2A, but the board doc and the product listing say ALITOVE 12V/3A. Either covers the ~1.15 A worst case.
- **04_environment_sensor**: USB, via the D1 Mini Pro's own onboard port.
- **05_fan_controller**: no external supply — logic power is diode-OR'd off the two fans' own +10–12V UIS lines into a DROK buck (fixed 5 V). If both fans lose power, this board goes with them.
- **06_light_controller**: USB, via the D1 mini's own onboard port (USB-C on the V4.0.0).

Net, per room: boards 01, 04, and 06 each need a USB wall adapter or hub; board 02 needs a dedicated 12V/2A adapter per physical unit (2 per room); board 05 needs nothing beyond the fans it's wired to. Across all 10 rooms: 30 USB adapters/hubs and 20 12V/2A adapters.

## Manufacturing package
This folder (`test/`) contains, per board: Gerbers (`Gerber_*.zip`), BOM (`BOM_*.xlsx`), pick-and-place (`PickAndPlace_*.xlsx`), schematic PDF (`SCH_*.pdf`), and netlist (`Netlist_*.tel`). All five BOMs are electrically clean — the only entries missing an LCSC supplier part number are mounting-hole screws. All boards' PCB layouts include ground copper pour under solder mask.

**Board sizes (mounting-hole span):**

| Board | Size | Notes |
|---|---|---|
| 01_reservoir_sensor_hub | **100.0×100.0mm** outline (corrected 2026-10-01 from the Gerber; ~105.3×83.2 was the old DevKitC layout) | 4 on-board pH/EC modules (SEN0169-V2, DFR0504, DFR0553, Atlas Isolated EZO Carrier) in a 2×2 grid, plus the display header (H3) on the same right-edge column as H1/H4; the ultrasonic header (H2) sits separately near the top-left corner |
| 02_dosing_controller | **100.0×50.0mm** outline (~92×42 is the mounting-hole span) | 4 pump connectors, 4 flyback diodes, 5-fuse array, buck converter |
| 04_environment_sensor | 50×50mm | Corrected 2026-08-25 (was ~62×62mm) — confirmed exact from the board-outline Gerber layer. 3 sensor headers (SK1–SK3) + 1 JST-SH Qwiic connector (CN1) for the 4th sensor, 44×44mm mounting-hole pitch |
| 05_fan_controller | **55.0×35.0mm** outline (corrected 2026-10-01 from the Gerber) | |
| 06_light_controller | **60.0×35.0mm** outline (corrected 2026-10-01 from the Gerber) | Single channel |

## JLCPCB ordering
**Cart rebuilt 2026-08-20, all 5 boards, quantities matching deployment scale (qty 20 board 02, qty 10 boards 01/04/05/06).** Blue solder mask, LeadFree HASL, all five boards on **Economic PCBA** (confirmed by the cart's own line-item labels, not assumed). Current pricing (from a live checkout PDF, shipping to Brooklyn NY):

| Board | Qty | PCB | PCBA (Economic) | Board subtotal |
|---|---|---|---|---|
| 01_reservoir_sensor_hub | 10 | $15.50 | $30.81 | $46.31 |
| 02_dosing_controller | 20 | $15.10 | $74.13 | $89.23 |
| 04_environment_sensor | 10 | $6.30 | $22.26 | $28.56 |
| 05_fan_controller | 10 | $6.20 | $31.09 | $37.29 |
| 06_light_controller | 10 | $6.20 | $19.49 | $25.69 |
| **Total** | | | | **$227.08** |

**Final checkout total (2026-08-20, all 11 BOM swaps applied + customs category corrected): Merchandise $227.08 + Shipping $82.03 (UPS Express Saver DDP) + Customs duties & taxes $79.53 = Grand Total $388.64**, for the full 60-board fleet across 10 rooms. Board 05/06 PCBA dropped further ($46.61→$31.09, $25.73→$19.49) once their pending Basic/cheaper-Extended swaps were applied — see `JLCPCB_Assembled_Components.md` for the full swap list, all now confirmed applied in the live BOM. Customs duty dropped $87.14→$79.53 alongside a per-board category correction (cart no longer shows blanket "Circuit Control" for every board) — the category picked for each board hasn't been independently re-screenshotted against the table below, worth a quick glance in Edit Order before paying if you want to confirm each board matches.

**Note, 2026-08-25: this whole total, and every board's row in the pricing table above, predates the board 01/02/04/05/06 redesigns found in this week's reviews and needs re-running once all five boards' BOMs/docs are fully current** — see the Outstanding section below for what changed on each board.

Running total from the color fix alone (previous entry) plus this BOM/customs pass: **$621.66 → $418.40 → $388.64**, a cumulative $233.02 (37%) off the original white-solder-mask, mixed-tier, unswapped cart.

**Why this matters — solder mask color gates Economic PCBA eligibility, separately from the 70×70mm Standard-tier size floor:** an earlier cart attempt used white solder mask/PCB color and JLCPCB's order form wouldn't offer Economic PCBA as a selectable tier at all — confirmed live by the user in JLCPCB's own UI, and consistent with JLCPCB's capabilities page, which lists Economic's "Surface Finish & Color" as "limited by specific options" (vs. "no limit" for Standard). Switching PCB color from white to **blue** made Economic selectable on every board, including board 01 (~105.3×83.2mm, large enough that it could technically use Standard instead — Economic was chosen anyway since it's cheaper). That one color change dropped the grand total from $621.66 (white, mixed Standard/Economic) to $418.40 (blue, all-Economic) — a $203.26 / ~33% reduction on the PCB+PCBA line items alone, no BOM or spec changes involved. No functional reason was in play for white (not an LED-reflectivity call on board 06 or similar) — it was just JLCPCB's default, unset explicitly.

**Customs/HS-code classification, per board (what applies once the cart is rebuilt):** boards ship without their MCU module populated, so classification follows what's actually mounted at ship time (GRI 2(a)).
- **Board 02** — "Elevator Controller," HS 8537.10, 2.7% base duty (6 mounted SMD fuses are a direct GRI-1 match for boards equipped with heading-8535/8536 apparatus).
- **Boards 05 & 06** — "Circuit Control," HS 8538.90, 3.5% base duty (no fuses/relays, discrete MOSFETs only; closest fit on JLCPCB's picklist).
- **Boards 01 & 04** — "Sensor\Controller\Precision Instrument → Others" (no HS code on this sub-item), with "Sensor hub" typed into the PCBA Remark free-text field — neither board has switching apparatus, and the sensors themselves are off-board modules.

All of the above fall under China Section 301 List 3, which adds a flat 25% on top of whatever base rate applies.

**JLCPCB UI quirks to expect when re-ordering:** the checkout page's shipping figure has previously differed from the cart page's selected rate by several dollars — reselect the shipping method at checkout, don't trust the cart page. Editing any board's PCBA order (BOM, category, remark) has previously reset the shipping method back to the default DHL Express — reselect the intended method immediately before paying.

Cart quantities should match the deployment scale: qty 20 for board 02 (2 units × 10 rooms), qty 10 for boards 01/04/05/06 (1 unit × 10 rooms).

## MCU sourcing and socket-mount

**DECIDED 2026-09-02 — JLCPCB assembles P1/P2 on boards 02, 04, 05, and 06.** This section's original premise (every MCU footprint is a real through-hole pattern for the module's own pins, hand-populated with user-sourced headers, nothing JLCPCB-placed) applies only to **board 01** now. On the four D1-Mini-Pro boards, U1's own footprint is a bare mechanical outline with zero pins (`WEMOS_D1_MINI_footprint`) — the D1 Mini Pro module itself is still hand-populated (nothing JLCPCB stocks a complete devkit module), but it plugs into two 1×8 female headers, **P1 and P2 (BOOMELE, LCSC C27438, Gold-plated — confirmed directly on jlcpcb.com/partdetail 2026-08-25)**, which are real placed, Extended BOM lines that **JLCPCB assembles as part of the PCBA order.** No hand-sourcing, no separate header purchase, no DIY soldering for these four boards' MCU sockets. This closes out the open recommendation carried in this doc since 2026-08-25.

**MCU modules themselves are still not placed by JLCPCB on any of the five boards** — checked directly in each board's PCBA "Unselected Parts" list, not just the raw BOM export. Every board ships needing its MCU module hand-populated: 10 ESP32-DevKitC-VE units for board 01 (plugged into hand-soldered headers, see below), and 50 Wemos D1 Mini Pro units total across boards 02 (×20), 04/05/06 (×10 each) (plugged into the JLCPCB-assembled P1/P2 sockets — no soldering needed for these).

**Board 01 — superseded 2026-10-01.** The DevKitC footprint described here is gone. The 09-19 board carries a FeatherS3[D] in two female headers, **H5 (12-pin, C18078134) and H6 (16-pin, C350305), both real BOM lines that JLCPCB places**. No hand-sourced or hand-soldered headers are needed for Board 01 either; only the Feather module is hand-fitted. Header plating is unconfirmed. Original text kept for history: "MCU footprint is still a through-hole pin pattern matching the module's own pins (`BULETM-SMD_ESP32-DEVKITC`, 2×19) … Source 2× 1×19 gold-plated low-profile female header strips per unit … and hand-solder them, then plug the ESP32-DevKitC-VE module in." Reseat/inspect periodically — sockets add exposed metal-to-metal contact points a soldered joint wouldn't have, and add a few mm of height versus direct-solder (check enclosure/lid clearance).

A related, separate item: with U1 unconnected in the schematic and P1/P2 drawn as anonymous numbered pins on boards 02/04/05/06, **GPIO pin identity is not verifiable from CAD alone** — resolved per-board via each board's own PCB documentation-layer pinout drawing (board 02's and board 06's GPIO mappings are confirmed this way; see each board's `.md` and, for board 06, `claude/board-06-pcb-engineering-audit-2026-08-28.md`'s 2026-09-02 addendum). Boards 04/05 haven't had this specific silkscreen check done yet — still carried as open in their own docs.

| Board | MCU | Header footprint | Sockets per unit | Units | Sourcing |
|---|---|---|---|---|---|
| 01 | FeatherS3[D] (corrected 2026-10-01) | H5 12-pin + H6 16-pin | 1× 12 + 1× 16 | 10 | **JLCPCB-assembled** (C18078134 / C350305) |
| 02 | D1 Mini Pro | P1/P2 (C27438) | 2× 1×8 | 20 | **JLCPCB-assembled (Extended)** |
| 04 | D1 Mini Pro | P1/P2 (C27438) | 2× 1×8 | 10 | **JLCPCB-assembled (Extended)** |
| 05 | D1 Mini Pro | P1/P2 (C27438) | 2× 1×8 | 10 | **JLCPCB-assembled (Extended)** |
| 06 | D1 Mini Pro | P1/P2 (C27438) | 2× 1×8 | 10 | **JLCPCB-assembled (Extended)** |

Only board 01's 20× 1×19 headers need separate sourcing/hand-soldering (~$4–5 total). Boards 02/04/05/06's 100× 1×8 sockets are covered by the PCBA order itself — no DIY step, no separate purchase.

## Connector plating

| LCSC part | Used as | Boards/designators | Plating | JLCPCB Status |
|---|---|---|---|---|
| C42431787 (PZ2.54-1X4P-H25, JXTCONN) | 4-pin male header | Board 01 H1, H2, H3 | Gold | Extended |
| C225480 (A2541WV-5P, CJT) | 5-pin male header | Board 01 H4; board 02 H1 | Gold | Extended |
| C22373890 (HC-PM254-8.5H-1x4PZ-02A, Hong Cheng) | 4-pin female header | Board 04 SK1–SK3 (corrected 2026-08-25 — was SK1–SK4; the design now only has 3 of these headers) | Gold | Extended |
| C27438 (2.54-1×8P母环保, BOOMELE) | 1×8 female header (MCU socket) | Board 02 P1/P2; board 04 P1/P2; board 05 P1/P2; board 06 P1/P2 (added 2026-08-25 — board 06 confirmed same architecture, see board-06 review) | **Gold — confirmed 2026-08-25 directly via jlcpcb.com/partdetail** (was "Not yet confirmed" in the prior pass of this table; see the resolution note in "MCU sourcing and socket-mount" above) | Extended |
| C2905006 (1.0-4PWB, DEALON) | 4-pin JST-SH connector | Board 04 CN1 (added 2026-08-25) | Not yet confirmed | Extended |
| C21713975 (USBLC6-2SC6, GOODWORK) | ESD/TVS array, not a connector but listed for completeness | Board 04 D1 (added 2026-08-25) | N/A | Extended |
| C29779968 (PZ2.54-1x5-11.2, ZHOURI) | 5-pin male header | Board 05 H1 (buck converter connector) | Gold | Extended |
| C693601 (SMAJ12A, FTR) — **current BOM has SMAJ14A (review C3, corrected 2026-10-01)** | Unidirectional TVS diode, not a connector but listed for completeness | Board 05 D3/D4 (added 2026-08-25 — new, protects each fan's fused +10V line) | N/A | Extended |
| C46641026 (SMD1206-100-30, R+O) | Resettable PTC fuse, not a connector but listed for completeness | Board 05 F1/F2 (added 2026-08-25 — new, one per fan) | N/A | Extended |
| C134948 (SMAJ12CA-13-F, Diodes Inc.) | Bidirectional TVS diode, not a connector but listed for completeness | Board 06 D1 (added 2026-08-25 — new, clamps the DIM+ bus) | N/A | Extended |
| C8592 (BAT54S KL4, CJ) | Dual Schottky diode array, not a connector but listed for completeness | Board 06 D2 (added 2026-08-25 — new, clamps the DIM+ bus) | N/A | Extended |
| C474921 (KF128-2.54-3P, Kefa) | 3-pin screw terminal | Board 01 S1; board 05 S1/S2 | Tin | TBD |
| C474920 (KF128-2.54-2P, Kefa) | 2-pin screw terminal | Board 01 S2/S3; board 02 S1–S5; board 06 S1 | Tin | TBD |

**Note, 2026-08-25:** this table was already stale for board 01/02 from their own reviews (see the project's dated doc-review log, not reproduced here); board 04's SK1–SK4→SK1–SK3 correction and the new C27438/C2905006/C21713975 rows are from this session's board-04 review. Board 05's P1/P2 row and the new D3/D4/F1/F2 rows are from this session's board-05 review — board 05 also picked up an entirely new MOSFET family (M1/M2 now AO3400, not 2N7000) and a changed C1 value (10µF, not 22µF), neither of which is a connector so isn't listed in this table — see `JLCPCB_Assembled_Components.md` and the board-05 doc. Board 06's P1/P2 row and the new D1/D2 rows are from this session's board-06 review — board 06 also picked up the same MOSFET-family change as board 05 (M1 now AO3400A, not 2N7000, though unlike board 05 this one flips M1 from Extended to Basic) — see `JLCPCB_Assembled_Components.md` and the board-06 doc. All five boards are now current as of this pass.

All connector/header parts across the deployment are JLCPCB **Extended** parts — none are Basic, so each carries JLCPCB's per-line Extended-component assembly fee on every PCBA order (once per unique part, regardless of quantity). Basic/Extended status for the screw terminals and the rest of the passive/discrete BOM is still being confirmed (see `JLCPCB_Assembled_Components.md`).

The tin-plated KF128 screw terminals are staying as-is — no gold-plated equivalent exists in this footprint (screw terminals rely on clamping force, not repeated-mate contact, so tin is the standard finish industry-wide). If the pump-power terminals (board 02) need more corrosion resistance later, options include dielectric grease, periodic re-torque/inspection, conformal coating, or a sealed/IP-rated terminal block — not a connector swap.

## PCB surface finish
All five boards use HASL (not ENIG — ENIG's cost tracks gold spot price and runs meaningfully higher). **Lead-free HASL confirmed selected** in the rebuilt 2026-08-20 cart (shown on every board's PCB line item). For this project's corrosion concerns leaded and lead-free are roughly equivalent — the real resistance jump is HASL-family vs. ENIG. Solder mask/PCB color is **blue** (also confirmed in the rebuilt cart) — required to make Economic PCBA selectable at all; see the JLCPCB ordering section above.

Copper pour under solder mask isn't itself a corrosion risk (the mask protects it the same as a trace); the actual exposure is at unmasked pads/vias and, in principle, electrochemical migration between adjacent oppositely-biased exposed copper under humidity plus contamination — a connector/pad-level concern, not a pour concern. No specific clearance issue has been found on any of the five boards.

## Outstanding across the deployment
- Board 01: local display module (DFR0486 or equivalent) not yet sourced; ADS1115 ADDR switch and PGA gain not yet confirmed; Atlas EZO one-time UART→I2C switch needed per unit. (Note: board 01 has changed substantially since this line was written — see the project's dated doc-review log for the 2026-08-25 board-01 review; ADS1115 is now on-board with ADDR hardwired to GND, no physical switch exists anymore.)
- Board 06: DIM+ floor voltage not yet measured against real SE7000 hardware. **Added 2026-10-01:** suspected that C1 (10 µF) defeats PWM averaging because the drivers source only ~100 µA each; bench-check at 50 % before trusting any brightness (see `06_light_controller.md`).
- Board 04: SCD41 current draw vs. D1 mini regulator margin worth checking if brownouts appear (somewhat mitigated — SCD41 now runs off a separate 5V rail via SK3, not the shared 3.3V rail, but not independently re-verified).
- Board 04: GPIO pin identity (P1/P2 → GPIO5/GPIO4) not verifiable from CAD, needs physical continuity check; P1/P2 (C27438) plating **confirmed Gold 2026-08-25**, no longer open. MCU-socket architecture question above now has a recommendation (let JLCPCB assemble) rather than being fully open — user decision still needed to act on it. **(Superseded: decided 2026-09-02 — JLCPCB assembles P1/P2; see "MCU sourcing and socket-mount".)** Same open items apply to board 02. **AS7343 default I2C address (0x39) confirmed 2026-08-25 directly from ams OSRAM's own AS7343 datasheet** — no longer just a carried-forward assumption.
- Board 05: same P1/P2/GPIO-unverifiable open items as boards 02/04 (see board-05 review); P1/P2 plating now confirmed Gold, same as board 04. F1/F2 (C46641026, R+O SMD1206-100-30, 30V/1A hold/1.8A trip) and D3/D4 (C693601, FTR SMAJ12A, 12V standoff/19.9V clamp) **checked directly on jlcpcb.com/partdetail 2026-08-25 — both Extended, no Basic equivalent, specs match the board doc exactly.** **(Corrected 2026-10-01, review C3: the current 09-18 BOM has SMAJ14A for D3/D4, not SMAJ12A; re-check its LCSC part and tier before ordering.)** M1/M2 (C382311, KEXIN AO3400, Extended) **has a concrete recommended swap: C20917 (AOS AO3400A, Basic)** — same electrical class, already used and proven on boards 02/06, removes 2 Extended-fee triggers per unit, not yet applied. C1's value changed from the documented 22µF to 10µF (confirmed, C13585, Basic). The $31.09 board-05 PCBA price and "3 Extended parts" count in the ordering table above and in `JLCPCB_Assembled_Components.md` both predate these changes and need re-running before ordering.
- Board 06: same P1/P2/GPIO-unverifiable open items as boards 02/04/05 (see board-06 review); P1/P2 plating now confirmed Gold. A changed MOSFET family (M1 now AO3400A, not 2N7000 — net win, flips M1 from Extended to Basic) and two new protection diodes (D1 SMAJ12CA-13-F TVS, D2 BAT54S dual Schottky, both clamping the DIM+ bus, both confirmed Extended per the board-06 review) haven't been run through the Basic/cheaper-Extended swap search for D1/D2. The $19.49 board-06 PCBA price and "1 Extended part" count in the ordering table above and in `JLCPCB_Assembled_Components.md` both predate these changes and need re-running before ordering.
- **Superseded 2026-10-01 (review C3):** the 08-30 BOM has **F1–F4 = BSMD1206-100 (1 A hold / 1.8 A trip)** for the pump branches and F5 = 750 mA for the buck branch, not C976304 on all five. Pumps draw 0.25 A, so the open question is only stall current vs the 1.8 A trip. Original text: **New finding, 2026-08-25 overview pass — board 02's F1–F5 fuses changed rating, not just manufacturer.** The 08-20-tracked swap (C170165, 15V/500mA hold/1A trip) is no longer in the design; the current 08-22 BOM has **C976304** (BHFUSE BSMD1206-075-24V, 24V/750mA hold/1.5A trip) — a 50%-higher hold and trip current, not a like-for-like manufacturer substitution. Confirmed via JLCPCB partdetail (still Extended, same 1206 package). Not yet checked against actual per-pump stall/inrush current — worth confirming a dosing pump's real draw stays comfortably under 750mA before treating this as harmless, since a higher-hold fuse trips later and may have been sized differently on purpose originally. See `JLCPCB_Assembled_Components.md` for detail.
- ~~Board 01 only: MCU header sockets (2× 1×19 per unit) not yet purchased~~ — **closed 2026-10-01:** H5/H6 are JLCPCB-placed BOM lines on the 09-19 board. **Boards 02/04/05/06 need no separate socket purchase — decided 2026-09-02 that JLCPCB assembles P1/P2 as part of the PCBA order; see "MCU sourcing and socket-mount" above.**
- **`JLCPCB_Assembled_Components.md` was fully rewritten 2026-08-25** against today's BOM exports (previously built from the 08-20 exports and badly out of date — e.g. it was missing board 01's entire ESD/LED/U2/H5-H7 additions). See that file for the complete current per-board Extended/Basic table and the F1–F5 finding above.
- JLCPCB cart: customs/HS-code category still needs re-picking per board (currently showing generic "Circuit Control" for all, not the per-board categories in the table above) — doesn't persist through the earlier cart clear/rebuild; shipping method should be reselected at checkout per the UI-quirks note above; other DHL/shipping method options below DHL Express (DDP) haven't been compared for price yet.
- **Added 2026-10-01 — firmware and HA logic now exist** in `kobegard/garden-automation`: ESPHome configs for all five boards, custom drivers (AS7343, MLX90632, DFR0997) and six Home Assistant blueprints (pH/EC dosing, dosing interlock, fans, lights, CO2). Validated but not yet flashed. Board 01 additionally needs the H1 → DFR0504 cable re-pinned (see `01_reservoir_sensor_hub.md`).
- **Added 2026-10-01 — WH52:** bench test (`wh52/bench_test`) pending for review findings F3 (D2 moisture path), F4 (TH1 on ADC2), R8 loading and the IO0/IO2 over-voltage; respin list in that README.
