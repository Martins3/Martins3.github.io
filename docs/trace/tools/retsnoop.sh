#!/usr/bin/env bash
set -E -e -u -o pipefail

# 这个包暂时只有 fedora 上才有的
# https://github.com/anakryiko/retsnoop
if [[ ! -f $"$HOME"/.local/bin/retsnoop ]]; then
	version=v0.10.1
	pushd /tmp
	arch=arm64
	if [[ $(uname -m) == x86_64 ]]; then
		arch=amd64
	fi
	wget https://github.com/anakryiko/retsnoop/releases/download/$version/retsnoop-${version}-$arch.tar.gz
	tar -xvf retsnoop-${version}-amd64.tar.gz
	mv retsnoop "$HOME"/.local/bin
	chmod +x "$HOME"/.local/bin/retsnoop
	popd
fi

# 将用过的方法先放到这里吧
# sudo retsnoop -e '*sys_madvise*' -a '*madvise*'
# 22:31:34.803878 -> 22:31:34.803884 TID/PID 278330/278330 (userfault.out/userfault.out):
#
#      5us [-EINVAL]  __x64_sys_madvise+0x2b
# !    0us [-EINVAL]  do_madvise
