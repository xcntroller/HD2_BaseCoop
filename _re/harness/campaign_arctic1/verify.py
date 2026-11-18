"""Check the logs from two actual games in one Arctic1 debug scenario."""
import argparse
from pathlib import Path
import re


def expect_once(log, pattern):
    matches = re.findall(pattern, log, re.MULTILINE)
    if len(matches) != 1:
        raise SystemExit(f"Expected one match for {pattern!r}, found {len(matches)}.")


def objective_states(log, prefix):
    return {
        int(index): int(status)
        for index, status in re.findall(rf"^{prefix} objective state (\d+) (\d+)$", log, re.MULTILINE)
    }


def verify(scenario, host, guest):
    success = int(scenario in ("wire-success", "native-objectives"))
    expect_once(host, rf"^host captured terminal mission outcome {success} 4$")
    expect_once(guest, rf"^guest applied terminal mission outcome {success} \d+$")
    original = objective_states(host, "host published")
    replica = objective_states(guest, "guest applied")
    # The guest only logs changes from the original inactive objective state.
    if any(original.get(index) != replica.get(index, 4) for index in original.keys() | replica.keys()):
        raise SystemExit(f"Objective states differ. Host {original}, guest {replica}.")
    expected = [1, 1, 1, 1] if success else [3, 2, 2]
    if [original.get(index) for index in range(1, len(expected) + 1)] != expected:
        raise SystemExit(f"Unexpected terminal objective states {original}.")

    if scenario == "wire-success":
        expect_once(host, r"^arctic1 test seeded success 0 0$")
        expect_once(host, r"^arctic1 test captured mission script value 1 3$")
        expect_once(host, r"^host accepted wire cut 99 0$")
        expect_once(host, r"^host completed wire cut 99 0$")
        expect_once(guest, r"^guest presented wire cut 99 0$")
        expect_once(host, r"^host captured terminal soldier 4 health 0$")
        expect_once(guest, r"^guest applied host squad health 3 0$")
        first_soldier = host.split("host captured terminal soldier 1 health ", 1)[-1]
        first_soldier = first_soldier.split("host captured terminal soldier 2 health ", 1)[0]
        expect_once(first_soldier, r"^  belt item 59 quantity 1$")
    elif scenario == "native-objectives":
        expect_once(host, r"^arctic1 test placed host near entrance with one guard alive 0 0$")
        expect_once(host, r"^arctic1 test observed entrance and outstanding guard 0 0$")
        expect_once(host, r"^arctic1 test observed garrison completion and gathering open 0 0$")
        expect_once(host, r"^arctic1 test brought remaining squad into native gathering range 0 0$")
        expect_once(host, r"^arctic1 test captured mission script value 1 4$")
        for soldier in range(1, 5):
            expect_once(host, rf"^host captured terminal soldier {soldier} health [1-9]\d*$")
        for log, prefix in ((host, "host published"), (guest, "guest applied")):
            expect_once(log, rf"^{prefix} objective state 6 1$")
            states = re.findall(rf"^{prefix} objective state ([234]) ([01])$", log, re.MULTILINE)
            if states != [("2", "0"), ("3", "0"), ("2", "1"), ("4", "0"), ("3", "1"), ("4", "1")]:
                raise SystemExit(f"{prefix}: unexpected entrance, garrison and gathering transitions {states}.")
    else:
        expect_once(host, r"^arctic1 test induced native guide death 0 0$")
        expect_once(host, r"^arctic1 test tried to complete failed guide 3 0$")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("scenario", choices=("wire-success", "native-objectives", "guide-failure"))
    parser.add_argument("host", type=Path)
    parser.add_argument("guest", type=Path)
    args = parser.parse_args()
    verify(args.scenario, args.host.read_text(errors="replace"), args.guest.read_text(errors="replace"))
    print(f"PASS {args.scenario} in two debug-assisted game instances")
