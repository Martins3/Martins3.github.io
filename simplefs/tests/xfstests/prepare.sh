#!/usr/bin/env bash
set -E -e -u -o pipefail

# shellcheck shell=bash
# shellcheck disable=SC2154

function check_env() {
	[[ -d $XFT_SRC ]] || {
		echo "xfstests 源码不存在: $XFT_SRC" >&2
		exit 1
	}

	[[ -f $SIMPLEFS_DIR/simplefs.ko ]] || {
		echo "模块不存在: $SIMPLEFS_DIR/simplefs.ko" >&2
		exit 1
	}

	[[ -x $SIMPLEFS_DIR/mkfs.simplefs.out ]] || {
		echo "mkfs 工具不存在: $SIMPLEFS_DIR/mkfs.simplefs.out" >&2
		exit 1
	}
}

function load_test_module() {
	local module=$1

	if ! modprobe "$module"; then
		echo "警告: 无法加载 $module，依赖它的用例将 NOTRUN" >&2
	fi
}

function write_xfstests_config() {
	cat >"$XFT_DIR/local.config" <<EOF
export FSTYP=simplefs
export TEST_DEV=$TEST_DEV
export TEST_DIR=$TEST_DIR
export SCRATCH_DEV=$SCRATCH_DEV
export SCRATCH_MNT=$SCRATCH_MNT
export LOGWRITES_DEV=$LOGWRITES_DEV
export EMAIL=root@localhost
export MKFS_OPTIONS=-f
EOF
}

function write_simplefs_helper() {
	cat >"$XFT_DIR/common/simplefs" <<EOF
#!/usr/bin/env bash
SIMPLEFS_MKFS="$SIMPLEFS_DIR/mkfs.simplefs.out"
SIMPLEFS_SCRATCH_IMAGE="$SCRATCH_IMAGE"

function _setup_simplefs() {
	mkdir -p "\$TEST_DIR" "\$SCRATCH_MNT"
}

function _mkfs_dev() {
	local dev=\$1
	_setup_simplefs
	"\$SIMPLEFS_MKFS" -f "\$dev"
}

function _scratch_mkfs() {
	_setup_simplefs
	"\$SIMPLEFS_MKFS" -f "\$SCRATCH_DEV"
}

function _simplefs_scratch_mkfs_sized() {
	local fssize=\$1
	shift

	_setup_simplefs
	umount "\$SCRATCH_MNT" 2>/dev/null || true
	losetup -d "\$SCRATCH_DEV" 2>/dev/null || true
	truncate -s "\$fssize" "\$SIMPLEFS_SCRATCH_IMAGE"
	losetup "\$SCRATCH_DEV" "\$SIMPLEFS_SCRATCH_IMAGE"
	"\$SIMPLEFS_MKFS" -f "\$SCRATCH_DEV"
}

function _mount() {
	local dev=\$1
	local mnt=\$2
	shift 2
	mkdir -p "\$mnt"
	mount -t "\$FSTYP" "\$dev" "\$mnt" "\$@"
	chmod 777 "\$mnt" 2>/dev/null || true
}

function _unmount() {
	umount "\$1" 2>/dev/null || true
}
EOF
	chmod +x "$XFT_DIR/common/simplefs"
}

function write_xfstests_wrappers() {
	cat >"$XFT_DIR/mkfs.simplefs" <<EOF
#!/usr/bin/env bash
exec "$SIMPLEFS_DIR/mkfs.simplefs.out" "\$@"
EOF
	chmod +x "$XFT_DIR/mkfs.simplefs"

	# util-linux 2.40 的 mount 走新 mount API（fsmount），该路径在此内核
	# 不会触发 timestamp expiry 警告（mnt_warn_timestamp_expiry 只挂接在
	# legacy mount(2) 上）。generic/402 的 _require_timestamp_range 需要
	# 挂载时在 dmesg 中看到 "supports timestamps until" 警告，因此把
	# MOUNT_PROG 换成包装脚本，让 simplefs 挂载改走 legacy mount(2)。
	cc -O2 -Wall "$SIMPLEFS_DIR/tests/xfstests/legacy_mount.c" \
		-o "$XFT_DIR/legacy_mount.out"

	cat >"$XFT_DIR/mount-wrapper" <<EOF
#!/usr/bin/env bash
# 把 mount(8) 参数翻译给 legacy_mount。非 simplefs 挂载回退到真实 mount。
type=""
opts=""
declare -a positional=()
declare -a passthrough=()
while [[ \$# -gt 0 ]]; do
	case "\$1" in
	-t) type=\$2; shift 2 ;;
	-o) opts="\$opts,\$2"; shift 2 ;;
	-r) opts="\$opts,ro"; shift ;;
	-w) opts="\$opts,rw"; shift ;;
	--*) passthrough+=("\$1"); shift ;;
	*) positional+=("\$1"); shift ;;
	esac
done
opts=\${opts#,}
if [[ \$type != simplefs ]]; then
	declare -a fallback=(/bin/mount "\${passthrough[@]}")
	if [[ -n \$type ]]; then
		fallback+=(-t "\$type")
	fi
	if [[ -n \$opts ]]; then
		fallback+=(-o "\$opts")
	fi
	exec "\${fallback[@]}" "\${positional[@]}"
fi
exec "$XFT_DIR/legacy_mount.out" "\$type" "\${positional[0]}" "\${positional[1]}" "\$opts"
EOF
	chmod +x "$XFT_DIR/mount-wrapper"
}

function setup_xfstests() {
	mkdir -p "$WORK_ROOT" "$TMP_ROOT"

	if [[ ! -x $XFT_DIR/check ]]; then
		echo "同步 xfstests 到 $XFT_DIR..."
		rm -rf "$XFT_DIR"
		rsync -a "$XFT_SRC/" "$XFT_DIR/"
	fi

	# xfstests 的 configure 在 libc 尚未声明 file_getattr(2) 时不会构建
	# file_attr。改用 guest 原生编译器生成 .out，避免宿主 Nix ELF 依赖
	# /nix/store；再通过标准文件名让 _require_test_program 能找到它。
	if [[ -f $XFT_DIR/src/file_attr.c ]]; then
		cc -O2 -g -I"$XFT_DIR/include" -D_GNU_SOURCE \
			-D_FILE_OFFSET_BITS=64 -DHAVE_FILE_GETATTR \
			"$XFT_DIR/src/file_attr.c" \
			-o "$XFT_DIR/src/file_attr.out"
		ln -sfn file_attr.out "$XFT_DIR/src/file_attr"
	fi

	mkdir -p "$XFT_DIR/results"

	# 测试所需的 device-mapper/scsi_debug 模块已装进 /lib/modules，但
	# VM 每次重启后都需重新加载，否则对应的故障注入用例会 NOTRUN。
	load_test_module dm-flakey
	load_test_module dm-thin-pool
	load_test_module dm-log-writes
	load_test_module scsi_debug

	write_xfstests_config
	write_simplefs_helper
	write_xfstests_wrappers
	python3 "$SIMPLEFS_DIR/tests/xfstests/patch.py" \
		"$XFT_DIR" "$XFT_DIR/mount-wrapper"
}
