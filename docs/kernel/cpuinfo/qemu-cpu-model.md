# 分析下 QEMU cpu model 的


> KVM 需要小心的配置 large pages, CPU model, vCPU pinning, and cache topology 等才可以发挥其全部的性能。

-  virsh cpu-models x86_64
  - 应该展示的 qemu 支持的 x86_64 的所有的 model

```txt L1d cache:                       992 KiB
L1i cache:                       992 KiB
L2 cache:                        124 MiB
L3 cache:                        16 MiB
NUMA node0 CPU(s):               0-30
```

```txt
Caches (sum of all):
  L1d:                   896 KiB (24 instances)
  L1i:                   1.3 MiB (24 instances)
  L2:                    32 MiB (12 instances)
  L3:                    36 MiB (1 instance)
```
