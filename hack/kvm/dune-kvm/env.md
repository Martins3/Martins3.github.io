# 交叉编译器 和 内核 的获取
- 龙芯官网
- gitlab : 4.19 , commit-id 46508b5ca3b3c7523993c7f49bf3efa8a7cffeb5 

# 环境搭建
1. 不能使用计算所网络，否则无法 ssh 到 4000 上
2. `systemctl start sshd` 之后，才可以被 ssh 链接上
   - 安装的方法 : camile git:(master) ✗ sudo apt-get update && sudo apt-get install openssh-server
```
➜  ~ ssh loongson@10.90.50.133
ssh: connect to host 10.90.50.133 port 22: Connection refused
```

## 交叉编译的方法
交叉编译的方法:

- env.sh
- make -j10 ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE
- rup='rsync -avz /home/maritns3/core/loongson-dune/cross loongson@10.90.50.133:/home/loongson/loongson' 在 loongson 上

- sudo make install / sudo make modules_install
- [ ]  (第一次需要 ?) grub2-mkconfig -o /boot/efi/EFI/BOOT/grub.cfg
    - 在使用 kvmtool 的时候，这个问题将会被解决掉
- /boot/efi/EFI/BOOT/grub.cfg 添加一个 menuentry, 全部照抄，包括 initramfs，内核指向各个编译的 /boot/the-kernel-name

交叉编译 Linux module 的方法 : https://stackoverflow.com/questions/3467850/cross-compiling-a-kernel-module
因为用户态程序是直接使用 x86 的编译器，从而可以 indexing, 而内核模块交叉编译，从而可以掌握全部的脚本。

```sh
NOTES_DIR=camile
alias mipsenv='cat /home/maritns3/core/${NOTES_DIR}/hack/compile/script/mips-env.sh && source /home/maritns3/core/${NOTES_DIR}/hack/compile/script/mips-env.sh'
alias armenv='cat /home/maritns3/core/xx/hack/compile/script/arm-env.sh && source /home/maritns3/core/xx/hack/compile/script/arm-env.sh'
alias riscvenv='cat /home/maritns3/core/mi/hack/compile/script/riscv-env.sh && source /home/maritns3/core/mi/hack/compile/script/riscv-env.sh'
```
具体文件在这个仓库中间

## compile_commands.json
1. 使用 compile_commands.json 生成方法
➜  linux git:(master) ✗ scripts/clang-tools/gen_compile_commands.py
2. 只有最新版的 ccls 才可以正确工作
3. 当 make tags 也可以使用索引

## 编译器下载
http://www.loongnix.org/index.php/Cross-compile


## 4000 的工作环境搭建
1. oh-my-zsh

2. https://github.com/skywind3000/z.lua
```sh
sudo yum install lua
eval "$(lua /path/to/z.lua --init zsh)"
```

## 同步

#### rsync
首先在 x86 内核上进行编译模块
```
mipsenv && make -j10 ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE
```

在 MIPS 上让其同步过来:
```
➜  ~ sudo rmmod kvm && synckernel && sudo insmod ~/cross/arch/mips/kvm/kvm.ko
```

rsync 举例子
```
rsync --delete -avzh --exclude='/.git' --filter="dir-merge,- .gitignore" maritns3@10.90.50.149:/home/maritns3/core/loongson-dune/3day/ /home/loongson/dune/
```

#### sshfs
```
sudo apt install sshfs
mkdir 4000
sshfs loongson@10.90.50.133:/home/loongson 4000
```

这种方法，编译内核的方式相同，但是内核模块的 install 只需要将两步

```
➜  ~ sudo rmmod kvm && sudo insmod ~/cross/arch/mips/kvm/kvm.ko
```
这其实有个问题，在 x86 上编译内核太慢，在 MIPS 上编译，应该是无法正常生成 compile_commands.json

但是对于小项目，这是无所谓的。


似乎 ltp 这种项目，有时候存在 git 和 zsh 的交火, 使用：
```
git config --add oh-my-zsh.hide-status 1
git config --add oh-my-zsh.hide-dirty 1
```
