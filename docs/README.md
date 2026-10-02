# Project docs

Corrected copies of the project docs, kept under version control next to the
firmware. Drop them onto the Mac in `Documents/garden automation/test/`,
replacing the old copies.

| File | Status |
|---|---|
| `System_Overview.md` | Corrections pass 2026-10-01: Board 01 MCU, rails and sockets; outlines from the Gerbers; Board 04 MLX90632; Board 05 F1 fixed; fuse ratings; firmware status. JLCPCB pricing **not** re-run. |
| `boards/01_reservoir_sensor_hub.md` | **Rewritten** from the 09-19 CAD. The old copy described the abandoned ESP32-DevKitC board. New: H1 cable re-pin and DS18B20 wiring findings. |
| `BOM_and_cart_audit_2026-10-02.md` | Cart vs current designs (Boards 01 and 05 must be re-uploaded), Extended-part counts, verified swaps. |
| `boards/06_light_controller.md` | Corrections 2026-10-01: suspected C1 dimming problem with a bench test, firmware now committed, D2 (BAT54S) description fixed. |

Every change is marked inline with "corrected/added 2026-10-01" and the old
value, so the correction history survives. Boards 02, 04 and 05 docs are
still the originals; their known corrections (Pump 3 = GPIO14, the Board 04
header list, the Board 05 outline and diodes) are already recorded in the
system review and the System Overview.
