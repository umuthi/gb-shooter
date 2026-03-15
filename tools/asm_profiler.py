#!/usr/bin/env python3
"""
asm_profiler.py — Analyze GBDK .lst files for GBZ80 performance issues.

Reads all src/*.lst files produced by the -Wa-l compiler flag and reports:
  - Instruction count per function (heaviest first)
  - 16-bit operations in inner loops (ld hl/de/bc, add hl, etc.)
  - Unnecessary VRAM/OAM calls (set_win_tiles, set_bkg_tiles, move_sprite)
  - Software division/modulo calls (slow SDCC runtime routines)
  - Any function called inside another function's loop estimate

Usage:
    python3 tools/asm_profiler.py [--src DIR] [--top N] [--verbose]

Options:
    --src DIR    Directory containing .lst files  (default: ./src)
    --top N      Show top N heaviest functions     (default: 15)
    --verbose    Show all flagged instructions, not just counts
"""

import sys
import re
import argparse
from pathlib import Path
from collections import defaultdict


# ── Patterns ──────────────────────────────────────────────────────────────

# ASxxxx .lst format produced by sdasgb:
#   "    00000000                         58 _enemies_init::"
#   "    00000001 EA 48 00         [16]   62 \tld\t(#_wave_num), a"
#   Optionally prefixed with "u" for undefined symbols.

# Function entry: address + whitespace + line# + _name::
RE_FUNC_ENTRY = re.compile(r'[0-9A-Fa-f]{8}\s+\d+\s+([_A-Za-z]\w*)::\s*$')

# Any label (single colon) — used to detect loop structure
RE_LABEL      = re.compile(r'[0-9A-Fa-f]{8}\s+\d+\s+([_A-Za-z0-9]\w*\d*\$?):\s*$')

# Instruction line: address + hex bytes + cycle count + line# + mnemonic
# e.g.: "    00000001 EA 48 00         [16]   62 \tld\t(#_wave_num), a"
RE_INSTR      = re.compile(r'u?\s*[0-9A-Fa-f]{8}\s+(?:[0-9A-Fa-f]{2}\s+)+\[\s*\d+\]\s+\d+\s+(.+)$')

# 16-bit register loads / arithmetic — expensive on GBZ80
RE_16BIT = re.compile(
    r'\b(ld\s+(hl|de|bc|sp)\s*,|add\s+hl\s*,|ld\s+\(\s*(hl|de|bc)\s*\)\s*,\s*(hl|de|bc)|'
    r'ld\s+(hl|de|bc)\s*,\s*\()',
    re.IGNORECASE
)

# VRAM / OAM write calls — each stalls waiting for safe VRAM window
RE_VRAM = re.compile(
    r'\bcall\s+(_set_win_tiles|_set_bkg_tiles|_set_bkg_tile|_set_win_tile|'
    r'_set_sprite_tile|_set_sprite_prop)\b',
    re.IGNORECASE
)

# OAM position writes — less costly than VRAM but still a function call per frame
RE_OAM = re.compile(r'\bcall\s+_move_sprite\b', re.IGNORECASE)

# Software division / modulo — very slow on Z80 (no hardware divide)
RE_DIV = re.compile(
    r'\bcall\s+(__div|__mod|__mul|_divsint|_modsint|_divuchar|_moduchar|'
    r'_divschar|_modschar|_divuint|_moduint)\b',
    re.IGNORECASE
)

# Jump-back instructions — rough heuristic for loop detection
RE_LOOP_JMP = re.compile(r'\b(djnz|jr\s+(c,|nc,|z,|nz,|)\s*\d*\$)', re.IGNORECASE)


# ── LST parser ────────────────────────────────────────────────────────────

class Function:
    def __init__(self, name, source_file):
        self.name        = name
        self.source_file = source_file
        self.instr_count = 0
        self.byte_count  = 0
        self.issues      = []   # list of (issue_type, detail, line)
        self.has_loop    = False
        self.calls       = []   # called function names


