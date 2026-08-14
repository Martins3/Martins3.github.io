#!/usr/bin/python
# Copyright (c) PLUMgrid, Inc.
# Licensed under the Apache License, Version 2.0 (the "License")

# run in project examples directory with:
# sudo ./hello_world.py"
# see trace_fields.py for a longer example

from bcc import BPF

# modern kernels (>= 4.17) name the syscall entry __x64_sys_clone, not sys_clone
BPF(text='int kprobe____x64_sys_clone(void *ctx) { bpf_trace_printk("Hello, World!\\n"); return 0; }').trace_print()
