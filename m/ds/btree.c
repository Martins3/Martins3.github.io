#include "internal.h"

/*
 * 似乎使用者也不是很多
 * fs/hfs/btree.h
 * fs/hfs/btree.c
 * include/linux/btree-128.h
 * fs/hfsplus/hfsplus_raw.h
 * fs/hfsplus/btree.c
 * fs/hfsplus/xattr.c
 * drivers/scsi/qla2xxx/qla_target.h
 * drivers/scsi/qla2xxx/qla_def.h
 * drivers/scsi/qla2xxx/tcm_qla2xxx.h
 */
int test_btree(long action)
{
	switch (action) {
		break;
	}
	return 0;
}

/*
 * 因为定义 EXPORT_SYMBOL_GPL 之后，可以使用如下的:
 * 🧀  cat Module.symvers
 * 0xebb953c7      test_btree      /home/martins3/core/vn/code/src/m/martins3      EXPORT_SYMBOL_GPL
 */
EXPORT_SYMBOL_GPL(test_btree);
