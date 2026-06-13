# mtrr

- [ ] 在 docs/qemu/seabios.md 遇到过

https://wiki.gentoo.org/wiki/MTRR_and_PAT

cat /proc/mtrr

cat /proc/partitions


原来是做这个的

cat /sys/kernel/debug/x86/pat_memtype_list
