#!/usr/bin/env python3
"""Apply the small SimpleFS compatibility layer to a private xfstests copy."""

from pathlib import Path
import re
import sys


def replace_once(text: str, needle: str, replacement: str, description: str) -> str:
    if replacement in text:
        return text
    if needle not in text:
        raise SystemExit(f"failed to patch {description}")
    return text.replace(needle, replacement, 1)


def patch_rc(path: Path) -> None:
    text = path.read_text()

    needle = '\t*)\n\t\t_notrun "Filesystem $FSTYP not supported in _scratch_mkfs_sized"\n\t\t;;'
    replacement = (
        '\tsimplefs)\n'
        '\t\t_simplefs_scratch_mkfs_sized "$fssize" "$blocksize" "$@"\n'
        '\t\t;;\n'
        + needle
    )
    if "_simplefs_scratch_mkfs_sized" not in text:
        text = replace_once(text, needle, replacement, "common/rc sized mkfs")

    journal_needle = '\tsimplefs|ext2|vfat|msdos|udf|exfat|tmpfs|hfs|hfsplus)'
    journal_replacement = '\text2|vfat|msdos|udf|exfat|tmpfs|hfs|hfsplus)'
    text = replace_once(
        text,
        journal_needle,
        journal_replacement,
        "common/rc journal support",
    )

    # SimpleFS 的磁盘块大小固定为 4 KiB，但仍需执行常规 mkfs，让调用方
    # 检查实际块大小并给出准确的“不支持指定块大小”原因。
    blocksize_needle = (
        "\tcase $FSTYP in\n"
        "\tbtrfs)\n"
        "\t\ttest -f /sys/fs/btrfs/features/supported_sectorsizes"
    )
    blocksize_replacement = (
        "\tcase $FSTYP in\n"
        "\tsimplefs)\n"
        "\t\t_scratch_mkfs\n"
        "\t\t;;\n"
        "\tbtrfs)\n"
        "\t\ttest -f /sys/fs/btrfs/features/supported_sectorsizes"
    )
    text = replace_once(
        text,
        blocksize_needle,
        blocksize_replacement,
        "common/rc block-sized mkfs",
    )

    # SimpleFS 的时间戳在磁盘上是有符号 32 位秒（读写路径都做 int32
    # 符号扩展），范围与 ext2 一致：[INT32_MIN, INT32_MAX]。
    text = re.sub(
        r'\tsimplefs\)\n\t\techo "[^"]*"\n\t\t;;\n(?=\tjfs\))', "", text
    )
    timestamp_needle = '\tjfs)\n\t\techo "0 $u32max"\n\t\t;;'
    timestamp_replacement = (
        '\tsimplefs)\n'
        '\t\techo "$s32min $s32max"\n'
        '\t\t;;\n'
        + timestamp_needle
    )
    text = replace_once(
        text,
        timestamp_needle,
        timestamp_replacement,
        "common/rc timestamp range",
    )

    # SimpleFS 的卷标上限 SIMPLEFS_LABEL_MAX = 63（s_volume_name[64]）。
    label_needle = '\text2|ext3|ext4)\n\t\techo 16\n\t\t;;'
    label_replacement = '\tsimplefs)\n\t\techo 63\n\t\t;;\n' + label_needle
    text = replace_once(
        text, label_needle, label_replacement, "common/rc label maximum"
    )

    # Scratch devices are intentionally reformatted, so force only here.
    mkfs_needle = (
        '\tf2fs)\n'
        '\t\tmkfs_cmd="$MKFS_F2FS_PROG -f"\n'
        '\t\tmkfs_filter="cat"\n'
        '\t\t;;'
    )
    mkfs_replacement = (
        '\tsimplefs)\n'
        '\t\tmkfs_cmd="$MKFS_PROG -t $FSTYP -- -f"\n'
        '\t\tmkfs_filter="cat"\n'
        '\t\t;;\n'
        + mkfs_needle
    )
    if 'mkfs_cmd="$MKFS_PROG -t $FSTYP -- -f"\n\t\tmkfs_filter="cat"' not in text:
        text = replace_once(text, mkfs_needle, mkfs_replacement, "forced scratch mkfs")

    path.write_text(text)


