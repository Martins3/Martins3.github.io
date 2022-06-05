# chipyard 环境搭建

## 尝试在 Ubuntu 20.04 中安装
```sh
#!/bin/bash

set -ex

sudo apt-get install -y build-essential bison flex software-properties-common curl
sudo apt-get install -y libgmp-dev libmpfr-dev libmpc-dev zlib1g-dev vim default-jdk default-jre
sudo apt-get install -y texinfo gengetopt
sudo apt-get install -y libexpat1-dev libusb-dev libncurses5-dev cmake
# deps for poky
sudo apt-get install -y python3.8 patch diffstat texi2html texinfo subversion chrpath wget
# deps for qemu
sudo apt-get install -y libgtk-3-dev gettext
# deps for firemarshal
sudo apt-get install -y python3-pip python3.8-dev rsync libguestfs-tools expat ctags
# install DTC
sudo apt-get install -y device-tree-compiler
sudo apt-get install -y python

# install verilator
sudo apt-get install -y autoconf
git clone http://git.veripool.org/git/verilator
cd verilator
git checkout v4.034
autoconf && ./configure && make -j$(nproc) && sudo make install
```

## questions
- [ ] chipyard 为什么是需要 QEMU 的啊
