# 为 BMBT 构建一个 mini libc
基于 musl 的版本: 1e4204d522670a1d8b8ab85f1cfefa960547e8af

- malloc : https://github.com/mtrebi/memory-allocators
- printf : 这个容易

根据 @UtopianFuture 之前整理的 bmbt 依赖的 libc，简单的分析了一下:

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
| math.h     |                                                                  | :full_moon:          |
| stdio.h    |                                                                  | :full_moon:           |
| unistd.h   | 只是依赖 sleep 而已                                              | :full_moon:          |
| stdlib.h   |                                                                  | :full_moon:          |
| string.h   | 估计主要是 strcpy 和 memset 之类的                               | :first_quarter_moon: |
| time.h     |                                                                  | :first_quarter_moon: |

## stdint / limits.h / inttypes.h / stdbool
musl 为什么需要动态的生成 bits/alltypes.h

```c
obj/include/bits/alltypes.h: $(srcdir)/arch/$(ARCH)/bits/alltypes.h.in $(srcdir)/include/alltypes.h.in $(srcdir)/tools/mkalltypes.sed
	sed -f $(srcdir)/tools/mkalltypes.sed $(srcdir)/arch/$(ARCH)/bits/alltypes.h.in $(srcdir)/include/alltypes.h.in > $@
```

- [ ] 似乎在 float_t 上的确有东西
- [ ] FILE 这个东西是不是定义的有点太随意了啊
- [ ] why NULL is defined every where，虽然很容易处理，但是无法理解

- [ ] 我希望整个 glibc 可以让用户态程序编译出来
  - [ ] 应该是有办法不使用用户态的 lib 然后只是使用系统态的
- [x] 为什么 assert.h 没有 once 来保护
- [ ] 现在 bits 下全部是按照 x86 的，除了 limits.h
  - 其中的 float 相关的需要重点关注一下
  - [ ] 需要仔细的比较一下在 types 上, la 和 x86 的实现的差别
- 似乎只是 capstone 使用过 qsort
  - AppPkg/Applications/bmbt/capstone/arch/X86/qsort.h
- [ ] 似乎只是需要补充几个函数，就可以让程序在用户态编译起来

## 为了处理 env
1. 因为 UEFI 在 MdePkg 中定义了 NULL 的，而 NULL 在很多头文件中都是定义了，所以在 `env/*/uaip/libc.h` 中处理 NULL。
2. 因为 UEFI 的 StdLib 的实现的差别，需要使用 USE_UEFI_LIBC 来构建其中的差别。
