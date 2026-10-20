#!/usr/bin/python3
"""Build audited probes from this host's DWARF and exact module instructions."""

import collections
import json
import re
import subprocess
from pathlib import Path
from typing import Any

from elftools.dwarf.dwarf_expr import DWARFExprParser
from elftools.dwarf.locationlists import LocationParser
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import NoteSection, SymbolTableSection

OUT = Path(__file__).resolve().parent
RELEASE = "7.2.7-200.fc44.x86_64"
DEBUG = Path("/usr/lib/debug/usr/lib/modules") / RELEASE / "kernel/arch/x86/kvm"


def name(die):
    if "DW_AT_name" in die.attributes:
        return die.attributes["DW_AT_name"].value.decode()
    if "DW_AT_abstract_origin" in die.attributes:
        return name(die.get_DIE_from_attribute("DW_AT_abstract_origin"))
    return ""


def ranges(die, dwarf):
    a = die.attributes
    if "DW_AT_low_pc" in a:
        lo = a["DW_AT_low_pc"].value
        hi = a["DW_AT_high_pc"]
        return [(lo, hi.value if hi.form == "DW_FORM_addr" else lo + hi.value)]
    if "DW_AT_ranges" not in a:
        return []
    base = die.cu.get_top_DIE().attributes.get("DW_AT_low_pc")
    base = base.value if base else 0
    result = []
    for entry in dwarf.range_lists().get_range_list_at_offset(
        a["DW_AT_ranges"].value, die.cu
    ):
        if hasattr(entry, "base_address"):
            base = entry.base_address
        else:
            off = 0 if entry.is_absolute else base
            result.append((off + entry.begin_offset, off + entry.end_offset))
    return result


def expressions(child, lp, xp):
    a = child.attributes
    if "DW_AT_const_value" in a:
        return [("constant", a["DW_AT_const_value"].value)]
    if "DW_AT_location" not in a:
        return []
    loc = lp.parse_from_attribute(a["DW_AT_location"], child.cu["version"], die=child)
    items = [loc] if hasattr(loc, "loc_expr") else loc
    result = []
    for item in items:
        if not hasattr(item, "loc_expr"):
            continue
        ops = xp.parse_expr(item.loc_expr)
        if len(ops) == 2 and ops[1].op_name == "DW_OP_stack_value":
            if ops[0].op_name.startswith("DW_OP_lit"):
                result.append(("constant", int(ops[0].op_name[9:])))
            elif ops[0].op_name.startswith("DW_OP_const"):
                result.append(("constant", ops[0].args[0]))
            else:
                result.append(("other", str(ops)))
        else:
            result.append(("other", str(ops)))
    return result


