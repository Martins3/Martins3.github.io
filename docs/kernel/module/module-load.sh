#!/usr/bin/env bash
set -E -e -u -o pipefail
set -x

if [[ ! -f /etc/modules-load.d/a.conf ]]; then
	cat <<_EOF_ >/etc/modules-load.d/a.conf
nf_nat_ftp
nf_nat_tftp
nf_conntrack_ftp
nf_conntrack_tftp
_EOF_

echo "abc" > /etc/modules-load.d/b.conf
fi

counter=0
while true; do
	echo "----- $counter"
	counter=$((counter + 1))

	/usr/lib/systemd/systemd-modules-load || :
	ls /sys/module/nf_nat_tftp
	ls /sys/module/nf_conntrack_tftp
	ls /sys/module/nf_nat_ftp
	ls /sys/module/nf_conntrack_ftp

	modprobe -r nf_nat_tftp
	modprobe -r nf_conntrack_tftp
	modprobe -r nf_nat_ftp
	modprobe -r nf_conntrack_ftp

	sleep 1
done
