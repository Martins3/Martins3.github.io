#!/usr/bin/env bash
set -E -e -u -o pipefail

set -x
function create_disk() {
	local dir=$1
	local num
	local format=qcow2
	local size=350
	local disk_opt="$dir/opt/disk"
	num=$(gum choose 1 3)
	if [[ $num == 1 ]]; then
		if gum confirm --default=No "raw boot image"; then
			format=raw
			size=40
		fi
	fi

	{
		echo "# supporte type :"
		echo "#		nvme"
		echo "# 	ide"
		echo "# 	virtio-blk"
		echo "# 	virtio-scsi"
	} >"$disk_opt"
	for ((i = 1; i <= num; i = i + 1)); do
		printf '%s\n' "$i"
		local path=${dir}/img/boot${i}
		if [[ ! -f $path ]]; then
			mkdir -p "${dir}"/img/
			qemu-img create -f "$format" "$path" "${size}"G
		fi
		echo "boot${i} virtio-blk $i" >>"$disk_opt"
	done
}

function add_disk() {
	local dir=$1
	local format=qcow2
	local size=350
	local disk_opt="$dir/opt/disk"

	local max=0
	for d in "$dir"/img/boot*[0-9]; do
		[ -e "$d" ] || continue
		num=${d##*boot}
		if [[ $num =~ ^[0-9]+$ ]] && ((num > max)); then
			max=$num
		fi
	done
	next=$((max + 1))

	local path=${dir}/img/boot${next}

	qemu-img create -f "$format" "$path" "${size}"G
	echo "boot${next} virtio-blk" >>"$disk_opt"
}

cmd=""
while getopts "hc:" opt; do
	case $opt in
		c) cmd=${OPTARG} ;;
		h)
			echo "-c create"
			echo "-m add one disk"
			;;
		*)
			exit 1
			;;
	esac
done
shift $((OPTIND - 1))

case "$cmd" in
	create)
		create_disk "$1"
		;;
	add)
		add_disk "$1"
		;;
	*)
		echo "collei/scripts/collei-disk.sh doesn't work"
		;;
esac