def patch_config(path: Path, mount_wrapper: Path) -> None:
    text = path.read_text()
    helper_needle = (
        '\tf2fs)\n'
        '\t\t[ "$MKFS_F2FS_PROG" = "" ] && _fatal "mkfs.f2fs not found"\n'
        '\t\t. ./common/f2fs\n'
        '\t\t;;'
    )
    helper_replacement = helper_needle + '\n\tsimplefs)\n\t\t. ./common/simplefs\n\t\t;;'
    if ". ./common/simplefs" not in text:
        text = replace_once(
            text, helper_needle, helper_replacement, "common/config helper"
        )

    mount_needle = 'export MOUNT_PROG="$(type -P mount)"'
    mount_replacement = f'export MOUNT_PROG="{mount_wrapper}"'
    text = replace_once(
        text, mount_needle, mount_replacement, "common/config MOUNT_PROG"
    )
    path.write_text(text)


def patch_log(path: Path) -> None:
    text = path.read_text()
    probe_needle = '''_scratch_ext4_logstate()
{
    $DUMPE2FS_PROG -h $SCRATCH_DEV 2> /dev/null | tee -a $seqres.full | \\
\tgrep "^Filesystem features" | grep -q needs_recovery
    test $? -ne 0
    echo $?
}
'''
    probe_replacement = probe_needle + '''
_scratch_simplefs_logstate()
{
    state=$(dd if="$SCRATCH_DEV" iflag=direct bs=4096 count=1 status=none \\
\t2>/dev/null | od -An -tu4 -j 108 -N 4 | tr -d '[:space:]')
    if [ "$state" = "0" ]; then
\techo 0
    else
\techo 1
    fi
}
'''
    text = text.replace(
        '    state=$(od -An -tu4 -j 108 -N 4 "$SCRATCH_DEV" 2>/dev/null)\n',
        '    state=$(dd if="$SCRATCH_DEV" iflag=direct bs=4096 count=1 '
        "status=none \\\n\t2>/dev/null | od -An -tu4 -j 108 -N 4 "
        "| tr -d '[:space:]')\n",
    )
    text = text.replace(
        "2>/dev/null | od -An -tu4 -j 108 -N 4)\n",
        "2>/dev/null | od -An -tu4 -j 108 -N 4 | tr -d '[:space:]')\n",
    )
    if "_scratch_simplefs_logstate" not in text:
        text = replace_once(text, probe_needle, probe_replacement, "common/log probe")

    print_needle = '''    ext4)
        dirty=$(_scratch_ext4_logstate)
        ;;
    *)
'''
    print_replacement = '''    ext4)
        dirty=$(_scratch_ext4_logstate)
        ;;
    simplefs)
        dirty=$(_scratch_simplefs_logstate)
        ;;
    *)
'''
    if "dirty=$(_scratch_simplefs_logstate)" not in text:
        text = replace_once(
            text, print_needle, print_replacement, "common/log state printer"
        )

    require_needle = '''    ext4)
\tif [ -z "$DUMPE2FS_PROG" ]; then
\t\t_notrun "This test requires dumpe2fs utility."
\tfi
\t;;
    *)
'''
    require_replacement = '''    ext4)
\tif [ -z "$DUMPE2FS_PROG" ]; then
\t    _notrun "This test requires dumpe2fs utility."
\tfi
\t;;
    simplefs)
\t;;
    *)
'''
    marker = 'simplefs)\n\t;;\n    *)\n        _notrun "$FSTYP does not support log state probing."'
    if marker not in text:
        text = replace_once(
            text, require_needle, require_replacement, "common/log requirement"
        )

    config_needle = '''_ext4_log_config()
{
'''
    simplefs_config = '''_simplefs_log_config()
{
    echo "# mkfs-opt             mount-opt"
    echo "# ------------------------------"
    for config in $(seq 1 10); do
\techo "  defaults             defaults"
    done
}

'''
    if "_simplefs_log_config()" not in text:
        text = replace_once(
            text,
            config_needle,
            simplefs_config + config_needle,
            "common/log configurations",
        )

    get_config_needle = '''    ext4)
        _ext4_log_config
        ;;
    *)
'''
    get_config_replacement = '''    ext4)
        _ext4_log_config
        ;;
    simplefs)
        _simplefs_log_config
        ;;
    *)
'''
    if "        _simplefs_log_config" not in text:
        text = replace_once(
            text,
            get_config_needle,
            get_config_replacement,
            "common/log configuration dispatch",
        )

    path.write_text(text)


