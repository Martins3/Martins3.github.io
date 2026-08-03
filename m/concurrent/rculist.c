#include "internal.h"
#include <linux/rculist.h>
#include <linux/jhash.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/rculist_bl.h>
#include <linux/list_bl.h>

static bool should_stop = false;
static LIST_HEAD(g_rcu_list);
struct foo {
	int a;
	int b;
	int c;
	struct list_head list;
};

static void rcu_list_reader(void)
{
	while (should_stop) {
		struct foo *p = NULL;
		rcu_read_lock();
		/*
		 * 可以当只有一个 writer 的时候, 在 rcu_read_lock 的
		 * critical region 中， 可以保证 list 中的元素不会被删除
		 * 
		 * 但是不能保证:
		 * 1. 多个 writer 同时操作 list 不会有问题 (例如，两个相邻的 node 被同时被两个 thread 删)
		 * 2. list 中的元素不会被并发的修改，这是显然的 
		 *
		 * list_for_each_entry_rcu 的也只是增加一些 READ_ONCE 而已
		 */
		list_for_each_entry_rcu(p, &g_rcu_list, list) {
			pr_debug("%s: a = %d, b = %d, c = %d\n", __func__, p->a,
				 p->b, p->c);
			if (p->a == 0 || p->b == 0 || p->c == 0) {
				pr_info("rcu protect failed\n");
				break;
			}
		}
		rcu_read_unlock();
		msleep(1);
	}
}

static void rcu_list_writer(void)
{
	struct foo *new;
	struct foo *p;
	int value = 1;

	pr_info("[%s] start\n", __FUNCTION__);
	while (should_stop) {
		new = (struct foo *)kzalloc(sizeof(struct foo), GFP_KERNEL);
		if (!new) {
			pr_err("kzalloc\n");
			break;
		}
		new->a = 1;
		new->b = 1;
		new->c = 1;

		/*
		* TODO 替换任何一个元素，使用哪一个 api ？
		*/
		p = list_first_or_null_rcu(&g_rcu_list, struct foo, list);
		if (p) {
			pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
			list_replace_rcu(&p->list, &new->list);
		} else {
			list_add_rcu(&new->list, &g_rcu_list);

			// api 定义在 include/linux/rculist.h
			// 和普通的 list 使用没有什么区别
			//
			// list_add_tail_rcu
			// list_del_rcu
		}
		value++;
		synchronize_rcu();
		kfree(p);
	}
	pr_info("[%s] finished with value=%d \n", __FUNCTION__, value);
}

static void test_rcu_list_logic(struct work_struct *work)
{
	struct work *test = (struct work *)work;
	pr_info("run %d\n", test->id);
	switch (test->id) {
	case 0:
		rcu_list_reader();
		break;
	case 1:
		rcu_list_reader();
		break;
	case 2:
		rcu_list_writer();
		break;
	default:
		msleep(3000);
		should_stop = true;
		break;
	}
}

// 模拟 dentry_hashtable 的使用
// 定义哈希表的大小 (2^4 = 16 个桶)
#define HASH_TABLE_BITS 4
struct my_entry {
#define NAME_LEN 32
	char name[NAME_LEN];
	struct hlist_bl_node node; // blocking hlist 节点
	struct rcu_head rcu; // 用于 call_rcu 安全释放内存
	int valid;
};
static struct hlist_bl_head my_hashtable[1 << HASH_TABLE_BITS];

static inline struct hlist_bl_head *e_hash(const char name[NAME_LEN])
{
	u32 hash;
	hash = jhash(name, NAME_LEN, 0);
	return &my_hashtable[hash & ((1 << HASH_TABLE_BITS) - 1)];
}

static void free_entry_callback(struct rcu_head *head)
{
	struct my_entry *entry = container_of(head, struct my_entry, rcu);
	pr_info("RCU callback: 释放 entry '%s'\n", entry->name);
	entry->valid = true;
	kfree(entry);
}

static inline void e_drop(struct my_entry *new_entry)
{
	struct hlist_bl_head *b = e_hash(new_entry->name);
	hlist_bl_lock(b);
	__hlist_bl_del(&new_entry->node);
	hlist_bl_unlock(b);
	call_rcu(&new_entry->rcu, free_entry_callback);
}

static inline void e_add(struct my_entry *new_entry)
{
	struct hlist_bl_head *b = e_hash(new_entry->name);
	hlist_bl_lock(b);
	hlist_bl_add_head_rcu(&new_entry->node, b);
	hlist_bl_unlock(b);
}

static inline struct my_entry *e_lookup(const char name[NAME_LEN])
{
	struct my_entry *entry;
	struct hlist_bl_node *node;
	struct my_entry *found = NULL;
	struct hlist_bl_head *head = e_hash(name);
	rcu_read_lock();
	hlist_bl_for_each_entry_rcu(entry, node, head, node) {
		if (!strcmp(entry->name, name)) {
			found = entry;
			break;
		}
	}
	rcu_read_unlock();
	return found;
}

static int hlist_writer(bool add, const char *name)
{
	struct my_entry *new_entry;
	if (add) {
		new_entry = e_lookup(name);
		if (new_entry)
			return 0;

		new_entry = kmalloc(sizeof(*new_entry), GFP_KERNEL);
		if (!new_entry)
			return -ENOMEM;
		strncpy(new_entry->name, name, NAME_LEN);
		new_entry->valid = true;
		e_add(new_entry);
	} else {
		new_entry = e_lookup(name);
		if (new_entry)
			e_drop(new_entry);
		else
			pr_debug("%s is not found\n", name);
	}
	return 0;
}

static inline int hlist_writer_add(const char *name)
{
	return hlist_writer(true, name);
}

static inline int hlist_writer_remove(const char *name)
{
	return hlist_writer(false, name);
}

static void rcu_hlist_basic_writer(void)
{
	while (should_stop) {
		hlist_writer_add("1");
		hlist_writer_remove("2");
		hlist_writer_remove("2");
		hlist_writer_add("3");
		hlist_writer_remove("1");
	}
}

static void test_hlist_bl_rcu_logic(struct work_struct *work)
{
	struct work *test = (struct work *)work;
	pr_info("run %d\n", test->id);
	switch (test->id) {
	case 0:
		rcu_hlist_basic_writer();
		break;
	case 1:
		rcu_hlist_basic_writer();
		break;
	default:
		msleep(3000);
		should_stop = true;
		break;
	}
}

static void test_hlist_bl_rcu(void)
{
	batch_queue_works(test_hlist_bl_rcu_logic, 3, sizeof(struct work));
	struct my_entry *entry;
	struct hlist_bl_node *node;
	for (int i = 0; i < (1 << HASH_TABLE_BITS); i++) {
		hlist_bl_for_each_entry_rcu(entry, node, &my_hashtable[i],
					    node) {
			kfree(entry);
		}
	}
}

int test_rculist(long action)
{
	should_stop = false;
	switch (action) {
		/*
		 * 其实 hash list 本质和 linked list 没有什么区别，只是先需要获取 hash
		 * 知道是那个 bucket ，bucket 后面还是 linked list
		 */
	case 1:
		return batch_queue_works(test_rcu_list_logic, 4, sizeof(struct work));
	case 2:
		/*
		 * hlist_for_each_entry_rcu 和 hlist_bl_for_each_entry_rcu 
		 * 是类似就不再继续
		 */
		test_hlist_bl_rcu();
		break;
	}
	return 0;
}
