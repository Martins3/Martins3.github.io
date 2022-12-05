# X86 上阅读 Loongarch 内核

## 背景
X86 机器上阅读 X86 内核的配置可以参考[我之前的文章](https://github.com/Martins3/My-Linux-config)

下面说明一下阅读 Loongarch 内核的一些调整。

阅读内核我喜欢 ccls + coc.nvim 的机制，lsp 的机制都是需要 compile_commands.json 的生成。
正确的编译内核，然后使用内核中下面的脚本即可。
```c
➜  linux git:(master) scripts/clang-tools/gen_compile_commands.py
```
但是阅读 Loongarch 的内核时候，ccls 很快就会停止工作，最后整个项目也没有办法索引。

## 解决方法
1. 将 ccls 单独拿出来分析
```sh
 ~/arch/ccls/Release/ccls --index=. -v=2 --init='{"index":{"threads":1}}'
```
可以看到 ccls 对于几乎所有的文件都是直接跳过的

2. 分析源码:

在 `indexer_Parse` 中, 可以发现调用 idx::index 的返回值 `ok` 总是等于 false
```cpp
    auto result =
        idx::index(completion, wfiles, vfs, entry.directory, path_to_index,
                   entry.args, remapped, no_linkage, ok);
    indexes = std::move(result.indexes);
    n_errs = result.n_errs;
    first_error = std::move(result.first_error);

    if (!ok) {
      if (request.id.valid()) {
        ResponseError err;
        err.code = ErrorCode::InternalError;
        err.message = "failed to index " + path_to_index;
        pipeline::replyError(request.id, err);
      }
      return true;
    }
```

在分析这个 `index` 函数，发现调用 `TargetInfo::CreateTargetInfo` 总是只能得到 nullptr，而分析 LLVM 的源码
在`llvm-project/clang/lib/Basic/Targets.cpp` 的进行了一系列的 cpu, abi 之类的检查。

再看一个 mips64r2 的内核编译参数:
```json
  {
    "command": "mips64el-loongson-linux-gcc -Wp,-MD,block/partitions/.msdos.o.d  -nostdinc -isystem /home/maritns3/Downloads/cross-gcc-4.9.3-n64-loongson-rc6.1/usr/bin/../lib/gcc/mips64el-loongson-linux/4.9.3/include -I./arch/mips/include -I./arch/mips/include/generated  -I./include -I./arch/mips/include/uapi -I./arch/mips/include/generated/uapi -I./include/uapi -I./include/generated/uapi -include ./include/linux/kconfig.h -include ./include/linux/compiler_types.h -D__KERNEL__ -DVMLINUX_LOAD_ADDRESS=0xffffffff80200000 -DDATAOFFSET=0 -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -fshort-wchar -Werror-implicit-function-declaration -Wno-format-security -std=gnu89 -fno-PIE -mno-check-zero-division -mabi=64 -G 0 -mno-abicalls -fno-pic -pipe -msoft-float -DGAS_HAS_SET_HARDFLOAT -Wa,-msoft-float -ffreestanding -fno-stack-check -DTOOLCHAIN_SUPPORTS_VIRT -Wa,--trap -Wa,-mno-fix-loongson3-llsc -march=mips64r2 -U_MIPS_ISA -D_MIPS_ISA=_MIPS_ISA_MIPS64 -I./arch/mips/include/asm/mach-loongson64 -mno-branch-likely -I./arch/mips/include/asm/mach-generic -msym32 -DKBUILD_64BIT_SYM32 -fno-asynchronous-unwind-tables -fno-delete-null-pointer-checks -O2 --param=allow-store-data-races=0 -Wframe-larger-than=1024 -fstack-protector-strong -Wno-unused-but-set-variable -fomit-frame-pointer -fno-var-tracking-assignments -g -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fno-merge-all-constants -fmerge-constants -fno-stack-check -fconserve-stack -Werror=implicit-int -Werror=strict-prototypes -Werror=date-time    -DKBUILD_BASENAME='\"msdos\"' -DKBUILD_MODNAME='\"msdos\"' -c -o block/partitions/.tmp_msdos.o block/partitions/msdos.c",
    "directory": "/home/maritns3/core/loongson-dune/cross",
    "file": "/home/maritns3/core/loongson-dune/cross/block/partitions/msdos.c"
  }
```
将其中 mips64el-loongson-linux-gcc 和 -mabi=64 替换掉 Loongarch 的内核 compile_commands.json 中对应的位置即可。

大功告成。

## 补充
阅读 Loongarch Qemu 的方法:
1. 在 Loongarch 机器上编译生成 compile_commands.json
```sh
mkdir build
cd build
../configure --target-list=loongson-softmmu --disable-werror
bear make -j10
```
2. 将 compile_commands.json 拷贝到 intel 机器上
3. 替换 "cc" 为 "mips64el-linux-gnu-gcc"
4. 替换路径

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
