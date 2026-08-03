#include "internal.h"
#include <linux/list_bl.h>

// https://zhjwpku.com/2018/11/20/kernel-data-structure-list-and-hlist.html
// 解释的很好，也就是:
//
// 也就是:
// struct list_head {
//     struct list_head *next, *prev;
// };
//
// Hash List 是为 Hash Table 设计的一种链表，其实上述的双向链表也能实现哈希表，但由于双向链表的头跟其它节点一样，
// 都有两个指针，因此当 bucket 很大的时候，会浪费内存。hlist 实现为以下两种结构体：
//
// struct hlist_head {
//     struct hlist_node *first;
// };
//
// struct hlist_node {
//     struct hlist_node *next, **pprev;
// };
//
//
//
// hlist 用于 hash table
// hlist_bl : bl 指的是 bit lock ，使用 bitlock 是为了性能
//
// TODO 添加一个这个测试:
// include/linux/list_nulls.h
//
// 在这里已经有一个 RCU hlist 测试: concurrent/rculist.c
int test_hlist(long action)
{
	switch (action) {
		break;
	}
	return 0;
}
