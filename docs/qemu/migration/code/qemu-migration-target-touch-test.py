#!/usr/bin/env python3
import json
import os
import socket
import subprocess
import time
from pathlib import Path

# 用于验证 docs/qemu/migration/zero-page.md 问题
# 这是 codex 写的，codex 比我想的还要 nb 啊

SIZE_MB = int(os.environ.get("SIZE_MB", "5120"))
TARGET_BACKEND = os.environ.get("TARGET_BACKEND", "memfd")  # ram or memfd
BASE = Path("/tmp/qemu-mig-touch")
SRC_QMP = str(BASE / "src.qmp")
DST_QMP = str(BASE / "dst.qmp")
SRC_RAM = str(BASE / "src.ram")
PORT = int(os.environ.get("MIG_PORT", "45678"))
QEMU = os.environ.get("QEMU", "qemu-system-x86_64")
SIZE = SIZE_MB * 1024 * 1024
PAGE = 4096


def run(cmd, **kw):
    print("+", " ".join(map(str, cmd)), flush=True)
    return subprocess.run(cmd, check=True, text=True, **kw)


def qmp_connect(path, timeout=10):
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            s.connect(path)
            break
        except FileNotFoundError:
            time.sleep(0.05)
        except ConnectionRefusedError:
            time.sleep(0.05)
    else:
        raise TimeoutError(path)
    f = s.makefile("rwb", buffering=0)
    greeting = json.loads(f.readline().decode())
    f.write(json.dumps({"execute":"qmp_capabilities"}).encode() + b"\r\n")
    resp = json.loads(f.readline().decode())
    if "error" in resp:
        raise RuntimeError(resp)
    return s, f


def qmp(f, cmd, args=None):
    req = {"execute": cmd}
    if args:
        req["arguments"] = args
    f.write(json.dumps(req).encode() + b"\r\n")
    while True:
        line = f.readline()
        if not line:
            raise EOFError(cmd)
        msg = json.loads(line.decode())
        if "event" in msg:
            continue
        if "error" in msg:
            raise RuntimeError(msg)
        return msg.get("return")


def get_status(pid):
    out = {}
    with open(f"/proc/{pid}/status") as f:
        for line in f:
            if line.startswith(("VmPeak:", "VmSize:", "VmRSS:", "RssAnon:", "RssFile:", "RssShmem:")):
                k, v = line.split(":", 1)
                out[k] = v.strip()
    return out


def get_stat(pid):
    # /proc stat fields: 10=minflt, 12=majflt, after comm parsing
    txt = Path(f"/proc/{pid}/stat").read_text()
    rest = txt[txt.rfind(')')+2:].split()
    return {"minflt": int(rest[7]), "majflt": int(rest[9])}


def measure(pid, label):
    st = get_status(pid)
    faults = get_stat(pid)
    print(f"{label}: pid={pid} VmSize={st.get('VmSize')} VmRSS={st.get('VmRSS')} RssAnon={st.get('RssAnon')} RssFile={st.get('RssFile')} RssShmem={st.get('RssShmem')} minflt={faults['minflt']} majflt={faults['majflt']}", flush=True)


def cleanup(procs):
    for p in procs:
        if p and p.poll() is None:
            p.terminate()
    time.sleep(0.3)
    for p in procs:
        if p and p.poll() is None:
            p.kill()


def main():
    BASE.mkdir(parents=True, exist_ok=True)
    for path in [SRC_QMP, DST_QMP, SRC_RAM]:
        try:
            os.unlink(path)
        except FileNotFoundError:
            pass

    print(f"Preparing source RAM file {SRC_RAM}: configured={SIZE_MB} MiB, target_backend={TARGET_BACKEND}")
    common = [
        QEMU, "-nodefaults", "-display", "none", "-nographic",
        "-serial", "none", "-monitor", "none", "-m", f"{SIZE_MB}M",
    ]
    src_cmd = common + [
        "-S",
        "-object", f"memory-backend-file,id=mem,size={SIZE},mem-path={SRC_RAM},share=on",
        "-machine", "q35,accel=tcg,memory-backend=mem",
        "-qmp", f"unix:{SRC_QMP},server=on,wait=off",
    ]
    if TARGET_BACKEND == "ram":
        target_object = f"memory-backend-ram,id=mem,size={SIZE}"
    elif TARGET_BACKEND == "memfd":
        target_object = f"memory-backend-memfd,id=mem,size={SIZE},share=on"
    else:
        raise ValueError("TARGET_BACKEND must be ram or memfd")

    dst_cmd = common + [
        "-object", target_object,
        "-machine", "q35,accel=tcg,memory-backend=mem",
        "-qmp", f"unix:{DST_QMP},server=on,wait=off",
        "-incoming", f"tcp:127.0.0.1:{PORT}",
    ]

    procs = []
    try:
        dst = subprocess.Popen(dst_cmd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
        procs.append(dst)
        time.sleep(0.5)
        src = subprocess.Popen(src_cmd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
        procs.append(src)
        s1, fsrc = qmp_connect(SRC_QMP)
        s2, fdst = qmp_connect(DST_QMP)
        print("Before migration:")
        measure(dst.pid, "target")
        before = get_stat(dst.pid)
        qmp(fsrc, "migrate", {"uri": f"tcp:127.0.0.1:{PORT}"})
        while True:
            info = qmp(fsrc, "query-migrate")
            status = info.get("status")
            ram = info.get("ram", {})
            print(f"migrate status={status} transferred={ram.get('transferred')} remaining={ram.get('remaining')} total={ram.get('total')}", flush=True)
            if status in ("completed", "failed", "cancelled"):
                break
            time.sleep(0.2)
        time.sleep(0.5)
        print("After migration:")
        measure(dst.pid, "target")
        after = get_stat(dst.pid)
        print(f"target minor faults delta during migration: {after['minflt'] - before['minflt']}")
        try:
            print("target migration status:", qmp(fdst, "query-status"))
        except Exception as e:
            print(f"target query-status failed after migration: {type(e).__name__}: {e}")
        s1.close(); s2.close()
    finally:
        cleanup(procs)

if __name__ == "__main__":
    main()
