#!/usr/bin/python3
import json
import subprocess
import time
from pathlib import Path

OUT = Path(__file__).resolve().parent
BPFTRACE = '/home/martins3/.nix-profile/bin/bpftrace'
BPFTOOL = '/home/martins3/.nix-profile/bin/bpftool'

window = '@start && nsecs - @start < 10000000000'
points = {
    'scan_entry': 'kprobe:kvm:vcpu_scan_ioapic',
    'eoi_set': 'kprobe:kvm:vcpu_scan_ioapic+153',
    'scan_check': 'kprobe:kvm:vcpu_enter_guest.constprop.0+3403',
    'eoi_check': 'kprobe:kvm:vcpu_enter_guest.constprop.0+3294',
    'steal_set': 'kprobe:kvm:kvm_arch_vcpu_load+229',
    'steal_check': 'kprobe:kvm:vcpu_enter_guest.constprop.0+2553',
    'steal_handler': 'kprobe:kvm:record_steal_time',
}
source = '\n'.join(f'{probe} /{window}/ {{ @{key} = count(); }}' for key, probe in points.items())
source += '\ninterval:s:1 { if (!@start) { @start = nsecs; } else if (nsecs - @start >= 10000000000) { exit(); } }\n'
(OUT / 'diagnose.bt').write_text(source)
before = json.loads(subprocess.check_output([BPFTOOL, '-j', 'prog', 'show']))
old_ids = {p['id'] for p in before}
with (OUT / 'diagnose.jsonl').open('w') as out:
    p = subprocess.Popen([BPFTRACE, '-f', 'json', str(OUT / 'diagnose.bt')], stdout=out)
    snapshots = []
    while p.poll() is None:
        try:
            p.wait(timeout=2)
        except subprocess.TimeoutExpired:
            progs = json.loads(subprocess.check_output([BPFTOOL, '-j', 'prog', 'show']))
            progs = [x for x in progs if x['id'] not in old_ids]
            links = json.loads(subprocess.check_output([BPFTOOL, '-j', 'link', 'show']))
            text = Path('/sys/kernel/debug/kprobes/list').read_text()
            snapshots.append(dict(time=time.time(), programs=progs, links=links, kprobes=text))
(OUT / 'diagnose-snapshots.json').write_text(json.dumps(snapshots, indent=2) + '\n')
print((OUT / 'diagnose.jsonl').read_text())
raise SystemExit(p.returncode)
