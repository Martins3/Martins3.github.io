# Reading

Theseus enables all software written in safe Rust, including
applications and libraries, to coexist alongside the core OS
components in a single address space (SAS) and execute at a
single privilege level (SPL), building upon language-provided
type and memory safety to realize isolation instead of hardware protection. Everything presented herein is written in
Rust and runs in the SAS/SPL environment.

Theseus follows three design principles:
P1. Require runtime-persistent bounds for all cells.
P2. Maximize the power of the language and compiler.
P3. Minimize state spill between cells.