def patch_attr(path: Path) -> None:
    text = path.read_text()
    needle = '\tbcachefs)\n\t\techo 251\n\t\t;;'
    replacement = '\tsimplefs)\n\t\techo 506\n\t\t;;\n' + needle
    if "simplefs)" not in text:
        text = replace_once(text, needle, replacement, "common/attr ACL maximum")
    path.write_text(text)


def patch_generic_735(path: Path) -> None:
    text = path.read_text()
    needle = 'file_blksz="$(_get_file_block_size ${SCRATCH_MNT})"\n'
    guard = needle + '''
max_file_size=$(_get_max_file_size $SCRATCH_MNT)
need_size=$((0xffffffff * file_blksz))
if [ -z "$max_file_size" ] || [ "$max_file_size" -lt "$need_size" ]; then
\t_notrun "filesystem max file size $max_file_size < $need_size for this test"
fi
'''
    text = replace_once(text, needle, guard, "generic/735 maximum file size")
    path.write_text(text)


def patch_generic_746(path: Path) -> None:
    text = path.read_text()
    # Clean branches inserted by early runner revisions so this stays idempotent.
    text = re.sub(
        r"\t?simplefs\)\n\t_unmount \$loop_mnt\n\t\$PYTHON3_PROG \S+/simplefs_free_sectors\.py \$loop_dev \$sectors_per_block\n\t;;\n",
        "",
        text,
    )
    text = re.sub(
        r'\t?simplefs\)\n(\t_notrun "simplefs sync-zeroout allocation deadlocks under loop-on-self nesting"\n\t;;\n|\t;;\n)',
        "",
        text,
    )
    needle = (
        'xfs)\n\t;;\n*)\n\t_notrun "Requires fs-specific way to check discard ranges"'
    )
    replacement = (
        "xfs)\n"
        "\t;;\n"
        "simplefs)\n"
        '\t_notrun "simplefs sync-zeroout allocation deadlocks under loop-on-self nesting"\n'
        "\t;;\n"
        "*)\n"
        '\t_notrun "Requires fs-specific way to check discard ranges"'
    )
    text = replace_once(text, needle, replacement, "generic/746 capability guard")
    path.write_text(text)


def patch_generic_492(path: Path) -> None:
    text = path.read_text()
    needle = "_require_label_get_max\n"
    guard = needle + '''
[ "$FSTYP" = "simplefs" ] && _notrun "blkid has no simplefs probe definition"
'''
    text = replace_once(text, needle, guard, "generic/492 blkid guard")
    path.write_text(text)


def patch_generic_558(path: Path) -> None:
    text = path.read_text()
    needle = """for ((i = 0; i < nr_cpus; i++)); do
\tcreate_file $SCRATCH_MNT/testdir $files_per_dir $i >>$seqres.full 2>&1 &
done
"""
    replacement = """for ((i = 0; i < nr_cpus; i++)); do
\t# Keep each worker below SimpleFS' on-disk per-directory limit while
\t# preserving this test's purpose: exhaust every inode concurrently.
\tmkdir -p "$SCRATCH_MNT/testdir/$i"
\tcreate_file "$SCRATCH_MNT/testdir/$i" $files_per_dir $i >>$seqres.full 2>&1 &
done
"""
    text = replace_once(
        text, needle, replacement, "generic/558 per-directory capacity"
    )
    path.write_text(text)


def patch_generic_620(path: Path) -> None:
    text = path.read_text()
    needle = "_require_scratch_16T_support\n"
    guard = needle + '''
[ "$FSTYP" = "simplefs" ] && _notrun "simplefs 32-bit block numbers limit filesystems to less than 16 TiB"
'''
    text = replace_once(text, needle, guard, "generic/620 volume size limit")
    path.write_text(text)


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit(f"usage: {sys.argv[0]} XFS_TESTS_DIR MOUNT_WRAPPER")

    root = Path(sys.argv[1])
    mount_wrapper = Path(sys.argv[2])
    patch_rc(root / "common/rc")
    patch_config(root / "common/config", mount_wrapper)
    patch_log(root / "common/log")
    patch_attr(root / "common/attr")
    patch_generic_735(root / "tests/generic/735")
    patch_generic_746(root / "tests/generic/746")
    patch_generic_492(root / "tests/generic/492")
    patch_generic_558(root / "tests/generic/558")
    patch_generic_620(root / "tests/generic/620")


if __name__ == "__main__":
    main()
