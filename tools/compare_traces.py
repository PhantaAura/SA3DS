#!/usr/bin/env python3
"""Compare SA3DS CSV traces without implying either trace is authoritative."""
import argparse
import csv
import math
import sys

INTEGER_FIELDS = {"trace_version", "frame", "jump_pressed", "jump_held", "jump_released",
                  "action_pressed", "action_held", "action_released", "filtered_input_angle_bams", "grounded", "action_state", "jump_ticks", "surface_id", "surface_flags"}

def tolerance_map(items, default_abs, default_rel):
    result = {}
    for item in items:
        try:
            field, values = item.split("=", 1)
            absolute, relative = values.split(":", 1)
            result[field] = (float(absolute), float(relative))
        except ValueError:
            raise SystemExit("invalid --tolerance; expected FIELD=ABS:REL")
    return result, (default_abs, default_rel)

def compare(reference_path, generated_path, tolerances):
    per_field, default = tolerances
    with open(reference_path, newline="") as left_file, open(generated_path, newline="") as right_file:
        left = csv.DictReader(left_file); right = csv.DictReader(right_file)
        if left.fieldnames != right.fieldnames:
            return "header divergence:\n  reference=%r\n  generated=%r" % (left.fieldnames, right.fieldnames)
        row_index = 0
        while True:
            left_row = next(left, None); right_row = next(right, None)
            if left_row is None or right_row is None:
                if left_row == right_row: return None
                return "length divergence after %d data frames" % row_index
            row_index += 1
            for field in left.fieldnames:
                if field in INTEGER_FIELDS:
                    if left_row[field] != right_row[field]:
                        return "first divergence at data row %d, frame %s, field %s:\n  reference=%s\n  generated=%s" % (
                            row_index, left_row.get("frame", "?"), field, left_row[field], right_row[field])
                    continue
                try: a, b = float(left_row[field]), float(right_row[field])
                except ValueError:
                    if left_row[field] != right_row[field]:
                        return "first divergence at data row %d, field %s: %r != %r" % (row_index, field, left_row[field], right_row[field])
                    continue
                absolute = abs(a - b)
                relative = absolute / max(abs(a), abs(b)) if max(abs(a), abs(b)) else 0.0
                abs_tol, rel_tol = per_field.get(field, default)
                if not math.isclose(a, b, abs_tol=abs_tol, rel_tol=rel_tol):
                    return ("first divergence at data row %d, frame %s, field %s:\n"
                            "  reference=%.17g\n  generated=%.17g\n  absolute_difference=%.17g\n"
                            "  relative_difference=%.17g\n  tolerances=abs %.9g, rel %.9g") % (
                                row_index, left_row.get("frame", "?"), field, a, b, absolute,
                                relative, abs_tol, rel_tol)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("reference"); parser.add_argument("generated")
    parser.add_argument("--abs", type=float, default=1e-6, dest="absolute")
    parser.add_argument("--rel", type=float, default=1e-6, dest="relative")
    parser.add_argument("--tolerance", action="append", default=[])
    args = parser.parse_args()
    divergence = compare(args.reference, args.generated,
                         tolerance_map(args.tolerance, args.absolute, args.relative))
    if divergence:
        print("FAIL: " + divergence); return 1
    print("PASS: traces match within configured tolerances"); return 0

if __name__ == "__main__":
    raise SystemExit(main())
