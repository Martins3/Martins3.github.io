#!/bin/sh
# First-stage initramfs loader for the Rust virtme init.

set -eu

export PATH=/bin

log() {
	if [ -e /dev/kmsg ]; then
		echo "<6>virtme-init-loader: $*" >/dev/kmsg
	else
		echo "virtme-init-loader: $*"
	fi
}

mount_base_filesystems() {
	log "mounting base filesystems"
	mount -t proc -o nosuid,noexec,nodev proc /proc
	mount -t sysfs -o nosuid,noexec,nodev sys /sys
	if ! mount -t devtmpfs -o mode=0755,nosuid,noexec devtmpfs /dev; then
		mount -t tmpfs -o mode=0755 tmpfs /dev
		mknod -m 0666 /dev/null c 1 3
		mknod -m 0660 /dev/kmsg c 1 11
		mknod -m 0600 /dev/console c 5 1
	fi
}

load_modules() {
	log "loading boot modules"
	for module in \
		virtio_pci_modern_dev \
		virtio_pci_legacy_dev \
		virtio_pci \
		fuse \
		virtio_fs \
		overlay \
		vsock \
		vmw_vsock_virtio_transport_common \
		vmw_vsock_virtio_transport; do
		module_path="/lib/modules/${module}.ko"
		if [ -f "${module_path}" ]; then
			log "loading ${module}.ko"
			insmod "${module_path}" || log "${module}.ko is built in or could not be loaded"
		fi
	done
}

mount_rootfs() {
	log "mounting ROOTFS"
	mkdir -p /newroot
	if ! mount -t virtiofs -o ro ROOTFS /newroot; then
		log "fatal: failed to mount ROOTFS"
		sleep 5
		exit 1
	fi
}

install_second_stage() {
	log "installing Rust second-stage init"
	mount -t tmpfs -o mode=1777,nosuid,nodev tmpfs /newroot/tmp
	cp /bin/virtme-ng-init.out /newroot/tmp/virtme-ng-init.out
	chmod 0755 /newroot/tmp/virtme-ng-init.out
}

switch_to_rootfs() {
	log "switching to ROOTFS"
	umount /proc
	umount /sys
	umount /dev
	exec switch_root /newroot /tmp/virtme-ng-init.out
}

mount_base_filesystems
load_modules
mount_rootfs
install_second_stage
switch_to_rootfs
