#!/usr/bin/env python3
"""A function list for a game's code without a disassembler: the recompiler's own analysis plus a prologue scan.

    python3 tools/function_table.py --exheader exheader.bin --code code.bin -o build/functions_found.tsv
    python3 tools/build_function_table.py --exheader exheader.bin --code code.bin \
        --functions build/functions_found.tsv --symbols config/symbols.txt --dti-ctor 0x... -o config/functions.tsv

A new game's first config/functions.tsv (the MH4U table came from Ghidra's analysis; tools/ghidra/). The passes:

1. the entry point (the start of .text) and the static constructors of RVCT's init table, followed through every
   direct call (tools/recomp/analysis.py, the recompiler's analysis), then the code addresses in .rodata/.data and in
   the literal pools that pass recompile.py's checks (vtables, callback tables, function pointer arguments);
2. the gaps between the functions found: an ARM prologue (push {..., lr}, str lr, [sp, #-4]!) that starts a gap, or
   a data pointer to one, as tools/find_missing_functions.py lists them for Ghidra's second pass; and a Thumb
   push {..., lr} that starts a gap near other Thumb code;
3. each candidate is kept only when its code decodes cleanly (no trap, no analysis problem) and does not overlap code
   of the other instruction set; the new functions and what they call go back to step 2 until nothing new turns up.

The output has Ghidra's export columns that tools/build_function_table.py reads (address, size, mode, name), one row per
function, sizes up to the next function. Code reached only through pointers made at run time is still missing: the
native runtime interprets it and appends each entry to build/missing_entries.txt, for config/extra_entries.txt.
The list is addresses and sizes only (no game data); the code it is made from stays on your machine.
"""
from __future__ import annotations

import argparse
import bisect
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import codebin2elf  # noqa: E402
import exheader as exh  # noqa: E402
import find_missing_functions as fmf  # noqa: E402
import recompile  # noqa: E402
from recomp.analysis import Analyzer, Image  # noqa: E402

# RVCT's start-up code in ARM (the Thumb form is recompile.py's init_array_entries): the static constructors' table of
# offsets relative to each entry, between two PC-relative bounds
#     ldr r4, =table-.; add r4, pc, r4; ldr r5, =end-.; add r5, pc, r5; b test
#     loop: ldr r0, [r4]; add lr, pc, #4; add r0, r0, r4; mov pc, r0; add r4, r4, #4; test: cmp r4, r5; bne loop
ARM_INIT_LOOP = struct.pack("<6I", 0xE5940000, 0xE28FE004, 0xE0800004, 0xE1A0F000, 0xE2844004, 0xE1540005)
THUMB_NEAR = 0x4000      # a Thumb candidate must sit this close to known Thumb code (recompile.py's rule)
MAX_ROUNDS = 12


def arm_init_entries(img: Image) -> dict[int, str]:
    """The static constructors of an ARM init loop (ARM_INIT_LOOP), which neither the data-pointer scan nor
    recompile.py's Thumb-only init_array_entries see."""
    found: dict[int, str] = {}
    text = img.code[: img.text_size]
    pos = text.find(ARM_INIT_LOOP)
    while pos != -1:
        loop = img.base + pos
        bounds = {}
        for k in range(1, 8):                   # ldr rN, [pc, #i] / add rN, pc, rN before the loop
            a = loop - 4 * k
            w, nxt = img.read32(a), img.read32(a + 4)
            if w is None or nxt is None:
                break
            reg = (w >> 12) & 0xF
            if (w & 0xFFFF0000) == 0xE59F0000 and nxt == (0xE08F0000 | reg << 16 | reg << 12 | reg):
                val = img.read32(a + 8 + (w & 0xFFF))
                if val is not None and reg not in bounds:
                    bounds[reg] = (val + a + 4 + 8) & 0xFFFFFFFF    # add rN, pc, rN: pc = add's address + 8
        if 4 in bounds and 5 in bounds and 0 < bounds[5] - bounds[4] < 0x100000:
            for e in range(bounds[4], bounds[5], 4):
                off = img.read32(e)
                if off is None:
                    break
                t = (e + off) & 0xFFFFFFFF
                if img.in_text(t & ~1):
                    found[t & ~1] = "thumb" if t & 1 else "arm"
        pos = text.find(ARM_INIT_LOOP, pos + 4)
    return found


def is_thumb_prologue(hw: int) -> bool:
    """push {..., lr} with at least one low register: 0xB5xx, xx != 0."""
    return (hw & 0xFF00) == 0xB500 and (hw & 0xFF) != 0


def extents(funcs: dict, text_end: int) -> list[tuple[int, int, str]]:
    """(address, size, mode) per function: from its entry to the end of its last instruction, cut at the next
    function's entry (code shared by two functions belongs to the first)."""
    starts = sorted(funcs)
    out = []
    for i, a in enumerate(starts):
        f = funcs[a]
        end = max((x + ins.size for x, ins in f.insns.items() if x >= a), default=a + 2)
        nxt = starts[i + 1] if i + 1 < len(starts) else text_end
        out.append((a, max(2, min(end, nxt) - a), f.mode))
    return out


