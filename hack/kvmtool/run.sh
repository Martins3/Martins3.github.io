#!/usr/bin/env bash

set -eux
kvmtool_dir=~/core/kvmtool
kernel_dir=~/core/ubuntu-linux
img=/home/maritns3/core/linux-kernel-labs/tools/labs/core-image-minimal-qemux86.ext4

kvmtool=${kvmtool_dir}/lkvm
kernel_img=${kernel_dir}/arch/x86/boot/bzImage
log_file=${kvmtool_dir}/debug.txt

DEBUG_KVMTOOL=false
while getopts "d" opt; do
  case $opt in
  d) DEBUG_KVMTOOL=true ;;
  *) exit 0 ;;
  esac
done

cmd="${kvmtool} run -k ${kernel_img} -d ${img} -m 5000 2> ${log_file}"

if [ $DEBUG_KVMTOOL = true ]; then
  debug="gdb --args ${cmd}"
  eval "$debug"
  exit 0
fi

eval "$cmd"

for ((i = 0; i < 10; i++)); do
  echo $i
done
foomatic-rip

# 如果不希望发生变量替换，可以把 Here 文档的开始标记放在单引号之中。
# << '_EOF_'
# text
# _EOF_

# 将 heredoc 导入到文档中
# cat << _EOF_ > /tmp/yourfile
# text
# _EOF_

# 此外 here string
# $ cat <<< 'hi there'
# 等同于
# $ echo 'hi there' | cat
