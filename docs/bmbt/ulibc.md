# 为 BMBT 构建一个 mini libc
基于 musl 的版本: 1e4204d522670a1d8b8ab85f1cfefa960547e8af

- malloc : https://github.com/mtrebi/memory-allocators
- printf :

根据 @UtopianFuture 之前整理的

| Header     | 处理方法                                                         | 状态                 |
|------------|------------------------------------------------------------------|----------------------|
| alloca.h   | 这个使用 gcc builtin 的                                          | :full_moon:          |
| assert.h   |                                                                  | :first_quarter_moon: |
| byteswap.h |                                                                  | :full_moon:          |
| ctype.h    | 实际上只有 greatest.h 中使用过，而且似乎只是为 reference isprint | :full_moon:          |
| errno.h    | 似乎 errno.h 只是定义了一堆 macros                               | :full_moon:          |
| execinfo.h | 只是调试，可以直接不支持                                         | :full_moon:          |
| setjmp.h   | 应该比较容易                                                     | :first_quarter_moon: |
| signal.h   | 到时候直接使用                                                   | :full_moon:          |
| stdint.h   |                                                                  | :first_quarter_moon: |
| limits.h   |                                                                  | :first_quarter_moon: |
| inttypes.h |                                                                  | :first_quarter_moon: |
| stdbool.h  |                                                                  | :full_moon:          |
| math.h     |                                                                  | :full_moon:           |
| stdio.h    |                                                                  | :new_moon:           |
| unistd.h   | 只是依赖 sleep 而已                                              | :full_moon:          |
| stdlib.h   |                                                                  | :full_moon:          |
| string.h   | 估计主要是 strcpy 和 memset 之类的                               | :first_quarter_moon: |
| time.h     |                                                                  | :first_quarter_moon: |

## stdint / limits.h / inttypes.h / stdbool
- [ ] 为什么需要搞出来这些奇奇怪怪的 types
- [ ] limits.h 中的
- [ ] 据说 stdint 是最近才引入的 c11 标准啊

```c
/*
➜  bmbt git:(uefi-app) git grep -e "stdint.h"
include/fpu/softfloat-types.h:#include <stdint.h>
include/hw/rtc/mc146818rtc_regs.h:#include <stdint.h>
include/qemu/host-utils.h:#include <stdint.h>
include/qemu/osdep.h:#include <stdint.h>
include/types.h:#include <stdint.h>
pc-bios/linuxboot_dma.c:#include <stdint.h>
pc-bios/optrom.h:#include <stdint.h>
src/qemu/bootdevice.c:#include <stdint.h>
src/tcg/tcg.h:#include <stdint.h>
```
主要定义 int32_t 之类的操作

```c
/*
➜  bmbt git:(uefi-app) git grep -e "limits.h"
glib/glibconfig.h:#include <limits.h>
glib/gtestutils.h:#include <limits.h>
glib/gtree.h:#include <limits.h>
include/qemu/bitops.h:#include <limits.h>
include/qemu/host-utils.h:#include <limits.h>
```

```c
/*
include/exec/hwaddr.h:#include <inttypes.h>
include/hw/core/cpu.h:#include <inttypes.h> // for VADDR_PRIx
src/i386/LATX/include/types.h:#include <inttypes.h>
src/util/qemu-option.c:#include <inttypes.h>
```
主要是定义
```c
#define PRId8  "d"
#define PRId16 "d"
#define PRId32 "d"
```

## stdlib.h
似乎只是 capstone 适应过 qsort 的:
- AppPkg/Applications/bmbt/capstone/arch/X86/qsort.h

## math.h
```c
/*
src/fpu/softfloat.c:// #include <math.h>
src/i386/LATX/translator/tr_farith.c:#include <math.h>
src/i386/LATX/translator/tr_fldst.c:#include <math.h>
src/i386/bpt_helper.c:#include <math.h>
src/i386/excp_helper.c:#include <math.h>
src/i386/fpu_helper.c:#include <math.h>
src/i386/int_helper.c:#include <math.h>
src/i386/misc_helper.c:#include <math.h>
src/i386/svm_helper.c:#include <math.h>
src/util/qdist.c:#include <math.h>
```

## stdio.h
需要使用

```c
/*
➜  bmbt git:(uefi-app) git grep -e "stdio.h"
env/uefi/test-libc.c:#include <stdio.h>
env/uefi/uefi-timer.c:#include <stdio.h>
env/userspace/signal-timer.c:#include <stdio.h>
env/userspace/test-timer.c:#include <stdio.h>

include/chardev/char.h:#include <stdio.h>
include/types.h:#include <stdio.h> // for EOF
include/unitest/greatest.h:#include <stdio.h>
script/snippt.c:#include <stdio.h>
src/i386/LATX/error.c:#include <stdio.h>
src/i386/LATX/include/error.h:#include <stdio.h>
src/i386/LATX/ir1/ir1.c:#include <stdio.h>
src/i386/LATX/memwatch.c:#include <stdio.h>
src/i386/cpu.c:#include <stdio.h>
src/qemu/memory.c:#include <stdio.h>
src/tcg/cpu-exec.c:#include <stdio.h>
src/util/qemu-error.c:#include <stdio.h>
src/util/qemu-option.c:#include <stdio.h>
```
