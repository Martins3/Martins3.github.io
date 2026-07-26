#!/usr/bin/env bash
set -E -e -u -o pipefail

function do_test() {
	MAIN_REPO=/home/martins3/data/vn

	$MAIN_REPO/collei/scripts/collei.py
	sleep 25
	ssh -p50284 root@localhost /root/a.sh
}

function setup() {
	tar -xvf "$version".tar
	cd install-"$version"
	./run.sh
	sudo grubby --set-default /boot/vmlinuz-"$version"
	sudo reboot
}

version=${1-}
if [[ $version ]]; then
	setup
else
	do_test
fi
