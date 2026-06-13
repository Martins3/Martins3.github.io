#!/usr/bin/env bash
rootfs=~/hack/uml/img
linux=~/core/linux-uml/linux

if ip a | grep tap-uml; then
	sudo tunctl -u martins3 -t tap-uml
fi

$linux mem=2048M umid=TEST \
	ubd0=$rootfs \
	vec0:transport=tap,ifname=tap-uml,depth=128,gro=1 \
	root=/dev/ubda con=null con0=null,fd:2 con1=fd:0,fd:1
