安装的事情:

fs : myfs 介绍虚拟的文件系统
minfs : 真正的文件系统


## bug
1. filesystem(lab1) 的测试中间，当 
```c
cd /mnt/minfs
ls -la f
```
在 do_dentry_open 中间触发错误，并且会引发 slub 的错误。

```c
static struct inode *minfs_alloc_inode(struct super_block *s)
{
	struct minfs_inode_info *mii;

	/* TODO 3: Allocate minfs_inode_info. */
	/* TODO 3: init VFS inode in minfs_inode_info */
	mii = (struct minfs_inode_info*)kzalloc(sizeof(struct minfs_inode_info), GFP_KERNEL); // FIXME 写成了 sizeof(mii)，导致分配的空间不对
  if(!mii){
    return NULL;
  }
	inode_init_once(&mii->vfs_inode);

	return &mii->vfs_inode;
}
```

2. myfs 当认为彻底完成之后，结果没有办法 cd

You will need to specify the following directory operations:
create a file (create function)
search (lookup function)
link (link function)
create directory (mkdir function)
deletion (rmdir and unlink functions)
create node (mknod)
rename (rename function)

但是只是注册了，其中的 mknod create 和 mkdir，但是其他的都是没有注册，
因为没有注册 lookup，所以没有办法 cd

## debug 部分的试验完全没有做啊!