def analyze(module):
    text = subprocess.check_output(
        ["objdump", "-d", "--no-show-raw-insn", str(OUT / (module + ".ko"))], text=True
    )
    instructions = collections.defaultdict(dict)
    section = None
    for line in text.splitlines():
        m = re.match(r"Disassembly of section (.+):", line)
        if m:
            section = m[1]
        m = re.match(r"\s*([0-9a-f]+):\s+(.+)", line)
        if m:
            instructions[section][int(m[1], 16)] = m[2].strip()
    rows = []
    rejected: list[dict[str, Any]] = []
    with (DEBUG / (module + ".ko.debug")).open("rb") as stream:
        elf = ELFFile(stream)
        dwarf = elf.get_dwarf_info()
        lp = LocationParser(dwarf.location_lists())
        xp = DWARFExprParser(dwarf.structs)
        symtab = elf.get_section_by_name(".symtab")
        notes = elf.get_section_by_name(".note.gnu.build-id")
        assert isinstance(symtab, SymbolTableSection)
        assert isinstance(notes, NoteSection)
        symbols = [
            s
            for s in symtab.iter_symbols()
            if s["st_info"]["type"] == "STT_FUNC" and s["st_size"]
        ]
        build_id = next(n["n_desc"] for n in notes.iter_notes())
        for cu in dwarf.iter_CUs():
            for die in cu.iter_DIEs():
                if die.tag != "DW_TAG_inlined_subroutine" or name(die) not in (
                    "__kvm_make_request",
                    "kvm_clear_request",
                ):
                    continue
                clearing = name(die) == "kvm_clear_request"
                parent = die.get_parent()
                direct = False
                checked = False
                call = die
                while parent is not None and parent.tag != "DW_TAG_subprogram":
                    if name(parent) == "kvm_make_request":
                        direct = True
                        call = parent
                    if name(parent) == "kvm_check_request":
                        checked = True
                        call = parent
                    parent = parent.get_parent()
                if clearing and not checked:
                    continue
                parent_name = name(parent) if parent else ""
                rr = ranges(die, dwarf)
                params = {
                    name(c): expressions(c, lp, xp)
                    for c in die.iter_children()
                    if c.tag == "DW_TAG_formal_parameter"
                }
                vals = set(v for k, v in params.get("req", []) if k == "constant")
                row = dict(
                    module=module.replace("-", "_"),
                    parent=parent_name,
                    die=hex(die.offset),
                    ranges=rr,
                    path="check" if clearing else ("direct" if direct else "bulk"),
                    req=next(iter(vals)) if len(vals) == 1 else None,
                    expressions=params.get("req", []),
                    call_line=call.attributes["DW_AT_call_line"].value,
                )
                candidates = []
                for sym in symbols:
                    if sym.name != parent_name and not sym.name.startswith(
                        parent_name + "."
                    ):
                        continue
                    sec = elf.get_section(sym["st_shndx"]).name
                    for lo, hi in rr:
                        for address, insn in instructions[sec].items():
                            if (
                                lo <= address < hi
                                and sym["st_value"]
                                <= address
                                < sym["st_value"] + sym["st_size"]
                            ):
                                if re.match(
                                    r"lock (and|btr)" if clearing else r"lock (or|bts)",
                                    insn,
                                ):
                                    candidates.append((sym, sec, address, insn))
                unique = {
                    (s.name, sec, addr): (s, sec, addr, insn)
                    for s, sec, addr, insn in candidates
                }
                if not unique:
                    rejected.append(row)
                    continue
                for sym, sec, address, insn in unique.values():
                    result = dict(
                        row,
                        symbol=sym.name,
                        section=sec,
                        address=address,
                        offset=address - sym["st_value"],
                        instruction=insn,
                        build_id=build_id,
                    )
                    rows.append(result)
    aliases, remaining = [], []
    for bad in rejected:
        covered = None
        for row in rows:
            if (row["parent"], row["req"], row["path"]) != (
                bad["parent"],
                bad["req"],
                bad["path"],
            ):
                continue
            for lo, hi in bad["ranges"]:
                insn = instructions[row["section"]].get(lo, "")
                jump = re.match(r"jmp\s+([0-9a-f]+)\s", insn)
                if (lo == hi and lo == row["address"]) or (
                    jump and int(jump[1], 16) == row["address"]
                ):
                    covered = row
        if covered:
            aliases.append(
                dict(
                    bad,
                    covered_by=f"{covered['module']}:{covered['symbol']}+{covered['offset']}",
                )
            )
        else:
            remaining.append(bad)
    return rows, remaining, aliases


def main():
    all_rows, rejected, aliases = [], [], []
    for module in ["kvm", "kvm-intel"]:
        rows, bad, shared = analyze(module)
        all_rows.extend(rows)
        rejected.extend(bad)
        aliases.extend(shared)
        print(
            module,
            "instructions",
            len(rows),
            "unresolved DIEs",
            len(bad),
            "merged DIEs",
            len(shared),
        )
    (OUT / "inventory.json").write_text(json.dumps(all_rows, indent=2) + "\n")
    (OUT / "unresolved.json").write_text(json.dumps(rejected, indent=2) + "\n")
    (OUT / "shared-sites.json").write_text(json.dumps(aliases, indent=2) + "\n")
    for row in all_rows:
        print(
            row["module"],
            row["path"],
            row["req"],
            row["symbol"] + "+" + str(row["offset"]),
            row["instruction"],
        )
    print("UNRESOLVED", json.dumps(rejected, indent=2))


if __name__ == "__main__":
    main()
