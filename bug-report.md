# 复现 LOONGARCH QEMU 无法正确执行

## QEMU version
```diff
commit 51590e93b9511f53b402bf911a6f3378c2299756 (HEAD -> loongisa, origin/loongisa, origin/HEAD)
Author: Zeng Lu <zenglu@loongson.cn>
Date:   Fri Mar 19 18:41:20 2021 +0800

    LoongArch: Add csr debug flag

    Signed-off-by: Zeng Lu <zenglu@loongson.cn>
```

## Kernel version
```diff
commit d5497d72b0893d587dfe08b4162b9cb6e32e0555 (tag: show, tag: list, tag: 4.19.167-5, tag: 0h)
Author: Xuefeng Li <lixuefeng@loongson.cn>
Date:   Thu Apr 15 19:53:58 2021 +0800

    Loongson: 4.19.167-rc5

    Change-Id: I910967bb16c5c5ecbcb9ab7930a91171644c44fa
    Signed-off-by: Xuefeng Li <lixuefeng@loongson.cn>
```
注: 这个 kernel 就是我右边物理机正在使用的 kernel

## compile
QEMU 编译方法
```bash
mkdir build
cd build
../configure --target-list=loongson-softmmu  --disable-werror
```

内核编译
1. 添加上 patch
```diff
diff --git a/include/linux/cpufreq.h b/include/linux/cpufreq.h
index 3361663144a1..f8b4c4a35e00 100644
--- a/include/linux/cpufreq.h
+++ b/include/linux/cpufreq.h
@@ -210,6 +210,12 @@ static inline unsigned int cpufreq_quick_get_max(unsigned int cpu)
 	return 0;
 }
 static inline void disable_cpufreq(void) { }
+static inline int cpufreq_get_policy(struct cpufreq_policy *policy, unsigned int cpu) { return 0;}
+static inline void cpufreq_update_policy(unsigned int cpu) {};
+static inline bool have_governor_per_policy(void) { return false;}
+static inline struct kobject *get_governor_parent_kobj(struct cpufreq_policy *policy) { return NULL; }
+static inline void cpufreq_enable_fast_switch(struct cpufreq_policy *policy) {}
+static inline void cpufreq_disable_fast_switch(struct cpufreq_policy *policy) {}
 #endif
 
 #ifdef CONFIG_CPU_FREQ_STAT
```
2. make 

## 测试

执行脚本
```bash
kernel=/home/loongson/native-kernel/vmlinux
qemu=/home/loongson/ld/qemu/build/loongson-softmmu/qemu-system-loongson
gcc -static -o init test.c
disk_img=test.cpio.gz
echo init | cpio -o -H newc | gzip >${disk_img}
${qemu}  -M ls3a5k -m 2048 -kernel ${kernel} -initrd ${disk_img} -smp 1
```

log 
```
mips_ls3a7a_init: num_nodes 1
mips_ls3a7a_init: node 0 mem 0x80000000
*****zl 1, mask0
memory_offset = 0x78;cpu_offset = 0xc88; system_offset = 0xce8; irq_offset = 0x3058; interface_offset = 0x30b8;
boot_params_buf is
param len=0x8a30
env 980000000f000060
VNC server running on ::1:5900
./run.sh：行 19:  6693 非法指令            ${qemu} -M ls3a5k -m 2048 -kernel ${kernel} -initrd ${disk_img} -smp 1
```

## 尝试
1. qemu 无论是在 5000 还是在我的 x86 上都立刻挂掉，只是 x86 上是 segment fault 报错而已
2. 兰彦志可以运行 qemu kernel 拷贝我的机器上，其 qemu 依赖的动态库和我的 x86 版本不一致, 放弃。使用兰彦志的镜像，QEMU 会死循环。
3. x86 上， 使用 gdb 分析，日志结果如下
```
>>> bt
#0  0x00005555557b6ef6 in test_bit (addr=0x7fffe8e4eed0, nr=7905747460161236391) at /home/maritns3/core/ld/qemu/include/qemu/bitops.h:135
#1  init_ts_info (infos=infos@entry=0x7fffb400ce10, temps_used=temps_used@entry=0x7fffe8e4eed0, ts=ts@entry=0x7fffb4000b60) at /home/maritns3/core/ld/qemu/tcg/optimize.c:96
#2  0x00005555557b8434 in tcg_optimize (s=s@entry=0x7fffb4000b60) at /home/maritns3/core/ld/qemu/tcg/optimize.c:630
#3  0x000055555579e545 in tcg_gen_code (s=0x7fffb4000b60, tb=tb@entry=0x7fffbc000a40 <code_gen_buffer+2579>) at /home/maritns3/core/ld/qemu/tcg/tcg.c:4074
#4  0x0000555555808527 in tb_gen_code (cpu=cpu@entry=0x55555663cf50, pc=10376293541477190156, cs_base=cs_base@entry=0, flags=268435608, cflags=-16252928, cflags@entry=524288) at /home/maritns3/core/ld/qemu/accel/tcg/translate-all.c:1757
#5  0x00005555558066a2 in tb_find (cf_mask=524288, tb_exit=0, last_tb=0x0, cpu=0x0) at /home/maritns3/core/ld/qemu/accel/tcg/cpu-exec.c:406
#6  cpu_exec (cpu=cpu@entry=0x55555663cf50) at /home/maritns3/core/ld/qemu/accel/tcg/cpu-exec.c:730
#7  0x00005555557d2410 in tcg_cpu_exec (cpu=0x55555663cf50) at /home/maritns3/core/ld/qemu/cpus.c:1473
#8  0x00005555557d46c4 in qemu_tcg_cpu_thread_fn (arg=arg@entry=0x55555663cf50) at /home/maritns3/core/ld/qemu/cpus.c:1781
#9  0x0000555555ca71d3 in qemu_thread_start (args=<optimized out>) at /home/maritns3/core/ld/qemu/util/qemu-thread-posix.c:519
#10 0x00007ffff5ce7609 in start_thread (arg=<optimized out>) at pthread_create.c:477
#11 0x00007ffff5c0e293 in clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:95

>>> p/x addr
$1 = 0x7fffe8e4eed0
>>> p/x nr
$2 = 0x6db6db6db6db6da7
```
出错原因是 test_bit 是 nr 是一个错误的参数，后续没有跟进。
```c
static inline int test_bit(long nr, const unsigned long *addr)
{
     return 1UL & (addr[BIT_WORD(nr)] >> (nr & (BITS_PER_LONG-1)));
}
```
