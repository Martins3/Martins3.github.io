#!/usr/bin/env python3
import socket
import struct
import time

ip = "10.0.2.2"
while True:
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    ts = int(time.time())
    msg = struct.pack("@3Q40s", 0, 1, ts, str.encode("martins3"))
    server_socket.sendto(msg, (ip, 10416))
    time.sleep(1)
