#!/usr/bin/python3
import os
import time

# bpftrace -c resumes this child only after attaching its probes.
# sys_enter_getpid marks the shared beginning of the measurement window.
os.getpid()
time.sleep(61)
