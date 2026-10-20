#!/usr/bin/python3
"""Summarize counts without confusing submitted, checked and broadcast units."""

import json
from pathlib import Path

OUT = Path(__file__).resolve().parent
NAMES = {
    2: "KVM_REQ_UNBLOCK",
    8: "KVM_REQ_MIGRATE_TIMER",
    14: "KVM_REQ_EVENT",
    16: "KVM_REQ_STEAL_UPDATE",
    31: "KVM_REQ_LOAD_EOI_EXITMAP",
    768: "KVM_REQ_TLB_FLUSH",
    791: "KVM_REQ_SCAN_IOAPIC",
}


def main():
    maps = {}
    for line in (OUT / "counts.jsonl").read_text().splitlines():
        event = json.loads(line)
        if event["type"] == "map":
            maps.update(event["data"])
    submitted = maps["@submitted"]
    total = sum(submitted.values())
    rows = []
    for key, count in sorted(submitted.items(), key=lambda x: -x[1]):
        direct = maps.get("@direct", {}).get(key, 0)
        bulk = maps.get("@bulk", {}).get(key, 0)
        assert count == direct + bulk
        row = dict(
            req=int(key),
            hex=hex(int(key)),
            name=NAMES.get(int(key), "unknown"),
            submitted=count,
            percent=100 * count / total,
            per_second=count / 60,
            direct=direct,
            bulk_per_target_vcpu=bulk,
            successful_check=maps.get("@check", {}).get(key, 0),
        )
        rows.append(row)
        print(
            f"{row['name']:28} {row['hex']:>5} {count:10,} {row['percent']:6.2f}% {row['per_second']:10.1f}/s direct={direct:,} bulk={bulk:,} check={row['successful_check']:,}"
        )
    print("TOTAL", total, "PER SECOND", total / 60)
    print("BROADCAST CALLS", maps.get("@broadcast", {}))
    print("REMOTE FLUSH CALLS", maps.get("@remote_flush_calls", 0))
    print("KICK CALLS", maps.get("@kick_calls", 0))
    print("DIRECT SITES", json.dumps(maps.get("@sites_direct", {}), indent=2))
    (OUT / "summary.json").write_text(
        json.dumps(dict(total=total, rows=rows), indent=2) + "\n"
    )


if __name__ == "__main__":
    main()
