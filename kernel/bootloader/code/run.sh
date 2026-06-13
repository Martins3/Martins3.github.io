#!/usr/bin/env bash
nasm -f bin boot$1.asm -o boot.bin
qemu-system-x86_64 -fda boot.bin
