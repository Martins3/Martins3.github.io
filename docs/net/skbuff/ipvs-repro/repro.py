#!/usr/bin/env python3
"""IPVS 复现辅助: setup/flush 配置 IPVS, server/client 制造 TCP 重传流量。

用法: repro.py setup | flush | server | client
"""

import socket
import struct
import sys
import threading

VIP = "10.99.0.100"
RS = "10.200.0.2"
VPORT = 8080
RPORT = 9090
CONNS = 120

IP_VS_BASE_CTL = 64 + 1024 + 64
SET_ADD = IP_VS_BASE_CTL + 2
SET_FLUSH = IP_VS_BASE_CTL + 5
SET_ADDDEST = IP_VS_BASE_CTL + 7


def _ctl(op, data=b""):
    socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_RAW).setsockopt(
        0, op, data
    )


def _svc():
    # struct ip_vs_service_user; inet_aton 的字节序 unpack 成 int 再 pack 回去即网络字节序
    addr = struct.unpack("I", socket.inet_aton(VIP))[0]
    return struct.pack(
        "@H2xIH2xI16sIII",
        6,
        addr,
        socket.htons(VPORT),
        0,
        b"rr".ljust(16, b"\0"),
        0,
        0,
        0,
    )


def setup():
    _ctl(SET_FLUSH)
    _ctl(SET_ADD, _svc())
    # struct ip_vs_dest_user
    addr = struct.unpack("I", socket.inet_aton(RS))[0]
    _ctl(
        SET_ADDDEST,
        _svc() + struct.pack("@IH2xIiII", addr, socket.htons(RPORT), 0, 1, 0, 0),
    )


def server():
    srv = socket.socket()
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((RS, RPORT))
    srv.listen(256)
    while True:
        conn, _ = srv.accept()
        threading.Thread(target=_drain, args=(conn,), daemon=True).start()


def _drain(conn):
    try:
        while conn.recv(65536):
            pass
    except OSError:
        pass


def client():
    def work():
        try:
            s = socket.socket()
            s.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 1 << 20)
            s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            s.connect((VIP, VPORT))
            while True:
                s.send(b"x" * 32768)
        except OSError:
            pass

    for _ in range(CONNS):
        threading.Thread(target=work, daemon=True).start()
    threading.Event().wait()


if __name__ == "__main__":
    {
        "setup": setup,
        "flush": lambda: _ctl(SET_FLUSH),
        "server": server,
        "client": client,
    }[sys.argv[1]]()
