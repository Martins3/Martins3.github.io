#include "internal.h"
#include <linux/device.h>
/*
 * kobject_add
 *
| Function                 | Meaning                                                                                                                                                                        |
|--------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| kobject_get, kobject_put | Increments or decrements the reference counter of a kobject                                                                                                                    |
| `kobject_(un)register`   | Registers or removes obj from a hierarchy (the object is added to the  existing set (if any) of the parent element; a corresponding entry is created in the sysfs filesystem). |
| `kobject_init`           | Initializes a kobject; that is, it sets the reference counter to its initial value and initializes the list elements of the object.                                            |
| kobect_add               | Initializes a kernel object and makes it visible in sysfs                                                                                                                      |
| kobject_cleanup          | Releases the allocated resources when a kobject (and therefore the embedding object) is no longer needed                                                                       |
 */

// 1. 普通的 kobj_attribute
static ssize_t ma(struct kobject *kobj, struct kobj_attribute *attr,
		  const char *buf, size_t count)
{
	int ret;
	long action;
	ret = kstrtol(buf, 10, &action);
	if (ret < 0)
		return ret;
	pr_info("action = %ld current=%s \n", action, current->comm);
	return count;
}

static struct kobj_attribute ma2 = __ATTR(show, 0644, NULL, ma);

static struct attribute *pci_bus_attrs[] = {
	&ma2.attr,
	NULL,
};

static const struct attribute_group pci_bus_group = {
	.attrs = pci_bus_attrs,
};

static const struct attribute_group *pci_bus_groups[] = {
	&pci_bus_group,
	NULL,
};

// 2. bin_attribute
static ssize_t firmware_data_read(struct file *filp, struct kobject *kobj,
				  struct bin_attribute *bin_attr, char *buffer,
				  loff_t offset, size_t count)
{
	pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	return 0;
}

static ssize_t firmware_data_write(struct file *filp, struct kobject *kobj,
				   struct bin_attribute *bin_attr, char *buffer,
				   loff_t offset, size_t count)
{
	pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	return 0;
}

static struct bin_attribute firmware_attr_data = {
	.attr = { .name = "data", .mode = 0644 },
	.size = 0,
	.read = firmware_data_read,
	.write = firmware_data_write,
	// TODO 看上去 bin_attribute 提供更加复杂的接口
	// 似乎就是可以暴露一个完整的文件出去
	.mmap = NULL,
};

static struct bin_attribute *fw_dev_bin_attrs[] = { &firmware_attr_data, NULL };

static const struct attribute_group fw_dev_attr_group = {
	.attrs = NULL,
	.bin_attrs = fw_dev_bin_attrs,
};

// 3. device_attribute
//
// 其实和 kobj_attribute 区别不大，但是这个函数的参数是 device 和 device_attribute

static ssize_t iommu_cpumask_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", 123);
}
static DEVICE_ATTR(cpumask, 0644, iommu_cpumask_show, NULL);

static struct attribute *iommu_cpumask_attrs[] = {
	&dev_attr_cpumask.attr,
	NULL,
};

// TODO 想不到这个居然返回的是属性
static umode_t disk_visible(struct kobject *kobj, struct attribute *a, int n)
{
	return 0644;
}

static struct attribute_group amd_iommu_cpumask_group = {
	.attrs = iommu_cpumask_attrs,
	.is_visible = disk_visible,
};

static const struct attribute_group *amd_iommu_attr_groups[] = {
	&amd_iommu_cpumask_group,
	NULL,
};

// 4. sysfs_ops
//
// 可以对照 drivers/md/md-bitmap.c 中 bitmap_location 来看 sysfs_ops 的
// 其实 sysfs_ops 可以实现各种类似这种的结构:
//
// struct md_sysfs_entry {
// 	struct attribute attr;
// 	ssize_t (*show)(struct mddev *, char *);
// 	ssize_t (*store)(struct mddev *, const char *, size_t);
// };
//
//
// 有点怀疑 kobj_attribute 也是通过 sysfs_ops 来实现的

static ssize_t blk_mq_hw_sysfs_show(struct kobject *kobj,
				    struct attribute *attr, char *page)
{
	/*
	 *  cat bitmap/A 或者 bitmap/B 都是调用这个函数，所以一般在这里判断
	 *  来调用具体的 md_sysfs_entry::show 的。
	 */
	pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	return 0;
}
static const struct sysfs_ops blk_mq_hw_sysfs_ops = {
	.show = blk_mq_hw_sysfs_show,
};

static struct attribute A = { .name = "A",
			      .mode = VERIFY_OCTAL_PERMISSIONS(0644) };
static struct attribute B = { .name = "B",
			      .mode = VERIFY_OCTAL_PERMISSIONS(0644) };
static struct attribute *md_bitmap_attrs[] = { &A, &B, NULL };

const struct attribute_group md_bitmap_group = {
	.name = "bitmap",
	.attrs = md_bitmap_attrs,
};

