#!/bin/sh
CC_PREFIX=~/Downloads/cross-gcc-4.9.3-n64-loongson-rc6.1

export ARCH=mips
export CROSS_COMPILE=mips64el-loongson-linux-
export PATH=$CC_PREFIX/usr/bin:$PATH
export LD_LIBRARY_PATH=$CC_PREFIX/usr/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$CC_PREFIX/usr/x86_64-unknown-linux-gnu/mips64el-loongson-linux/lib/:$LD_LIBRARY_PATH
