#!/usr/bin/env bash

set -e

DIR=/home/maritns3/arch/
cd $DIR || exit 1
WORKDIR=$DIR/uftrace
if [[ ! -d $WORKDIR ]]; then
  git clone https://github.com/namhyung/uftrace
  cd uftrace
  ./configure
  make -j
  sudo make install
fi
# 仅仅显示用户态程序的调用图
uftrace ./program.out
# 同时显示内核的调用图
sudo uftrace -k ./program.out
