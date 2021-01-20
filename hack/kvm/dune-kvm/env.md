# 环境搭建

1. 不能使用计算所网络，否则无法 ssh 到 4000 上
2. `systemctl start sshd` 之后，才可以被 ssh 链接上
   - 安装的方法 : camile git:(master) ✗ sudo apt-get update && sudo apt-get install openssh-server
```c
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

## compile_commands.json
1. 使用 compile_commands.json 生成方法
➜  linux git:(master) ✗ scripts/clang-tools/gen_compile_commands.py
2. 只有最新版的 ccls 才可以正确工作
3. 当 make tags 也可以使用索引

## 编译器下载
http://www.loongnix.org/index.php/Cross-compile
