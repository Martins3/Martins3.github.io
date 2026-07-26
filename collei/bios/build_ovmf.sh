#!/usr/bin/env bash
set -E -e -u -o pipefail

# 添加我们自己写的一个 uefi 程序
if [[ ! -e BootLoaderPkg ]]; then
	ln -s "$HOME"/data/vn/code/src/BootLoaderPkg BootLoaderPkg
fi

#
if [[ ! -e BaseTools/BuildEnv ]]; then
	make -C BaseTools
fi

# 执行完成之后，才有 Conf/target.txt
# shellcheck disable=1091
source edksetup.sh

ARCH=$(uname -m)

# 默认为 vs2022 ，需要修改一下
sed -i -E "s/^TOOL_CHAIN_TAG.*/TOOL_CHAIN_TAG=GCC/" Conf/target.txt

if [[ $ARCH == x86_64 ]]; then
	sed -i -E "s/^TARGET_ARCH.*/TARGET_ARCH=X64/" Conf/target.txt
	sed -i -E 's/^ACTIVE_PLATFORM.*/ACTIVE_PLATFORM=OvmfPkg\/OvmfPkgX64.dsc/' Conf/target.txt
else
	sed -i -E "s/^TARGET_ARCH.*/TARGET_ARCH=AARCH64/" Conf/target.txt
	sed -i -E 's/^ACTIVE_PLATFORM.*/ACTIVE_PLATFORM=ArmVirtPkg\/ArmVirtQemu.dsc/' Conf/target.txt
fi

# 默认用所有的核心
# sed -i -E "s/^# MAX_CONCURRENT_THREAD_NUMBER = 1/MAX_CONCURRENT_THREAD_NUMBER=32/" Conf/target.txt

# 似乎配置两个参数才会有 compile_commands
build -Y COMPILE_INFO -y BuildReport.log

# 这里仅仅是编译 ovmf ，参考 docs/uefi/edk2/build.md 进行各种灵活的测试
# 之后可以直接进入到命令行，执行
#
# 之后继续调试的话，只是需要:
# source edksetup.sh
# build
