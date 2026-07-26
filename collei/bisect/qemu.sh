#!/usr/bin/env bash
set -E -e -u -o pipefail

# 记录一个小小的 demo ，配合现有体系，全自动二分。
function run() {
	if [[ -d /nix ]]; then
		nix-shell --command "$1"
	else
		eval "$1"
	fi
}

cd /home/martins3/data/qemu
hash=$(git rev-parse --short HEAD)

cd /home/martins3/data
test_qemu=qemu-"$hash"
if [[ ! -e $test_qemu ]]; then
	git clone qemu "$test_qemu"
fi

cd "$test_qemu"
echo "use nix" >>.envrc && direnv allow
cp /home/martins3/.dotfiles/scripts/nix/env/qemu.nix default.nix

options=(
	--prefix="$(pwd)"/install
	--disable-bpf
	--disable-werror
	--disable-docs
	--disable-numa
)
QEMU_options=" --target-list=$(uname -m)-softmmu "
for p in "${options[@]}"; do
	QEMU_options+=" $p"
done

cmd="mkdir -p build && cd build && ../configure ${QEMU_options} && make -j$(nproc) && make install"
if [[ ! -f build/qemu-system-x86_64 ]]; then
	run "$cmd"
fi
sed -i "s/\/data\/qemu-[a-z0-9]*/\/data\/$test_qemu/g" /home/martins3/data/vn/collei/scripts/bash-archive/collei-lib.sh
/home/martins3/data/vn/collei/scripts/collei.py &
sleep 20
/home/martins3/data/vn/collei/scripts/collei-action.py -a auto
if /usr/sbin/ping 10.0.12.0 -c 30; then
	/home/martins3/data/vn/collei/scripts/collei-action.py -a kill -y
	exit 1
else
	/home/martins3/data/vn/collei/scripts/collei-action.py -a kill -y
	exit 0
fi

# 配合一个使用
# git bisect start HEAD 44f28df24767
# git bisect reset
# git bisect run /home/martins3/data/vn/code/qemu/auto.sh
# git bisect log
