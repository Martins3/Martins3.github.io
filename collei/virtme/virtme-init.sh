#!/bin/sh
# virtme-init: 精简版 initramfs init 脚本

set -e

export PATH=/bin:/sbin:/usr/bin:/usr/sbin

log() {
	if [ -e /dev/kmsg ]; then
		echo "<6>virtme-init: $*" >/dev/kmsg 2>/dev/null || true
	else
		echo "virtme-init: $*"
	fi
}

mount_base_fs() {
	log "mounting base filesystems..."
	mount -t proc -o nosuid,noexec,nodev proc /proc 2>/dev/null || true
	mount -t sysfs -o nosuid,noexec,nodev sys /sys 2>/dev/null || true
	if ! grep -q devtmpfs /proc/mounts 2>/dev/null; then
		mount -t devtmpfs -o mode=0755,nosuid,noexec devtmpfs /dev 2>/dev/null || {
			mount -t tmpfs -o mode=0755 none /dev
			[ -e /dev/null ] || mknod -m 0666 /dev/null c 1 3 2>/dev/null || true
			[ -e /dev/kmsg ] || mknod -m 0660 /dev/kmsg c 1 11 2>/dev/null || true
			[ -e /dev/console ] || mknod -m 0600 /dev/console c 5 1 2>/dev/null || true
		}
	fi
	[ -e /dev/fd ] || ln -sf /proc/self/fd /dev/fd 2>/dev/null || true
	[ -e /dev/stdin ] || ln -sf /proc/self/fd/0 /dev/stdin 2>/dev/null || true
	[ -e /dev/stdout ] || ln -sf /proc/self/fd/1 /dev/stdout 2>/dev/null || true
	[ -e /dev/stderr ] || ln -sf /proc/self/fd/2 /dev/stderr 2>/dev/null || true
}

load_modules() {
	log "loading kernel modules..."

	# 检测需要加载的模块：检查哪些模块文件存在
	# pc/q35 机器: virtio_pci_modern_dev -> virtio_pci_legacy_dev -> virtio_pci -> fuse -> virtio_fs -> overlay
	# microvm 机器: virtio_mmio -> fuse -> virtio_fs -> overlay
	# 暂时不考虑 microvm

	mods_to_load=""

	# pc/q35 机器类型 - 使用 virtio-pci
	log "detected virtio-pci modules (pc/q35 machine)"
	# vsock 模块仅在使用 vsock 选项时才会被复制进 initramfs，不存在则跳过
	mods_to_load="virtio_pci_modern_dev virtio_pci_legacy_dev virtio_pci fuse virtio_fs overlay"
	mods_to_load="$mods_to_load vsock vmw_vsock_virtio_transport_common vmw_vsock_virtio_transport"

	for mod in $mods_to_load; do
		if [ -f "/lib/modules/${mod}.ko" ]; then
			log "loading ${mod}.ko..."
			if insmod "/lib/modules/${mod}.ko" 2>&1; then
				log "loaded module ${mod}"
			else
				log "module ${mod} built-in or failed (exitcode: $?)"
			fi
		fi
	done
}

mount_rootfs() {
	log "mounting ROOTFS..."
	mkdir -p /newroot
	if mount -t virtiofs -o ro ROOTFS /newroot 2>/dev/null; then
		log "mounted virtiofs successfully"
		return 0
	fi
	log "ERROR: failed to mount virtiofs"
	return 1
}

