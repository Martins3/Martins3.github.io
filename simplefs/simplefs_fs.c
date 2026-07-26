#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/file.h>
#include <linux/fs_context.h>
#include <linux/fs_parser.h>

#include "simplefs.h"
#include "simplefs_debug.h"

/* Mount option IDs */
enum { Opt_nojournal, Opt_norecovery, Opt_discard };

static const struct fs_parameter_spec simplefs_param_specs[] = {
	fsparam_flag("nojournal", Opt_nojournal),
	fsparam_flag("norecovery", Opt_norecovery),
	fsparam_flag("discard", Opt_discard),
	{}
};

static int simplefs_parse_param(struct fs_context *fc, struct fs_parameter *param)
{
	struct simplefs_fs_context *ctx = fc->fs_private;
	struct fs_parse_result result;
	int opt;

	opt = fs_parse(fc, simplefs_param_specs, param, &result);
	if (opt < 0)
		return opt;

	switch (opt) {
	case Opt_nojournal:
		ctx->nojournal = 1;
		break;
	case Opt_norecovery:
		ctx->norecovery = 1;
		break;
	case Opt_discard:
		ctx->discard = 1;
		break;
	}
	return 0;
}

/* Forward declarations */
static int simplefs_get_tree(struct fs_context *fc);

static const struct fs_context_operations simplefs_context_ops;

/* Get tree for simplefs (new mount API) */
static int simplefs_get_tree(struct fs_context *fc)
{
	return get_tree_bdev(fc, simplefs_fill_super);
}

static void simplefs_free_fc(struct fs_context *fc)
{
	kfree(fc->fs_private);
}

static const struct fs_context_operations simplefs_context_ops = {
	.parse_param = simplefs_parse_param,
	.get_tree = simplefs_get_tree,
	.free = simplefs_free_fc,
};

/* Initialize fs context */
static int simplefs_init_fs_context(struct fs_context *fc)
{
	struct simplefs_fs_context *ctx;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	fc->fs_private = ctx;
	fc->ops = &simplefs_context_ops;
	return 0;
}

/* Unmount a simplefs partition */
static void simplefs_kill_sb(struct super_block *sb)
{
	kill_block_super(sb);

	/* struct simplefs_sb_info *csb = SIMPLEFS_SB(sb); */
	/* fput(csb->bdev); */
	/* dump_stack(); */

	pr_debug("unmounted disk\n");
}

static struct file_system_type simplefs_file_system_type = {
	.owner = THIS_MODULE,
	.name = "simplefs",
	.init_fs_context = simplefs_init_fs_context,
	.kill_sb = simplefs_kill_sb,
	.fs_flags = FS_REQUIRES_DEV,
	.next = NULL,
};

static int __init simplefs_init(void)
{
	int ret = simplefs_init_inode_cache();
	if (ret) {
		pr_err("inode cache creation failed\n");
		goto end;
	}

	ret = simplefs_debugfs_init();
	if (ret) {
		pr_err("debugfs init failed\n");
		goto err_debugfs;
	}

	ret = register_filesystem(&simplefs_file_system_type);
	if (ret) {
		pr_err("register_filesystem() failed\n");
		goto err_register;
	}

	pr_debug("module loaded\n");
	return 0;

err_register:
	simplefs_debugfs_exit();
err_debugfs:
	simplefs_destroy_inode_cache();
end:
	return ret;
}

static void __exit simplefs_exit(void)
{
	int ret = unregister_filesystem(&simplefs_file_system_type);
	if (ret)
		pr_err("unregister_filesystem() failed\n");

	simplefs_debugfs_exit();
	simplefs_destroy_inode_cache();

	pr_debug("module unloaded\n");
}

module_init(simplefs_init);
module_exit(simplefs_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("a simple file system");
