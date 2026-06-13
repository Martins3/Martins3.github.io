#!/usr/bin/env bash
set -E -e -u -o pipefail

cd "$(dirname "$0")"

function help() {
	echo "irq -c 指定 CPU"
}

cpu=0
while getopts "hc:" opt; do
	case $opt in
		c) cpu=${OPTARG} ;;
		h) help ;;
		*) help ;;
	esac
done
shift $((OPTIND - 1))
file=/proc/interrupts
# file=$HOME/core/vn/code/trace/irq.txt

# 如果是第一行，那么使用
# awk -F ' ' "FNR==1 {printf \"\t%s\n\" , \$$((cpu)) }" "$file"
# awk -F ' ' "FNR!=1 {printf \"%s\t%s\n\" , \$1, \$$((cpu + 1)) }" "$file"
#
# printf "\t%s\n" CPU$cpu
# awk -F ' ' "FNR!=1 {for (i = 1; i <= NF ; i++) {printf \"%s\t\", \$i}; printf \"\n\"}" "$file"

cpu_num=$(awk -F ' ' '{print NF; exit}' "$file")
if [[ $cpu -gt $cpu_num ]]; then
	echo "out of range : $cpu > $cpu_num 🤣"
	exit 0
fi
awk -v cpu_col="$cpu" -v cpu_num="${cpu_num}" -f irq.awk "$file"
