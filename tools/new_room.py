#!/usr/bin/env python3
"""Write the six ESPHome device files for a grow room.

    python3 tools/new_room.py 3            -> esphome/room03-*.yaml
    python3 tools/new_room.py 3 --force    overwrite existing files

Pump names default to "Pump N"; edit the generated dosing files once the
chemical assignments for pumps 3-8 are decided.
"""
import argparse
from pathlib import Path

ESPHOME = Path(__file__).resolve().parent.parent / "esphome"

PUMP_NAMES = {
    1: "Pump 1 pH Up",
    2: "Pump 2 pH Down",
}


def header(rr, slug, title):
    return (
        "substitutions:\n"
        f"  node_name: room{rr}-{slug}\n"
        f'  friendly_name: "Room {rr} {title}"\n'
        f'  area: "Room {rr}"\n'
    )


def packages(board):
    return (
        "\npackages:\n"
        "  base: !include common/base.yaml\n"
        + ("  mcu: !include common/d1_mini.yaml\n" if board != "01_reservoir_sensor_hub" else "")
        + f"  board: !include packages/{board}.yaml\n"
    )


def dosing(rr, unit, first):
    names = "".join(
        f'  p{i}_name: "{PUMP_NAMES.get(first + i - 1, f"Pump {first + i - 1}")}"\n'
        for i in range(1, 5)
    )
    return header(rr, f"dosing-{unit}", f"Dosing {unit.upper()}") + names + packages("02_dosing_controller")


def files(room):
    rr = f"{room:02d}"
    return {
        f"room{rr}-reservoir.yaml": header(rr, "reservoir", "Reservoir")
        + "  tank_empty_mm: \"600\"   # TODO measure: sensor face -> tank floor\n"
        + "  tank_full_mm: \"100\"    # TODO measure: sensor face -> full line\n"
        + packages("01_reservoir_sensor_hub"),
        f"room{rr}-dosing-a.yaml": dosing(rr, "a", 1),
        f"room{rr}-dosing-b.yaml": dosing(rr, "b", 5),
        f"room{rr}-environment.yaml": header(rr, "environment", "Environment")
        + packages("04_environment_sensor"),
        f"room{rr}-fan.yaml": header(rr, "fan", "Fans") + packages("05_fan_controller"),
        f"room{rr}-light.yaml": header(rr, "light", "Lights") + packages("06_light_controller"),
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("rooms", type=int, nargs="+")
    ap.add_argument("--force", action="store_true")
    args = ap.parse_args()
    for room in args.rooms:
        for name, body in files(room).items():
            path = ESPHOME / name
            if path.exists() and not args.force:
                print(f"skip  {path.name} (exists)")
                continue
            path.write_text(body)
            print(f"wrote {path.name}")


if __name__ == "__main__":
    main()