static const struct attribute_group *md_attr_groups[] = {
	&md_bitmap_group,
	NULL,
};

static const struct kobj_type blk_mq_hw_ktype = {
	.sysfs_ops = &blk_mq_hw_sysfs_ops,
	.default_groups = md_attr_groups,
};

static ssize_t class_attr_show(struct kobject *kobj, struct attribute *attr,
			       char *buf)
{
	return 0;
}

static ssize_t class_attr_store(struct kobject *kobj, struct attribute *attr,
				const char *buf, size_t count)
{
	return 0;
}

static const struct sysfs_ops class_sysfs_ops = {
	.show = class_attr_show,
	.store = class_attr_store,
};

static const struct kobj_type class_ktype = {
	.sysfs_ops = &class_sysfs_ops,
};

static struct kobject *kobj1;
static struct kobject *kobj2;
static struct kobject *kobj3;
static struct kobject kobj4;
static struct kobject kobj5;
static struct kobject kobj6;

static struct kset *set;
static struct kset set2;

int test_kobject_init(void)
{
	int error;
	kobj1 = kobject_create_and_add("obj1", kernel_kobj);
	if (!kobj1)
		return -ENOMEM;

	kobj2 = kobject_create_and_add("obj2", kernel_kobj);
	if (!kobj2)
		return -ENOMEM;

	kobj3 = kobject_create_and_add("obj3", kernel_kobj);
	// TODO 这个有意义吗?
	kobj3->kset = set;
	if (!kobj3)
		// 这里的 return 都是有问题的
		return -ENOMEM;

	// 其实 sysfs_create_groups 就是添加一个数组，而 sysfs_create_group 只是添加
	error = sysfs_create_groups(kobj1, pci_bus_groups);
	if (error)
		// 显然，其实需要释放其他的
		return -ENOMEM;

	error = sysfs_create_group(kobj2, &fw_dev_attr_group);
	if (error)
		return -ENOMEM;

	error = sysfs_create_groups(kobj3, amd_iommu_attr_groups);
	if (error)
		return -ENOMEM;

	// 神奇，这会添加一个目录到 /sys/kernel/obj1 下
	set = kset_create_and_add("test_kset", NULL, kobj1);
	if (!set)
		return -ENOMEM;

	error = sysfs_create_link(&set->kobj, kobj2, "link");
	if (error)
		return -ENOMEM;

	// 这个没有效果，link 只有 set 下才可以创建吗?
	/* error = sysfs_create_link(kobj1, kobj2, "link"); */
	/* if (error) */
	/* 	return -ENOMEM; */


	// 在 obj3 下会多一个 obj4 ，使用两个步骤添加
	kobject_init(&kobj4, &blk_mq_hw_ktype);
	error = kobject_add(&kobj4, kobj3, "obj%u", 4);
	if (error)
		return -EINVAL;

	// 模拟 /sys/class 下一个目录
	// 理解下 kset 下同时有 kobj 的含义
	error = kobject_set_name(&set2.kobj, "%s", "class_name");
	if (error)
		return -EINVAL;
	// 必须有 kobj_type 来初始化:
	// [  456.592993] kobject: must have a ktype to be initialized properly!
	// [  456.593686] kobject init failed
	set2.kobj.kset = set;
	set2.kobj.ktype = &class_ktype;
	error = kset_register(&set2);
	if (error)
		return -EINVAL;

	// 将 samples/kobject/kset-example.c 中 kobject_init_and_add 测试下
	// 效果就是，如果指定了 set 和 ktype ，那么 add 就可以添加到目录下
	kobj5.kset = &set2;
	error = kobject_init_and_add(&kobj5, &class_ktype, NULL, "%s", "kobj5");
	if (error)
		return -EINVAL;
	kobj6.kset = &set2;
	error = kobject_init_and_add(&kobj6, &class_ktype, NULL, "%s", "kobj6");
	if (error)
		return -EINVAL;

	// 终极问题:
	// 既然 kobj 可以构成目录，那么这个目录下的 obj 不就是自动在一个 kset 下吗?
	// 为什么还需要一个 kobj 的概念?
	//
	// 这个问题可以通过 kset_find_obj 的使用者来回答，如果将相关的 kobj 都添加到一个
	// 一个 kset ，那么可以通过遍历这个 kset 来获取到 kobject 的。

	return 0;
}

int test_kobject_exit(void)
{
	/* 1. 似乎不可以 kset_put(set); 正确的是 kset_unregister
	 * 2. 似乎总是 link 之类总是可以自动被解决
	 *
	 * TODO 似乎这里的 free 有顺序问题
	 * */
	kset_unregister(set);
	kset_unregister(&set2);
	kobject_put(kobj1);
	kobject_put(kobj2);
	kobject_put(kobj3);
	kobject_put(&kobj4);
	kobject_put(&kobj5);
	kobject_put(&kobj6);
	return 0;
}

int test_kobject(long action)
{
	switch (action) {
	}
	return 0;
}