def parse_lst_file(path):
    """Parse one .lst file; return list of Function objects."""
    functions = []
    current   = None
    in_loop   = False
    source    = path.stem  # e.g. "enemy" from "enemy.lst"

    for raw_line in path.read_text(errors="replace").splitlines():
        line = raw_line.strip()

        # Function entry
        m = RE_FUNC_ENTRY.match(line)
        if m:
            current = Function(m.group(1), source)
            functions.append(current)
            in_loop = False
            continue

        if current is None:
            continue

        # Count loop jump-backs as a rough loop indicator
        if RE_LOOP_JMP.search(line):
            in_loop = True
            current.has_loop = True

        # Instruction line — count it
        m = RE_INSTR.match(raw_line)
        if m:
            current.instr_count += 1
            # Count bytes from the hex section
            hex_part = re.findall(r'[0-9A-Fa-f]{2}', raw_line.split()[1:4+1][:3][0:1][0] if len(raw_line.split()) > 1 else "")
            mnemonic_area = m.group(1)

            # 16-bit ops — more concerning inside a loop
            if RE_16BIT.search(mnemonic_area):
                severity = "WARN" if current.has_loop else "info"
                current.issues.append((
                    "16BIT",
                    f"{severity}: 16-bit op {'(in loop)' if current.has_loop else ''}: {mnemonic_area.strip()}",
                    line
                ))

            # VRAM write calls
            vm = RE_VRAM.search(mnemonic_area)
            if vm:
                current.issues.append((
                    "VRAM",
                    f"WARN: VRAM write call {'(in loop — expensive!)' if current.has_loop else ''}: {vm.group(1)}",
                    line
                ))
                fn = vm.group(1).lstrip('_')
                current.calls.append(fn)

            # OAM calls
            om = RE_OAM.search(mnemonic_area)
            if om:
                current.issues.append((
                    "OAM",
                    f"{'WARN: move_sprite in loop' if current.has_loop else 'info: move_sprite call'}",
                    line
                ))
                current.calls.append("move_sprite")

            # Division
            dm = RE_DIV.search(mnemonic_area)
            if dm:
                current.issues.append((
                    "DIV",
                    f"WARN: software division/modulo {'(in loop — very slow!)' if current.has_loop else ''}: {dm.group(1)}",
                    line
                ))

    return functions


# ── Reporter ──────────────────────────────────────────────────────────────

ISSUE_SYMBOLS = {
    "16BIT": "⚡",
    "VRAM":  "📺",
    "OAM":   "🎮",
    "DIV":   "➗",
}

def issue_symbol(kind):
    return {"16BIT": "[16b]", "VRAM": "[vram]", "OAM": "[oam]", "DIV": "[div]"}.get(kind, "[?]")