parse_cmdline() {
	log "parsing kernel cmdline..."
	if [ -r /proc/cmdline ]; then
		read -r CMDLINE </proc/cmdline
	fi
	for arg in ${CMDLINE}; do
		case "${arg}" in
			virtme_hostname=*) VIRTME_HOSTNAME="${arg#virtme_hostname=}" ;;
			virtme_console=*) VIRTME_CONSOLE="${arg#virtme_console=}" ;;
			virtme_user=*) VIRTME_USER="${arg#virtme_user=}" ;;
			virtme_root_user=*) VIRTME_ROOT_USER="${arg#virtme_root_user=}" ;;
			virtme_chdir=*) VIRTME_CHDIR="${arg#virtme_chdir=}" ;;
			virtme_link_mods=*) VIRTME_LINK_MODS="${arg#virtme_link_mods=}" ;;
			virtme.exec=*)
				VIRTME_EXEC="${arg#virtme.exec=}"
				VIRTME_EXEC="${VIRTME_EXEC#\`}"
				VIRTME_EXEC="${VIRTME_EXEC%\`}"
				;;
			virtme.vsock_cid=*)
				VIRTME_VSOCK_CID="${arg#virtme.vsock_cid=}"
				;;
			virtme_sudo_bin=*)
				VIRTME_SUDO_BIN="${arg#virtme_sudo_bin=}"
				;;
			virtme_rw_overlay*=*) eval "export ${arg%%=*}='${arg#*=}'" ;;
		esac
	done
}

setup_overlays() {
	log "setting up overlayfs..."
	mkdir -p /run/tmpfs
	mount -t tmpfs -o mode=0755 tmpfs /run/tmpfs

	# 与 virtme-ng 一致，/run 和 /tmp 使用 guest 独立的 tmpfs。
	# virtiofsd --no-announce-submounts 会隐藏 host 的 tmpfs，只暴露底层
	# 挂载点目录；如果继续使用 overlay，/tmp 会继承错误的 0755 权限。
	log "tmpfs: /run"
	mount -t tmpfs -o mode=0755,nosuid,nodev tmpfs /newroot/run
	# 确保 /run/tmp 存在
	mkdir -p /newroot/run/tmp
	log "tmpfs: /tmp"
	mount -t tmpfs -o mode=1777,nosuid,nodev tmpfs /newroot/tmp

	idx=0
	while true; do
		eval "dir=\${virtme_rw_overlay${idx}:-}"
		[ -z "$dir" ] && break
		log "overlay: $dir"
		upperdir="/run/tmpfs/overlay${idx}_upper"
		workdir="/run/tmpfs/overlay${idx}_work"
		mkdir -p "$upperdir" "$workdir"
		mkdir -p "/newroot${dir}"
		mount -t overlay overlay \
			-o "lowerdir=/newroot${dir},upperdir=${upperdir},workdir=${workdir}" \
			"/newroot${dir}" 2>/dev/null || log "warning: failed to overlay $dir"
		idx=$((idx + 1))
	done
}

setup_modules() {
	if [ -n "${VIRTME_LINK_MODS}" ]; then
		log "setting up kernel modules link..."
		kver=$(uname -r)
		mount -t tmpfs none /newroot/lib/modules 2>/dev/null || true
		ln -sf "${VIRTME_LINK_MODS}" "/newroot/lib/modules/${kver}" 2>/dev/null || true
	fi
}

setup_system_files() {
	log "setting up system files..."
	if [ -n "${VIRTME_HOSTNAME}" ]; then
		mkdir -p /newroot/run/tmp
		if [ -f /newroot/etc/hosts ]; then
			cp /newroot/etc/hosts /newroot/run/tmp/hosts
			printf '\n127.0.0.1 %s\n::1 %s\n' "$VIRTME_HOSTNAME" "$VIRTME_HOSTNAME" >>/newroot/run/tmp/hosts
			mount --bind /newroot/run/tmp/hosts /newroot/etc/hosts 2>/dev/null || true
		fi
		if [ -f /newroot/etc/hostname ]; then
			echo "$VIRTME_HOSTNAME" >/newroot/run/tmp/hostname
			mount --bind /newroot/run/tmp/hostname /newroot/etc/hostname 2>/dev/null || true
		fi
	fi
	touch /newroot/run/tmp/fstab 2>/dev/null || true
	if [ -f /newroot/etc/fstab ]; then
		mount --bind /newroot/run/tmp/fstab /newroot/etc/fstab 2>/dev/null || true
	fi
	setup_sudo
}

