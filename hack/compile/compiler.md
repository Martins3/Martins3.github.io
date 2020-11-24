# Compiler for linux kernel

## latent_entropy
- [ ] although I know how it works, but I can't run a demo
   - https://lwn.net/Articles/688492/
   - https://github.com/ephox-gcc-plugins/latent_entropy

## build kernel with clang
- https://www.kernel.org/doc/html/latest/kbuild/llvm.html
    - [ClangBuiltLinux](https://github.com/ClangBuiltLinux/tc-build)

[ccls 交叉编译 提供关于 clang 的交叉编译是有问题](https://github.com/MaskRay/ccls/wiki/Example-Projects)
```
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- LLVM=1 defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- LLVM=1 -j10
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- LLVM=1 -k Image.gz modules // 好吧，-k Image.gz modules 是什么意思
```

但是还是存在版本不够的问题: https://apt.llvm.org/ 可以安装最新的 LLVM, 但是实际上也没有用。

暂时使用 gcc 来实现交叉编译 :
```c
ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- make
ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- make defconfig
```

下面测试一下 clang 编译配合 clangd 的效果:
```
# Use clang 9 or newer.
make ARCH=x86_64 LLVM=1 defconfig
make ARCH=x86_64 LLVM=1 -k bzImage modules  # generate .<target>.cmd files
```

对于 Loongnix 的交叉编译:
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

```c
cd /usr/bin
sudo ln -f -s clang-11 clang
sudo ln -f -s clang++-11 clang++
sudo ln -f -s llvm-ar-11 llvm-ar
sudo ln -f -s llvm-as-11 llvm-as
sudo ln -f -s clangd-11  clangd
sudo ln -f -s clang-tidy-11 clang-tidy
```
