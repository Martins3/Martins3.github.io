#!/usr/bin/env bash

DIR=/home/maritns3/arch/
cd $DIR || exit 1
WORKDIR=$DIR/flamescope
if [[ ! -d $WORKDIR ]]; then
  git clone https://github.com/Netflix/flamescope
  cd flamescope || exit 1
  pip3 install -r requirements.txt
  python3 run.py
fi

perf record -a -g /home/maritns3/core/kvmqemu/build/x86_64-softmmu/qemu-system-x86_64 -kernel /home/maritns3/core/ubuntu-linux/arch/x86/boot/bzImage
perf script --header > $WORKDIR/examples/stacks.myproductionapp.2018-03-30_01.gz
# gzip stacks.myproductionapp.2018-03-30_01 # optional
# 127.0.0.1:5000