# 参考 virtme-ng 的做法（virtme/guest/virtme-init 的 generate_sudoers /
# generate_shadow）：host 的 /etc/sudoers 和 /etc/shadow 是 root-only 的，
# virtiofsd 以普通用户运行读不到，必须整体替换成 guest tmpfs 里的副本。
setup_sudo() {
	# sudoers 完全替换而不是追加 sudoers.d（主配置本身就不可读）
	if [ -e /newroot/etc/sudoers ]; then
		{
			echo 'Defaults secure_path="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"'
			echo 'root ALL = (ALL) NOPASSWD: ALL'
			[ -n "${VIRTME_USER}" ] && echo "${VIRTME_USER} ALL = (ALL) NOPASSWD: ALL"
		} >/newroot/run/tmp/sudoers
		chmod 0440 /newroot/run/tmp/sudoers
		mount --bind /newroot/run/tmp/sudoers /newroot/etc/sudoers 2>/dev/null || true
	fi
	# shadow 全部空密码（等价于 virtme-ng 的 --empty-passwords），让 su 可用。
	# 用纯 shell 生成，initramfs 里没有 sed。
	if [ -e /newroot/etc/shadow ] && [ -e /newroot/etc/passwd ]; then
		while IFS=: read -r name _; do
			echo "${name}::::::::"
		done </newroot/etc/passwd >/newroot/run/tmp/shadow
		chmod 0644 /newroot/run/tmp/shadow
		mount --bind /newroot/run/tmp/shadow /newroot/etc/shadow 2>/dev/null || true
	fi
	# sudo 的时间戳目录，避免只读 rootfs 上写入失败
	for dir in /var/db/sudo /var/lib/sudo; do
		if [ -d "/newroot${dir}" ]; then
			mount -t tmpfs tmpfs "/newroot${dir}" 2>/dev/null || true
		fi
	done
	# /etc/sudo.conf 同样是 root-only，bind-mount 一份最小配置消除警告
	if [ -e /newroot/etc/sudo.conf ]; then
		{
			echo 'Plugin sudoers_policy sudoers.so'
			echo 'Plugin sudoers_io sudoers.so'
		} >/newroot/run/tmp/sudo.conf
		chmod 0644 /newroot/run/tmp/sudo.conf
		mount --bind /newroot/run/tmp/sudo.conf /newroot/etc/sudo.conf 2>/dev/null || true
	fi
	# /root 对 virtiofsd 不可读时，挂一个 tmpfs 让 root 的 home 可用
	if [ -d /newroot/root ] && ! ls /newroot/root >/dev/null 2>&1; then
		mount -t tmpfs -o mode=0700 tmpfs /newroot/root 2>/dev/null || true
	fi
	# Fedora 的 sudo 是 ---s--x--x (4111)，virtiofsd 读不到导致无法 exec。
	# host 侧准备了 4755 的副本，bind-mount 覆盖 /usr/bin/sudo。
	if [ -n "${VIRTME_SUDO_BIN}" ] && [ -f "/newroot${VIRTME_SUDO_BIN}" ] && [ -e /newroot/usr/bin/sudo ]; then
		mount --bind "/newroot${VIRTME_SUDO_BIN}" /newroot/usr/bin/sudo 2>/dev/null || true
	fi
}

mount_extra_fs() {
	log "mounting extra filesystems..."
	mkdir -p /newroot/proc /newroot/sys /newroot/dev
	mount -t proc -o nosuid,noexec,nodev proc /newroot/proc
	mount -t sysfs -o nosuid,noexec,nodev sys /newroot/sys
	if ! mount -t devtmpfs -o mode=0755,nosuid,noexec devtmpfs /newroot/dev; then
		log "warning: failed to mount a second devtmpfs, moving initramfs /dev"
		mount --move /dev /newroot/dev
	fi
	# devtmpfs 不会自动创建 pts/ 和 shm/ 目录；没有 /dev/pts 时
	# sshd 无法分配 pty（报 PTY allocation request failed）
	mkdir -p /newroot/dev/pts /newroot/dev/shm 2>/dev/null || true
	mount -t devpts -o gid=5,mode=620,noexec,nosuid devpts /newroot/dev/pts 2>/dev/null || true
	mount -t tmpfs -o mode=1777,nosuid,nodev tmpfs /newroot/dev/shm 2>/dev/null || true
	if [ -d /newroot/sys/fs/cgroup ]; then
		mount -t cgroup2 cgroup2 /newroot/sys/fs/cgroup 2>/dev/null || true
	fi
	for fs in debugfs tracefs securityfs configfs; do
		mount_point="/newroot/sys/kernel/${fs}"
		if [ -d "$mount_point" ]; then
			mount -t "$fs" "$fs" "$mount_point" 2>/dev/null || true
		fi
	done
}

