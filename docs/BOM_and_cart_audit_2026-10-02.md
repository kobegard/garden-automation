# BOM and JLCPCB cart audit — 2026-10-02

**Inputs:** the JLCPCB cart PDF (saved 2026-08-31), current BOM exports for boards 01 (09-19), 02 (08-30), 04 (08-30), 05 (09-18), 06 (08-30) and WH52 (PCB1 09-09), and the netlists.
**Limit:** jlcpcb.com and lcsc.com are blocked from the audit environment. Prices aren't re-quoted, and tier (Basic/Extended) comes from the BOM exports themselves. A swap is marked **verified** only when the replacement already appears as a *Basic Part* in one of your own BOM exports. Others are marked **verify**.

> `PCB_Assembly_Cost_Analysis.pdf` (09-09) is a Gemini product/business chat (retail tiers, Pi 4 HMI, cloud subscription), not a parts costing. It isn't used here.

## 1. The cart is stale for two boards (blocking)

| Cart item (2026-08-31) | Qty | Built from | Current design | Verdict |
|---|---|---|---|---|
| 01_reservoir_sensor_hub | 5 | `…_202608…` export | **09-19** | **Re-upload.** The cart predates the FeatherS3[D] socket layout's 09-19 state and the ADS1115 VDD → 3.3 V fix (review F5). |
| 02_dosing_controller_board_1 | 10 | 08-30 | 08-30 | OK (see swap B02-1) |
| 04_environment_sensor | 5 | 08-30 | 08-30 | OK |
| 05_fan_controller | 5 | 08-30 | **09-18** | **Re-upload. Critical:** the cart version has the reversed power-path diodes (review F1); boards built from it can't power up. |
| 06_light_controller | 5 | 08-30 | 08-30 | OK, but hold for the C1 bench test (swap B06-1) |
| WH52 PCB1 | — | not in cart | 09-09 | Respin pending the bench test |

Cart total at the time: $246.05 merchandise + $44.53 shipping (prototype quantities). The 10-room production figures in `System_Overview.md` ($388.64) predate all of these redesigns.

**Re-ordering reminders** (from the System Overview): blue mask (needed for Economic PCBA), LeadFree HASL, re-pick the customs category per board, and re-select shipping at checkout, because editing an order resets it.

## 2. Extended-part count per board (from the BOM exports)

JLCPCB adds a one-off loading fee for each unique Extended part on every PCBA order, typically about $3 each on Economic. Check the exact figure in the cart. Each line removed saves that fee on every order.

| Board | Lines | Extended | Of which unavoidable connectors/sockets |
|---|---|---|---|
| 01 | 22 | 11 | 7 (H1, H2, H4, H5, H6, H7, S1/S2) |
| 02 | 18 | 9 | 3 (H1, P1/P2, S1–S5) |
| 04 | 15 | 7 | 3 (CN1, P1/P2, SK1/SK3) |
| 05 | 13 | 7 | 3 (H1, P1/P2, S1/S2) |
| 06 | 10 | 5 | 2 (P1/P2, S1) |
| WH52 | 20 | 11 | 2 (BT1, USBC1) |

No line is missing an LCSC number except mounting holes, MCU footprints (hand-fitted modules) and the WH52 electrode pads, all of which are expected.

## 3. Swaps

| ID | Board | Change | Saves | Status |
|---|---|---|---|---|
| B05-1 | 05 | M1/M2 **AO3400 KEXIN C382311 (Extended) → AO3400A AOS C20917 (Basic)**. Same SOT-23 pinout; AOS is the original part. | 1 Extended line | **Verified:** C20917 is Basic in your 02 and 06 BOMs. First recommended 2026-08-25 and still not applied in the 09-18 BOM. |
| B02-1 | 02 | C6 **100uF16V6.3x11KM C49304900 → LKMC0901V101MF C442865** (same as C1–C5), and change C6's footprint from `CAP-TH_BD6.3-P2.50-D0.5` to C1's `…-D1.0`. Same 6.3 mm body and 2.5 mm pitch; only the drill differs. | 1 Extended line | Part already on this board. Small layout edit. |
| B06-1 | 06 | **C1 C112497 → do not fit**, *if* the bench test confirms the dimming finding (DIM+ ≈ 0.05 V at 50 %). | 1 Extended line + fixes dimming | Waits on the bench test |
| W-1 | WH52 | R1/R6/R11 10 kΩ **RC0603FR-0710KL C98220 (Extended) → 0603WAF1002T5E C25804 (Basic)** | 1 | **Verified:** Basic in your 01 BOM, same 0603 package |
| W-2 | WH52 | R7/R8 1 kΩ **FRC0603F1001TS C2907002 (Extended) → UNI-ROYAL 0603WAF1001T5E (commonly C21190, Basic)** | 1 | **Verify** the LCSC number and tier |
| W-3 | WH52 | C2/C5 22 µF 0603 **CL10A226MO7JZNC C2762594 (Extended) →** a Basic 22 µF 0603 ≥ 6.3 V (e.g. Samsung CL10A226MQ8NRNC, commonly C59461) | 1 | **Verify.** The only Basic 22 µF in your BOMs (C12891) is 1206, which is a different footprint. |

Fold W-1 to W-3 into the WH52 respin, which already has to change D2, the IO4/IO5 roles, the R8 return, the battery sense, USB ESD (USBLC6-2SC6 C21713975, already used on Board 04) and BT1 protection. Note that the respin *adds* some Extended lines.

## 4. Consistency notes (no action required)

- **Screw terminals:** boards 01, 02 and 05 moved to JXTCONN C254128V (2P C49291872 / 3P C49291873). Board 06 still uses Kefa KF128 C474920. Both are tin-plated and either works; aligning them is optional.
- **Board 05 H1** is now DS1023-1x5SF11 (C7509517). The System Overview connector table still lists ZHOURI C29779968; correct it when that table is next touched.
- **Board 02 fuses** match the review: F1–F4 BSMD1206-100 (1 A / 1.8 A), F5 750 mA, F6 2 A.
- **Board 01:** H5/H6 Feather sockets (C18078134, C350305) are Extended and their plating is unconfirmed. Check on partdetail before ordering, as was done for C27438.

## 5. Order checklist

1. Re-upload Board 05 from the **09-18** export. Ideally apply B05-1 first.
2. Re-upload Board 01 from the **09-19** export.
3. Board 02: apply B02-1 (optional) and re-export.
4. Board 06: run the C1 bench test on one prototype before ordering more; apply B06-1 if confirmed.
5. Re-quote and update the System Overview pricing table.