def report(functions, top_n, verbose):
    # Sort by instruction count descending
    ranked = sorted(functions, key=lambda f: f.instr_count, reverse=True)

    warn_funcs = [f for f in functions if any(
        "WARN" in i[1] for i in f.issues
    )]

    total_instrs = sum(f.instr_count for f in functions)

    print("=" * 60)
    print("ASM PROFILER REPORT")
    print("=" * 60)
    print(f"Functions analyzed : {len(functions)}")
    print(f"Total instructions : {total_instrs}")
    print()

    # ── Top N heaviest ────────────────────────────────────────
    print(f"TOP {top_n} HEAVIEST FUNCTIONS")
    print("-" * 60)
    print(f"{'Rank':<5} {'Instructions':>12}  {'Loop':^5}  {'Issues':^20}  Function")
    print(f"{'----':<5} {'------------':>12}  {'-----':^5}  {'------':^20}  --------")

    for rank, fn in enumerate(ranked[:top_n], 1):
        pct   = fn.instr_count / total_instrs * 100 if total_instrs else 0
        loop  = "  ✓  " if fn.has_loop else "     "
        issue_kinds = list(dict.fromkeys(i[0] for i in fn.issues))
        issue_str = " ".join(issue_symbol(k) for k in issue_kinds)
        src = f"({fn.source_file}.c)"
        print(f"  {rank:<3} {fn.instr_count:>8} {pct:>5.1f}%  {loop}  {issue_str:<20}  {fn.name} {src}")

    print()

    # ── Warnings ─────────────────────────────────────────────
    if warn_funcs:
        print("PERFORMANCE WARNINGS")
        print("-" * 60)
        for fn in sorted(warn_funcs, key=lambda f: f.instr_count, reverse=True):
            warnings = [i for i in fn.issues if "WARN" in i[1]]
            print(f"\n  {fn.name}() [{fn.source_file}.c]  —  {fn.instr_count} instructions"
                  f"{'  [has loop]' if fn.has_loop else ''}")
            seen = set()
            for kind, detail, ctx in warnings:
                key = (kind, detail[:60])
                if key in seen and not verbose:
                    continue
                seen.add(key)
                sym = issue_symbol(kind)
                print(f"    {sym}  {detail}")
                if verbose:
                    print(f"         context: {ctx[:70]}")
    else:
        print("No performance warnings detected.")

    print()

    # ── Recommendations ───────────────────────────────────────
    print("RECOMMENDATIONS")
    print("-" * 60)

    vram_in_loop = [f for f in functions if any(
        i[0] == "VRAM" and "loop" in i[1] for i in f.issues
    )]
    if vram_in_loop:
        names = ", ".join(f.name for f in vram_in_loop[:3])
        print(f"  [vram] VRAM writes inside loops detected in: {names}")
        print(f"         → Cache values and only write when dirty (dirty-flag pattern)")

    div_fns = [f for f in functions if any(i[0] == "DIV" for i in f.issues)]
    if div_fns:
        names = ", ".join(f.name for f in div_fns[:3])
        print(f"  [div]  Division/modulo in: {names}")
        print(f"         → Replace with bit-shifts (÷2=>>1, ÷4=>>2) or lookup tables")

    heavy_16bit = [f for f in functions if
                   sum(1 for i in f.issues if i[0] == "16BIT" and "WARN" in i[1]) >= 3]
    if heavy_16bit:
        names = ", ".join(f.name for f in heavy_16bit[:3])
        print(f"  [16b]  Repeated 16-bit ops in loops: {names}")
        print(f"         → Use uint8_t locals; avoid int16_t in inner loops")

    oam_in_loop = [f for f in functions if any(
        i[0] == "OAM" and "loop" in i[1] for i in f.issues
    )]
    if oam_in_loop:
        names = ", ".join(f.name for f in oam_in_loop[:3])
        print(f"  [oam]  move_sprite in loops: {names}")
        print(f"         → This is expected for update functions; ensure sprites are")
        print(f"           cached with 'active' checks before calling move_sprite")

    if not any([vram_in_loop, div_fns, heavy_16bit, oam_in_loop]):
        print("  No obvious hotspot issues found.")

    print()


# ── Main ──────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="GBDK .lst performance profiler")
    parser.add_argument("--src",     default="src", help="Directory with .lst files")
    parser.add_argument("--top",     type=int, default=15)
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()

    src_dir = Path(args.src)
    lst_files = sorted(src_dir.glob("*.lst"))

    if not lst_files:
        sys.exit(
            f"No .lst files found in '{src_dir}'.\n"
            "Make sure CFLAGS includes -Wa-l and run 'make' first."
        )

    all_functions = []
    for lst in lst_files:
        fns = parse_lst_file(lst)
        all_functions.extend(fns)
        print(f"  Parsed {lst.name}: {len(fns)} functions")

    print()
    report(all_functions, args.top, args.verbose)


if __name__ == "__main__":
    main()