def gap_candidates(img: Image, header: exh.ExHeader, funcs: dict) -> dict[int, str]:
    """Prologues at the start of the gaps between the known functions (ARM and Thumb), and ARM prologues that data
    points to (tools/find_missing_functions.py's rules)."""
    ext = extents(funcs, img.text_end)
    found = {a: "arm" for a in fmf.find_missing(img.code, header, [(a, s) for a, s, _ in ext])}
    thumb = sorted(a for a, f in funcs.items() if f.mode == "thumb")
    for lo, hi in fmf.gaps([(a, s) for a, s, _ in ext], img.base, img.text_end):
        a = (lo + 1) & ~1
        # skip the padding a gap starts with (zeros, ARM nops), then look at the first instruction of what follows
        while a < hi - 1 and img.read16(a) in (0x0000, 0x46C0, 0xBF00):
            a += 2
        hw = img.read16(a) if a < hi - 1 else None
        if hw is not None and is_thumb_prologue(hw) and a not in found and recompile.near(thumb, a, THUMB_NEAR):
            found[a] = "thumb"
    return found


def accept(an: Analyzer, a: int, mode: str, known: dict[int, str], cover: dict[str, set[int]]) -> bool:
    if not an.plausible_entry(a, mode):
        return False
    if mode == "arm" and (img_word := an.img.read32(a)) is not None and img_word >> 28 != 0xE:
        return False
    f = an.analyze(a, mode, known)
    if f.problems or any(i.kind == "trap" for i in f.insns.values()):
        return False
    other = cover["thumb" if mode == "arm" else "arm"]
    return not any(h in other for x, i in f.insns.items() for h in range(x, x + i.size, 2))


def find_functions(code: bytes, header: exh.ExHeader, extra: dict[int, str] | None = None, log=print,
                   data_pointers: bool = True) -> dict:
    """The functions of the code (recomp.analysis.Function by entry), from the entry point, the init table, `extra`
    entries and data pointers, then rounds of gap prologues until no new one passes."""
    img = Image(code, header.text.address, recompile.code_size(header, code))
    an = Analyzer(img)
    entries = {img.base: "arm"}
    entries.update(recompile.init_array_entries(img))
    entries.update(arm_init_entries(img))
    entries.update({a: m for a, m in (extra or {}).items() if img.in_text(a)})
    log(f"start: {len(entries)} entry points (the entry, the init table, extra)")
    funcs = an.analyze_all(entries)
    if data_pointers:
        cover, fall_into = recompile.code_cover(funcs)
        known = {a: f.mode for a, f in funcs.items()}
        literals = {i.literal for f in funcs.values() for i in f.insns.values() if i.literal is not None}
        segs = [(header.ro.address, header.ro.size), (header.data.address, header.data.size)]
        more = recompile.data_pointer_entries(img, an, segs, known, cover, fall_into, literals)
        log(f"data pointers: {len(more)} entry points")
        funcs = an.analyze_all({**known, **more})
    for rnd in range(1, MAX_ROUNDS + 1):
        known = {a: f.mode for a, f in funcs.items()}
        cover, _ = recompile.code_cover(funcs)
        added = {a: m for a, m in gap_candidates(img, header, funcs).items()
                 if a not in known and accept(an, a, m, known, cover)}
        log(f"round {rnd}: {len(funcs)} functions, {len(added)} new from gap prologues "
            f"({sum(1 for m in added.values() if m == 'thumb')} Thumb)")
        if not added:
            break
        funcs = an.analyze_all({**known, **added})
    return funcs


def write_tsv(funcs: dict, text_end: int, path: str) -> None:
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("address\tsize\tmode\tname\n")
        for a, size, mode in extents(funcs, text_end):
            f.write(f"0x{a:08x}\t0x{size:x}\t{mode}\t\n")


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--exheader", required=True)
    ap.add_argument("--code", required=True, help="code.bin, compressed or not")
    ap.add_argument("--extra", action="append", default=[],
                    help="more entry points (address [arm|thumb] per line, as config/extra_entries.txt)")
    ap.add_argument("--no-data-pointers", action="store_true")
    ap.add_argument("-o", "--output", required=True)
    args = ap.parse_args(argv)
    with open(args.exheader, "rb") as f:
        header = exh.parse(f.read())
    with open(args.code, "rb") as f:
        code = codebin2elf.load_code(header, f.read())
    extra = {}
    for path in args.extra:
        with open(path, encoding="utf-8") as f:
            for line in f:
                line = line.split("#", 1)[0].strip()
                if line:
                    parts = line.split()
                    a = int(parts[0], 16)
                    extra[a & ~1] = "thumb" if a & 1 or (len(parts) > 1 and parts[1] == "thumb") else "arm"
    funcs = find_functions(code, header, extra, data_pointers=not args.no_data_pointers)
    text_end = header.text.address + recompile.code_size(header, code)
    write_tsv(funcs, text_end, args.output)
    thumb = sum(1 for f in funcs.values() if f.mode == "thumb")
    print(f"{len(funcs)} functions ({thumb} Thumb), "
          f"{sum(len(f.insns) for f in funcs.values())} instructions; wrote {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
