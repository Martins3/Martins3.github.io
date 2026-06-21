#!/usr/bin/env bash
set -E -e -u -o pipefail

if [[ ! -d $HOME/core/libfuse ]]; then
	git clone https://github.com/libfuse/libfuse
fi

cd "$HOME"/core/libfuse/
mkdir build
cd build
meson setup .. -D prefix="$PWD"/install -D default_library=both

# meson configure -D prefix="$PWD"/install -D default_library=both


ninja -j32
ninja install

# # martins3 fs
#
# ## 看看 proc fs
#
#
# -r--r--r--   1 root root 0 Oct 27 22:19 mounts
#
# 607 32 0:64 / /home/martins3/core/vn/code/src/mfs/demo rw,nosuid,nodev,relatime shared:512 - fuse.hello.out hello.out rw,user_id=0,group_id=0
#
# -r--------   1 root root 0 Oct 27 22:19 mountstats
#
# device hello.out mounted on /home/martins3/core/vn/code/src/mfs/demo with fstype fuse.hello.out
#
# -r--r--r--   1 root root 0 Oct 27 22:19 mountinfo
#
# 607 32 0:64 / /home/martins3/core/vn/code/src/mfs/demo rw,nosuid,nodev,relatime shared:512 - fuse.hello.out hello.out rw,user_id=0,group_id=0
#
#
# 1. fuse_daemonize : 显然，还是一个用户态的程序的
# 2. lib/fuse_loop.c 中来监听所有的
#
# lib/fuse_lowlevel.c 中 fuse_ll_ops 来处理所有的业务的行为。
#
# 所以，基本的模式就是:
# 1. 对于文件的所有的操作 ，先到 vfs ，vfs 通过 fuse 模块转发给 fuse ，最后用户态来想其中提供数据
