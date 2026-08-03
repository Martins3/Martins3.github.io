#!/usr/bin/env bash
set -E -e -u -o pipefail
# Kconfig 的逆天实现，解析 ./config.h 中内容，
# 然后来自动修改 Makefile 中的内容

readarray -t objects < <(
	grep '^#define CONFIG_TEST_[A-Z0-9_]* 1' config.h \
		| grep -o "CONFIG_TEST_[A-Z0-9_]*"
)

all_objects=()

for i in "${objects[@]}"; do
	object=$(echo "${i##CONFIG_TEST_}" | tr '[:upper:]' '[:lower:]')
	src_path=$(find . -not -path '*/.*' -name "$object.c")
	object_path=${src_path/.c/.o}
	if [[ $object_path == ./arch/* ]]; then
		if [[ $object_path != ./arch/$(uname -m)/* ]]; then
			continue
		fi
	fi

	if [[ $object_path == ./arch/x86_64/asm.o ]]; then
		all_objects+=(arch/x86_64/asm_add.o)
	fi
	# TODO 不知道为什么，这里必须去掉 ./ ，例如把 ./sched/preempt.o 变为
	# sched/preempt.o ，不然存在这个报错
	# ERROR: modpost: "__SCK__cond_resched" [martins3.ko] undefined!
	# ERROR: modpost: "__SCK__might_resched" [martins3.ko] undefined!
	# ERROR: modpost: "__SCK__preempt_schedule" [martins3.ko] undefined!
	#
	# echo "${object_path}"
	all_objects+=("${object_path##./}")
done

# 默认情况下 echo arr 是输出 arr 的第一个字符
files=$(printf '%s ' "${all_objects[@]}")
# 询问 deepseek 的，只能说技巧性很强
sed -i "s|^OBJECTS :=.*|OBJECTS := $files|" Makefile