write_vsock_ssh_service() {
	# 生成 guest 内的 vsock SSH 服务：启动只监听 loopback 的 sshd，
	# 并把 vsock:22 转发过去，host 即可通过 socat VSOCK-CONNECT 登录。
	cat >/newroot/run/tmp/vsock-ssh.py <<'PYEOF'
#!/usr/bin/env python3
"""virtme vsock SSH 服务：启动 sshd，并把 vsock:22 转发到 127.0.0.1:22。

host 的 /etc/ssh/sshd_config 和 host key 是 root-only 的，而 virtiofsd 以
普通用户身份运行，guest 里的 root 也读不到，所以 sshd 使用 /run 下自己
生成的配置和 host key。
"""
import os
import socket
import subprocess
import sys
import threading

SSHD = "/usr/sbin/sshd"
PORT = 22
WORKDIR = "/run/vsock-ssh"
HOST_KEY = f"{WORKDIR}/ssh_host_ed25519_key"
SSHD_CONFIG = f"""\
Port {PORT}
ListenAddress 127.0.0.1
HostKey {HOST_KEY}
PermitRootLogin yes
PubkeyAuthentication yes
UsePAM no
PidFile {WORKDIR}/sshd.pid
Subsystem sftp internal-sftp
"""


def pump(source, target):
    try:
        while data := source.recv(65536):
            target.sendall(data)
    except OSError:
        pass
    for sock in (source, target):
        try:
            sock.shutdown(socket.SHUT_RDWR)
        except OSError:
            pass


def handle(conn):
    try:
        upstream = socket.create_connection(("127.0.0.1", PORT))
    except OSError:
        conn.close()
        return
    threading.Thread(target=pump, args=(upstream, conn), daemon=True).start()
    pump(conn, upstream)


def prepare_sshd():
    os.makedirs(WORKDIR, exist_ok=True)
    if not os.path.exists(HOST_KEY):
        subprocess.run(
            ["ssh-keygen", "-t", "ed25519", "-f", HOST_KEY, "-N", "", "-q"],
            check=True,
        )
    with open(f"{WORKDIR}/sshd_config", "w") as output:
        output.write(SSHD_CONFIG)


def main():
    if not os.path.isfile(SSHD):
        sys.exit("vsock-ssh: /usr/sbin/sshd not found")
    prepare_sshd()
    subprocess.Popen([SSHD, "-f", f"{WORKDIR}/sshd_config", "-e"])
    listener = socket.socket(socket.AF_VSOCK, socket.SOCK_STREAM)
    listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    listener.bind((socket.VMADDR_CID_ANY, PORT))
    listener.listen()
    while True:
        conn, _ = listener.accept()
        threading.Thread(target=handle, args=(conn,), daemon=True).start()


main()
PYEOF
}

