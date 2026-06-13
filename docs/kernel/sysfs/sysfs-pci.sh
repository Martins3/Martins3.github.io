#!/usr/bin/env bash
set -E -e -u -o pipefail

for d in /sys/bus/pci/devices/*/; do
	pushd "$d" >/dev/null
	dir=$(basename "$d")
	lspci -s "$dir"
	echo "irq : $(cat irq)"
	echo "msi_bus : $(cat msi_bus)"
	if [[ -d msi_irqs ]]; then
		readarray -t m < <(ls msi_irqs)
		printf "msi_irqs : "
		printf "[%d] " ${#m[@]}
		printf "%s " "${m[@]}"
		printf "\n"
	fi
	ls resource*
	printf "\n"
	popd >/dev/null
done
