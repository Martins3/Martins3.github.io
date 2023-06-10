# sysfs 的内容
曾经，以为，sysfs 和 proc 中间的内容互相重复!

sysfs 应该都是各种 kobject 或者 device　之类的暴露 !

1. mount.c : 这个mount 不当人啊!

## group.c
其中的参数全部含有 kobject 值的分析的内容，

```c
/**
 * sysfs_create_group - given a directory kobject, create an attribute group
 * @kobj:	The kobject to create the group on
 * @grp:	The attribute group to create
 *
 * This function creates a group for the first time.  It will explicitly
 * warn and error if any of the attribute files being created already exist.
 *
 * Returns 0 on success or error code on failure.
 */
int sysfs_create_group(struct kobject *kobj,
		       const struct attribute_group *grp)
{
	return internal_create_group(kobj, 0, grp);
}
```
