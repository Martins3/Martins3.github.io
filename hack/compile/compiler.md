# Compiler for linux kernel

## latent_entropy
- [ ] although I know how it works, but I can't run a demo
   - https://lwn.net/Articles/688492/
   - https://github.com/ephox-gcc-plugins/latent_entropy

## build kernel with clang
- https://www.kernel.org/doc/html/latest/kbuild/llvm.html
    - [ClangBuiltLinux](https://github.com/ClangBuiltLinux/tc-build)

- [使用 clang 的交叉编译](https://github.com/MaskRay/ccls/wiki/Example-Projects)
```
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- LLVM=1 defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- LLVM=1 -j10
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- LLVM=1 -k Image.gz modules // 好吧，-k Image.gz modules 是什么意思
```

也可以使用 gcc 来实现交叉编译 :
```
ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- make defconfig
ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- make
```


使用 LLVM 编译 x86
```
# Use clang 9 or newer.
make ARCH=x86_64 LLVM=1 defconfig
make ARCH=x86_64 LLVM=1 -j10
```

在最近的内核版本如果遇到问题，很有可能是
```diff
diff --git a/include/linux/compiler-clang.h b/include/linux/compiler-clang.h
index 230604e7f057..e913000f4b95 100644
--- a/include/linux/compiler-clang.h
+++ b/include/linux/compiler-clang.h
@@ -7,7 +7,7 @@
                     + __clang_minor__ * 100    \
                     + __clang_patchlevel__)

-#if CLANG_VERSION < 100001
+#if CLANG_VERSION < 100000
 # error Sorry, your version of Clang is too old - please use 10.0.1 or newer.
 #endif
```

### 对于 Loongnix 的交叉编译:
1. http://www.loongnix.org/index.php/Cross-compile 下载 gcc-4.9.3 64位
2. 执行:
```sh
CC_PREFIX=~/Downloads/cross-gcc-4.9.3-n64-loongson-rc6.1

export ARCH=mips
export CROSS_COMPILE=mips64el-loongson-linux-
export PATH=$CC_PREFIX/usr/bin:$PATH
export LD_LIBRARY_PATH=$CC_PREFIX/usr/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$CC_PREFIX/usr/x86_64-unknown-linux-gnu/mips64el-loongson-linux/lib/:$LD_LIBRARY_PATH
```
3. 并行 make 可能对于 MIPS 出现问题，那么
https://unix.stackexchange.com/questions/496500/how-to-get-php-7-1-to-use-libreadline-so-8

### Cross Compile standard MIPS
Tried, but faile to find the compiler

### Cross Compile Riscv
```
make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu-
```
https://risc-v-getting-started-guide.readthedocs.io/en/latest/linux-introduction.html

## `__attribute__((destructor))`

https://stackoverflow.com/questions/2053029/how-exactly-does-attribute-constructor-work

## `__ASSEMBLY__`

https://stackoverflow.com/questions/28924355/gcc-assembler-preprocessor-not-compatible-with-standard-headers

when reading `arch/x86/include/asm/idtentry.h`,
I can't find the caller of asm_common_interrupt.
There is only one, but ccls's highlight tell me that it will be compiled because of option `_ASSEMBLY__` not selected.
In fact, things doesn’t happened in that way.
ideentry will be include by normal C file and asm file,
when include by asm, `_ASSEMBLY__` will be selected.

## `__cacheline_aligned`

https://stackoverflow.com/questions/25947962/cacheline-aligned-in-smp-for-structure-in-the-linux-kernel

## asmlinkage
https://stackoverflow.com/questions/10459688/what-is-the-asmlinkage-modifier-meant-for

## volatile
https://gcc.gnu.org/onlinedocs/gcc/Volatiles.html#Volatiles

## [深入理解volatile关键字](https://blog.regehr.org/archives/28)
1. volative 到底实现了什么功能 ?
2. 使用 volative 的位置在什么地方 ?
3. volative 需要硬件如何支持 ? 并不需要特殊指令，但是需要编译器的支持。

The way the volatile connects the abstract and real semantics is this:
> For every read from a volatile variable by the abstract machine, the actual machine must load from the memory address corresponding to that variable.  **Also, each read may return a different value.**  For every write to a volatile variable by the abstract machine, the actual machine must store to the corresponding address.  Otherwise, the address should not be accessed (with some exceptions) and also accesses to volatiles should not be reordered (with some exceptions).

Historically, the connection between the abstract and actual machines was established mainly through accident: **compilers weren’t good enough at optimizing to create an important semantic gap.**


最终结论，不要使用volatile 而要使用锁。

> 作者是犹他大学的教授，强的一匹。

## 如何正确的切换 gcc 版本
https://badsimplicity.com/2019/07/10/gcc-9-and-ubuntu-kernel-error/

```sh
sudo apt install gcc-8 # 安装
sudo update-alternatives --install /usr/bin/gcc gcc  /usr/bin/gcc-8 1 # 添加 alternatives
sudo update-alternatives --install /usr/bin/gcc gcc  /usr/bin/gcc-8 2 # 添加 alternatives
sudo update-alternatives --config gcc # 选择

sudo update-alternatives --config cc # 设置默认 clang
sudo update-alternatives --config c++
```
类似可以选择默认的 clang 版本。

```sh
cd /usr/bin
sudo ln -f -s clang-11 clang
sudo ln -f -s clang++-11 clang++
sudo ln -f -s llvm-ar-11 llvm-ar
sudo ln -f -s llvm-as-11 llvm-as
sudo ln -f -s clangd-11  clangd
sudo ln -f -s clang-tidy-11 clang-tidy
```
