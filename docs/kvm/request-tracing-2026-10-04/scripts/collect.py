#!/usr/bin/python3
"""Validate this exact build, run bounded BPF sampling, auto-detach on exit."""

import collections
import json
import os
import re
import struct
import subprocess
import time
from pathlib import Path

OUT = Path(__file__).resolve().parent
RELEASE = "7.2.7-200.fc44.x86_64"
BPFTRACE = "/home/martins3/.nix-profile/bin/bpftrace"
BPFTOOL = "/home/martins3/.nix-profile/bin/bpftool"


def build_id(module):
    data = Path(f"/sys/module/{module}/notes/.note.gnu.build-id").read_bytes()
    namesz, descsz, _ = struct.unpack_from("<III", data)
    offset = 12 + ((namesz + 3) & ~3)
    return data[offset : offset + descsz].hex()


def main():
    if os.uname().release != RELEASE:
        raise SystemExit("Kernel version changed; regenerate the probes.")
    inventory = json.loads((OUT / "inventory.json").read_text())
    ids = {row["module"]: row["build_id"] for row in inventory}
    for module, expected in ids.items():
        if build_id(module) != expected:
            raise SystemExit(f"Loaded build-id mismatch: {module}")
    # Validate names against loaded kallsyms before asking bpftrace to attach.
    symbols = {}
    for line in Path("/proc/kallsyms").read_text().splitlines():
        fields = line.split()
        if len(fields) == 4:
            symbols[(fields[3].strip("[]"), fields[2])] = int(fields[0], 16)
    sites = {}
    for row in inventory:
        if (row["module"], row["symbol"]) not in symbols:
            raise SystemExit(f"No loaded symbol: {row['module']}:{row['symbol']}")
        key = (row["module"], row["symbol"], row["offset"])
        if key in sites:
            raise SystemExit(f"Duplicate probe location: {key}")
        sites[key] = row
    vms = []
    for proc in Path("/proc").iterdir():
        if not proc.name.isdigit():
            continue
        try:
            if not (proc / "comm").read_text().startswith("qemu-system"):
                continue
            args = (proc / "cmdline").read_bytes().decode().split("\0")
            vms.append(
                dict(
                    pid=int(proc.name),
                    **{
                        key.lstrip("-"): args[args.index(key) + 1]
                        for key in ["-name", "-smp", "-m", "-cpu"]
                        if key in args
                    },
                )
            )
        except (FileNotFoundError, PermissionError, ProcessLookupError):
            continue
    source = ['BEGIN { printf("Preparing a common 60-second window\\n"); }']
    window = "@window_start_ns && nsecs - @window_start_ns < 60000000000"
    for row in inventory:
        probe = f"kprobe:{row['module']}:{row['symbol']}+{row['offset']}"
        label = probe
        # bpftrace's symbol enumeration omits this compiler-generated function.
        if row["symbol"] == "kvm_ioapic_send_eoi.part.0":
            base = symbols[(row["module"], row["symbol"])]
            if base == 0:
                raise SystemExit("Kernel symbol addresses are restricted")
            probe = f"kprobe:0x{base + row['offset']:x}"
        req = row["req"]
        if req is None:
            # DWARF: req is DW_OP_reg4 (%rsi); exact instruction checked below.
            if (
                row["symbol"] != "kvm_make_vcpu_request"
                or row["instruction"] != "lock bts %rax,0x38(%rdi)"
            ):
                raise SystemExit("Unexpected variable request location")
            expr = '(uint32)reg("si")'
        else:
            expr = str(req)
            # Verify the immediate bit mask agrees with the DWARF constant.
            match = re.match(r"lock (orb|andb) \$0x([0-9a-f]+),", row["instruction"])
            if not match:
                raise SystemExit(f"Unreviewed instruction: {row}")
            mask = int(match[2], 16)
            if match[1] == "andb":
                mask = (~mask) & 255
            if mask != 1 << ((req & 255) % 8):
                raise SystemExit(f"Bit mismatch: {row}")
        path = row["path"]
        body = f'$req = {expr}; @{path}[$req] = count(); @sites_{path}[$req, "{label}"] = count();'
        if path != "check":
            body += " @submitted[$req] = count();"
        if req == 31 and path != "check":
            body += " @eoi_submit_stack[kstack(12)] = count();"
        if path == "bulk":
            body += " @bulk_submit_stack[$req, kstack(12)] = count();"
        source.append(probe + f" /{window}/ {{ " + body + " }")
    source += [
        f"kprobe:kvm:kvm_make_vcpu_request /({window}) && (arg1 & 1024)/ {{ @no_action[(uint32)arg1] = count(); }}",
        f"kprobe:kvm:kvm_make_all_cpus_request /{window}/ {{ @broadcast[(uint32)arg1] = count(); }}",
        f"kprobe:kvm:kvm_flush_remote_tlbs /{window}/ {{ @remote_flush_calls = count(); }}",
        f"kprobe:kvm:__kvm_vcpu_kick /{window}/ {{ @kick_calls = count(); }}",
        f"tracepoint:kvm:kvm_exit /{window}/ {{ @vmexit_reason[args.exit_reason] = count(); }}",
        'tracepoint:syscalls:sys_enter_getpid /pid == cpid && !@window_start_ns/ { @window_start_ns = nsecs; printf("Window started after attachment\\n"); }',
        "interval:s:1 { if (@window_start_ns && nsecs - @window_start_ns >= 60000000000) { exit(); } }",
    ]
    (OUT / "requests.bt").write_text("\n".join(source) + "\n")
    metadata = dict(
        kernel=RELEASE,
        build_ids=ids,
        vms=vms,
        probes=dict(collections.Counter(row["path"] for row in inventory)),
        duration_seconds=60,
        start_time=time.strftime("%Y-%m-%d %H:%M:%S %z"),
    )
    (OUT / "run.json").write_text(json.dumps(metadata, indent=2) + "\n")
    print(json.dumps(metadata, indent=2), flush=True)
    profile = Path("/sys/kernel/tracing/kprobe_profile")
    events = Path("/sys/kernel/tracing/dynamic_events")
    before = events.read_text()
    profiles = []
    old_ids = {
        x["id"]
        for x in json.loads(subprocess.check_output([BPFTOOL, "-j", "prog", "show"]))
    }
    with (OUT / "bpftrace.stderr").open("w") as err:
        process = subprocess.Popen(
            [
                BPFTRACE,
                "--unsafe",
                "-c",
                f"/usr/bin/python3 {OUT / 'window.py'}",
                "-f",
                "json",
                "-o",
                str(OUT / "counts.jsonl"),
                str(OUT / "requests.bt"),
            ],
            stderr=err,
        )
        while process.poll() is None:
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                progs = json.loads(
                    subprocess.check_output([BPFTOOL, "-j", "prog", "show"])
                )
                progs = [x for x in progs if x["id"] not in old_ids]
                ids = {x["id"] for x in progs}
                links = json.loads(
                    subprocess.check_output([BPFTOOL, "-j", "link", "show"])
                )
                links = [x for x in links if x.get("prog_id") in ids]
                profiles.append(
                    dict(
                        time=time.time(),
                        profile=profile.read_text(),
                        programs=progs,
                        links=links,
                    )
                )
    (OUT / "probe-profiles.json").write_text(json.dumps(profiles, indent=2) + "\n")
    after = events.read_text()
    (OUT / "cleanup.json").write_text(
        json.dumps(
            dict(
                new_dynamic_events=sorted(
                    set(after.splitlines()) - set(before.splitlines())
                ),
                exit_code=process.returncode,
            ),
            indent=2,
        )
        + "\n"
    )
    print((OUT / "bpftrace.stderr").read_text(), flush=True)
    print("bpftrace exit:", process.returncode, flush=True)
    raise SystemExit(process.returncode)


if __name__ == "__main__":
    main()
