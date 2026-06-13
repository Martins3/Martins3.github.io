# super


```c
// 其他的内容
// 再一次，需要忽视quota相关的配置



// sync 同步工作似乎什么都没有做!
void ext2_sync_super(struct super_block *sb, struct ext2_super_block *es,
		     int wait)

static int ext2_sync_fs(struct super_block *sb, int wait)




// what is this ?
static void ext2_put_super (struct super_block * sb)



// 其中整个体系中间都是为了mount_super而创建的辅助函数。

// ext2_fill_super 辅助函数
static unsigned long get_sb_block(void **data)

// mount 相关的内容
static int ext2_remount (struct super_block * sb, int * flags, char * data)
// TODO

static struct dentry *ext2_mount(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data)
{
  // ext2_fill_super 将superblock 从disk 并且初始化
	return mount_bdev(fs_type, flags, dev_name, data, ext2_fill_super);
}

// 被ext2_mount 和 ext2_remount 使用，应该是用于表示其中启动
// super 中间各种 super_block 的各种加载写会维护之类的工作
static int ext2_setup_super (struct super_block * sb,
			      struct ext2_super_block * es,
			      int read_only)

static int ext2_fill_super(struct super_block *sb, void *data, int silent)
```

```c
// 不知道两者之间的功能关系是什么 ?
void kill_block_super(struct super_block *sb)
static void ext2_put_super (struct super_block * sb)
// mount的反向过程是什么
```


```c
// super_operation 部分新添加的内容
// TODO
static int ext2_freeze(struct super_block *sb)
static int ext2_unfreeze(struct super_block *sb)
```
