#!/usr/bin/env bash
set -E -e -u -o pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
pushd "$SCRIPT_DIR" >/dev/null
# shellcheck source=code/const
source ../../code/const
# shellcheck source=code/lib.sh
source ../../code/lib.sh

popd >/dev/null
set -x

mkdir -p "$BIOS_SOURCE_WORKDIR"
cd "$BIOS_SOURCE_WORKDIR"
ARCH=$(uname -m)

function seabios() {
	if [[ -f seabios/out/bios.bin ]]; then
		return
	fi
	if [[ ! -d seabios ]]; then
		git clone https://github.com/coreboot/seabios
	fi
	cd seabios
	make -j"$(nproc)"
}

function qboot() {
	if [[ ! -f qboot/build/bios.bin ]]; then
		cd ~/core
		if [[ ! -d qboot ]]; then
			git clone https://github.com/bonzini/qboot.git
		fi
		cd qboot
		meson setup --reconfigure build
		cd build
		ninja
	fi

}

function ovmf_binary() {
	if [[ -d ovmf_binary ]]; then
		return
	fi

	# 猜测 pflash 的 raw 和 fd 只是使用不同的参数而已
	#
	# 服气，一个叫 QEMU_EFI 一个叫 OVMF
	mkdir ovmf_binary
	pushd ovmf_binary
	if [[ $ARCH == aarch64 ]]; then
		# ./usr/share/edk2/aarch64/QEMU_EFI-pflash.raw
		# ./usr/share/edk2/aarch64/QEMU_EFI.fd
		# ./usr/share/edk2/aarch64/QEMU_VARS.fd
		# ./usr/share/edk2/aarch64/vars-template-pflash.raw
		wget https://mirrors.aliyun.com/openeuler/openEuler-22.03-LTS/everything/aarch64/Packages/edk2-aarch64-202011-3.oe2203.noarch.rpm
		rpm2cpio edk2-aarch64-202011-3.oe2203.noarch.rpm | cpio -idmv
	elif [[ $ARCH == x86_64 ]]; then
		# ./usr/share/edk2
		# ./usr/share/edk2/ovmf
		# ./usr/share/edk2/ovmf/OVMF.fd
		# ./usr/share/edk2/ovmf/OVMF_CODE.fd
		# ./usr/share/edk2/ovmf/OVMF_VARS.fd
		# ./usr/share/licenses/edk2-ovmf
		# ./usr/share/licenses/edk2-ovmf/LICENSE.openssl
		# ./usr/share/licenses/edk2-ovmf/License.txt
		wget https://mirrors.aliyun.com/openeuler/openEuler-22.03-LTS/everything/x86_64/Packages/edk2-ovmf-202011-3.oe2203.noarch.rpm
		rpm2cpio edk2-ovmf-202011-3.oe2203.noarch.rpm | cpio -idmv
	fi
	popd
	# TODO 似乎是默认关闭 smm 导致的吗?
	#
	# https://lore.kernel.org/qemu-devel/87y378n5iy.fsf@dusky.pond.sub.org/
}

function ovmf_binary_secure() {
	if [[ -d ovmf_binary_secure ]]; then
		return
	fi

	mkdir ovmf_binary_secure
	pushd ovmf_binary_secure

	if [[ $ARCH == aarch64 ]]; then
		wget https://repo.almalinux.org/almalinux/9/AppStream/aarch64/os/Packages/edk2-ovmf-20240524-6.el9_5.3.noarch.rpm
		rpm2cpio edk2-ovmf-20240524-6.el9_5.3.noarch.rpm | cpio -idmv
	elif [[ $ARCH == x86_64 ]]; then
		# └── usr
		#     └── share
		#         ├── doc
		#         │   └── edk2-ovmf
		#         │       ├── ovmf-whitepaper-c770f8c.txt
		#         │       └── README
		#         ├── edk2
		#         │   └── ovmf
		#         │       ├── DBXUpdate-20230509.x64.bin
		#         │       ├── EnrollDefaultKeys.efi
		#         │       ├── OVMF.amdsev.fd
		#         │       ├── OVMF_CODE.cc.fd -> OVMF_CODE.fd
		#         │       ├── OVMF_CODE.fd
		#         │       ├── OVMF_CODE.secboot.fd
		#         │       ├── OVMF.inteltdx.fd
		#         │       ├── OVMF.inteltdx.secboot.fd
		#         │       ├── OVMF_VARS.fd
		#         │       ├── OVMF_VARS.secboot.fd
		#         │       ├── Shell.efi
		#         │       └── UefiShell.iso
		#         ├── licenses
		#         │   └── edk2-ovmf
		#         │       ├── License-History.txt
		#         │       ├── LICENSE.openssl
		#         │       ├── License.OvmfPkg.txt
		#         │       └── License.txt
		#         ├── OVMF
		#         │   ├── OVMF_CODE.secboot.fd -> ../edk2/ovmf/OVMF_CODE.secboot.fd
		#         │   ├── OVMF_VARS.fd -> ../edk2/ovmf/OVMF_VARS.fd
		#         │   ├── OVMF_VARS.secboot.fd -> ../edk2/ovmf/OVMF_VARS.secboot.fd
		#         │   └── UefiShell.iso -> ../edk2/ovmf/UefiShell.iso
		#         └── qemu
		#             └── firmware
		#                 ├── 30-edk2-ovmf-x64-sb-enrolled.json
		#                 ├── 40-edk2-ovmf-x64-sb.json
		#                 ├── 50-edk2-ovmf-x64-nosb.json
		#                 ├── 60-edk2-ovmf-x64-amdsev.json
		#                 └── 60-edk2-ovmf-x64-inteltdx.json
		#
		wget https://repo.almalinux.org/almalinux/9/AppStream/x86_64/os/Packages/edk2-ovmf-20241117-4.el9.noarch.rpm
		rpm2cpio edk2-ovmf-20241117-4.el9.noarch.rpm | cpio -idmv

	fi
	popd

}

function uboot() {
	if [[ ! -d u-boot ]]; then
		git clone https://github.com/u-boot/u-boot
	fi
	pushd u-boot
	if [[ ! -e default.nix ]]; then
		ln -s ~/.dotfiles/scripts/nix/env/uboot.nix default.nix
		echo "use nix" >>.envrc && direnv allow
		python -m venv .venv
	fi
	build_arch=$(uname -m | sed 's/x86_64/x86/' \
		| sed 's/aarch64/arm64/')
	cat <<_EOF_
		make qemu_${build_arch}_defconfig
		make all -j32
_EOF_
}

function coreboot() {
	echo "TODO"
}

function ovmf() {
	# 参考 https://github.com/tianocore/tianocore.github.io/wiki/How-to-build-OVMF
	if [[ ! -d edk2 ]]; then
		git clone https://github.com/tianocore/edk2.git
		pushd edk2
		git submodule update --init
		popd
	fi
	pushd edk2
	if [[ ! -e default.nix ]]; then
		ln -s "$HOME/.dotfiles/scripts/nix/env/edk2.nix" default.nix
	fi
	run "bash $SCRIPT_DIR/bios_build_ovmf.sh"
}

supported_bios=(
	seabios
	qboot
	ovmf_binary
	ovmf_binary_secure
	ovmf
	uboot
	coreboot
)

target=${1-}
if [[ -z $target ]]; then
	target=$(printf "%s\n" "${supported_bios[@]}" | fzf)
fi
$target