start_session() {
	log "starting user session..."
	user="root"
	[ -n "${VIRTME_USER}" ] && user="${VIRTME_USER}"
	cwd="/"
	[ -n "${VIRTME_CHDIR}" ] && cwd="${VIRTME_CHDIR}"
	console="${VIRTME_CONSOLE:-}"
	[ -n "${VIRTME_ROOT_USER:-}" ] && :

	runtime_module_loader=$(
		cat <<'EOF'
load_runtime_modules() {
	# Load common virtio drivers before dropping to the target user.
	for mod in virtio_blk virtio_net virtio_scsi; do
		modprobe "$mod" >/dev/null 2>&1 || true
	done
}
EOF
	)

	vsock_ssh_launcher=$(
		cat <<'EOF'
start_vsock_ssh() {
	# vsock-ssh.py 仅在启用 vsock 选项时生成
	[ -f /run/tmp/vsock-ssh.py ] || return 0
	if command -v python3 >/dev/null 2>&1; then
		# sshd 要监听 127.0.0.1，先拉起 loopback
		ip link set lo up 2>/dev/null || true
		python3 /run/tmp/vsock-ssh.py >/run/tmp/vsock-ssh.log 2>&1 &
	else
		echo "vsock ssh disabled: python3 not found"
	fi
}
EOF
	)
	if [ -n "${VIRTME_VSOCK_CID}" ]; then
		write_vsock_ssh_service
	fi

	if [ -n "${VIRTME_EXEC}" ]; then
		log "script execution mode"
		echo "${VIRTME_EXEC}" | base64 -d >/newroot/run/tmp/virtme-script.sh 2>/dev/null || {
			log "ERROR: failed to decode script"
			cat >/newroot/run/tmp/virtme-script.sh <<'EOF'
#!/bin/sh
echo ERROR: failed to decode script
exit 1
EOF
		}
		chmod +x /newroot/run/tmp/virtme-script.sh

		# 脚本执行器
		cat >/newroot/run/tmp/virtme-start.sh <<EOF
#!/bin/sh
export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
USER='$user'
CWD='$cwd'
VIRTME_CONSOLE='$console'
${runtime_module_loader}
${vsock_ssh_launcher}
cd "\$CWD" 2>/dev/null || cd /
if [ -n "\$VIRTME_CONSOLE" ] && [ -c "/dev/\$VIRTME_CONSOLE" ]; then
	exec </dev/"\$VIRTME_CONSOLE" >/dev/"\$VIRTME_CONSOLE" 2>&1
elif [ -c /dev/console ]; then
	exec </dev/console >/dev/console 2>&1
fi
load_runtime_modules
start_vsock_ssh
if [ "\$USER" = root ]; then
	/bin/bash /run/tmp/virtme-script.sh
else
	su -c "/bin/bash /run/tmp/virtme-script.sh" "\$USER"
fi
echo \$? >/run/tmp/virtme-exit-code
poweroff -f
EOF
	else
		log "interactive shell mode"
		cat >/newroot/run/tmp/virtme-start.sh <<EOF
#!/bin/sh
export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
USER='$user'
CWD='$cwd'
VIRTME_CONSOLE='$console'
${runtime_module_loader}
${vsock_ssh_launcher}
cd "\$CWD" 2>/dev/null || cd /
if [ -n "\$VIRTME_CONSOLE" ] && [ -c "/dev/\$VIRTME_CONSOLE" ]; then
	exec </dev/"\$VIRTME_CONSOLE" >/dev/"\$VIRTME_CONSOLE" 2>&1
elif [ -c /dev/console ]; then
	exec </dev/console >/dev/console 2>&1
fi
load_runtime_modules
start_vsock_ssh
echo
echo "  Welcome to virtme mode!"
echo "  User: \$USER"
echo "  CWD: \$CWD"
echo "  [CTRL+d to exit]"
echo
if [ "\$USER" = root ]; then
	setsid /bin/busybox cttyhack /bin/bash --login
else
	setsid /bin/busybox cttyhack su - "\$USER"
fi
poweroff -f
EOF
	fi

	cat >/newroot/run/tmp/init.sh <<'EOF'
#!/bin/sh
exec /run/tmp/virtme-start.sh
EOF
	chmod +x /newroot/run/tmp/virtme-start.sh /newroot/run/tmp/init.sh
}

main() {
	log "virtme-init starting..."
	mount_base_fs
	load_modules
	parse_cmdline
	mount_rootfs || {
		log "FATAL: cannot mount rootfs"
		sleep 5
		exit 1
	}
	setup_overlays
	setup_modules
	setup_system_files
	mount_extra_fs
	start_session
	log "switching to real root..."
	umount /proc 2>/dev/null || true
	umount /sys 2>/dev/null || true
	umount /dev 2>/dev/null || true
	exec switch_root /newroot /bin/sh -c 'exec /run/tmp/init.sh'
}

main
log "ERROR: init exited unexpectedly"
sleep 5
exit 1
