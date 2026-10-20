#!/usr/bing/env python3
# sudo taskset -ac 1 python3 a.py
# 然后使用  sudo turbostat 来观察，发现所有的 CPU 都进入到 POLL 状态了
import os
import signal
import sys
if not os.path.exists('/dev/cpu_dma_latency'):
    print("no PM QOS interface on this system!")
    sys.exit(1)
try:
    fd = os.open('/dev/cpu_dma_latency', os.O_WRONLY)
    os.write(fd, b'\0\0\0\0')
    print("Press ^C to close /dev/cpu_dma_latency and exit")
    signal.pause()
except KeyboardInterrupt:
    print("closing /dev/cpu_dma_latency")
    os.close(fd)
    sys.exit(0)

