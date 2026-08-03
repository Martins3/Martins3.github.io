// SPDX-License-Identifier: GPL-2.0
/*
 * legacy_mount - 通过 legacy mount(2) 系统调用挂载，绕过 util-linux
 * 默认使用的新 mount API（fsmount）。
 *
 * 背景：mnt_warn_timestamp_expiry（"supports timestamps until" 警告）
 * 在此内核只挂接在 legacy mount(2) 路径上，fsmount 路径不触发。
 * generic/402 的 _require_timestamp_range 需要挂载时在 dmesg 中看到
 * 该警告。xfstests 把 MOUNT_PROG 指到 mount-wrapper 后，simplefs 的
 * 挂载经由本 helper 走 legacy mount(2)，警告会在测试期间真实产生，
 * 402 的实质钳制断言即可执行。
 *
 * 用法: legacy_mount TYPE DEVICE MOUNTPOINT [OPTIONS]
 *
 * OPTIONS 以逗号分隔。与 mount(8) 一致：atime/sync/ro 等标准选项
 * 翻译成 MS_* 挂载标志位（不能放进 data 串，否则文件系统 parse_param
 * 会拒绝）；其余选项原样拼回 data 传给文件系统。
 */
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/mount.h>

struct optmap {
	const char *name;
	unsigned long set;
	unsigned long clr;
};

/* 与 util-linux libmount 的标准选项表对应（常用子集） */
static const struct optmap standard_opts[] = {
	{ "ro", MS_RDONLY, 0 },
	{ "rw", 0, MS_RDONLY },
	{ "remount", MS_REMOUNT, 0 },
	{ "bind", MS_BIND, 0 },
	{ "rbind", MS_BIND | MS_REC, 0 },
	{ "move", MS_MOVE, 0 },
	{ "rec", MS_REC, 0 },
	{ "sync", MS_SYNCHRONOUS, 0 },
	{ "async", 0, MS_SYNCHRONOUS },
	{ "dirsync", MS_DIRSYNC, 0 },
	{ "silent", MS_SILENT, 0 },
	{ "loud", 0, MS_SILENT },
	{ "mand", MS_MANDLOCK, 0 },
	{ "nomand", 0, MS_MANDLOCK },
	{ "atime", 0, MS_NOATIME },
	{ "noatime", MS_NOATIME, 0 },
	{ "relatime", MS_RELATIME, 0 },
	{ "norelatime", 0, MS_RELATIME },
	{ "strictatime", MS_STRICTATIME, 0 },
	{ "nostrictatime", 0, MS_STRICTATIME },
	{ "nodiratime", MS_NODIRATIME, 0 },
	{ "diratime", 0, MS_NODIRATIME },
	{ "lazytime", MS_LAZYTIME, 0 },
	{ "nolazytime", 0, MS_LAZYTIME },
	{ "dev", 0, MS_NODEV },
	{ "nodev", MS_NODEV, 0 },
	{ "exec", 0, MS_NOEXEC },
	{ "noexec", MS_NOEXEC, 0 },
	{ "suid", 0, MS_NOSUID },
	{ "nosuid", MS_NOSUID, 0 },
	{ "iversion", MS_I_VERSION, 0 },
	{ "noiversion", 0, MS_I_VERSION },
	{ "private", MS_PRIVATE, 0 },
	{ "shared", MS_SHARED, 0 },
	{ "slave", MS_SLAVE, 0 },
	{ "unbindable", MS_UNBINDABLE, 0 },
	{ "rprivate", MS_PRIVATE | MS_REC, 0 },
	{ "rshared", MS_SHARED | MS_REC, 0 },
	{ "rslave", MS_SLAVE | MS_REC, 0 },
	{ "runbindable", MS_UNBINDABLE | MS_REC, 0 },
	/* util-linux 层面的选项，直接忽略 */
	{ "defaults", 0, 0 },
	{ "auto", 0, 0 },
	{ "noauto", 0, 0 },
	{ "user", 0, 0 },
	{ "nouser", 0, 0 },
	{ "owner", 0, 0 },
	{ "group", 0, 0 },
	{ "nofail", 0, 0 },
};

int main(int argc, char **argv)
{
	unsigned long flags = 0;
	static char opts[4096];
	static char data[4096];
	const char *type, *dev, *mnt;
	char *tok, *save = NULL;
	unsigned int i;

	if (argc < 4) {
		fprintf(stderr, "usage: %s TYPE DEVICE MOUNTPOINT [OPTIONS]\n",
			argv[0]);
		return 1;
	}
	type = argv[1];
	dev = argv[2];
	mnt = argv[3];
	data[0] = '\0';

	if (argc > 4) {
		strncpy(opts, argv[4], sizeof(opts) - 1);
		opts[sizeof(opts) - 1] = '\0';

		for (tok = strtok_r(opts, ",", &save); tok;
		     tok = strtok_r(NULL, ",", &save)) {
			int handled = 0;

			for (i = 0; i < sizeof(standard_opts) /
					sizeof(standard_opts[0]); i++) {
				if (!strcmp(tok, standard_opts[i].name)) {
					flags |= standard_opts[i].set;
					flags &= ~standard_opts[i].clr;
					handled = 1;
					break;
				}
			}
			if (handled)
				continue;
			if (data[0])
				strncat(data, ",",
					sizeof(data) - strlen(data) - 1);
			strncat(data, tok, sizeof(data) - strlen(data) - 1);
		}
	}

	if (mount(dev, mnt, type, flags, data[0] ? data : NULL) == 0)
		return 0;

	/* mount(8) retries a read-only block device with MS_RDONLY.  The
	 * legacy helper must preserve that behavior because generic/050
	 * intentionally toggles BLKROSET underneath the filesystem. */
	if (!(flags & MS_RDONLY) && (errno == EACCES || errno == EROFS)) {
		fprintf(stderr,
			"mount: %s is write-protected, mounting read-only\n", dev);
		if (mount(dev, mnt, type, flags | MS_RDONLY,
			  data[0] ? data : NULL) == 0)
			return 0;
		fprintf(stderr, "mount: cannot mount %s read-only\n", dev);
		return 1;
	}

	perror("mount");
	return 1;
}
