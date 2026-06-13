/*
 * route_seq.c: Sequential Pre-BSD Routing Table
 *
 * This is the baseline single-threaded implementation.
 * It is NOT safe for concurrent access.
 */

#include "common.h"

struct route_entry {
	struct list_head re_next;
	unsigned long addr;
	unsigned long iface;
};

static LIST_HEAD(route_list);

unsigned long route_lookup(unsigned long addr)
{
	struct route_entry *rep;
	struct list_head *p;

	for (p = route_list.next; p != &route_list; p = p->next) {
		rep = container_of(p, struct route_entry, re_next);
		if (rep->addr == addr)
			return rep->iface;
	}
	return ULONG_MAX;
}

int route_add(unsigned long addr, unsigned long interface)
{
	struct route_entry *rep = malloc(sizeof(*rep));
	if (!rep)
		return -ENOMEM;
	rep->addr = addr;
	rep->iface = interface;
	list_add(&rep->re_next, &route_list);
	return 0;
}

int route_del(unsigned long addr)
{
	struct route_entry *rep;
	struct list_head *p;

	for (p = route_list.next; p != &route_list; p = p->next) {
		rep = container_of(p, struct route_entry, re_next);
		if (rep->addr == addr) {
			list_del(&rep->re_next);
			free(rep);
			return 0;
		}
	}
	return -ENOENT;
}

void route_clear(void)
{
	struct route_entry *rep;
	struct list_head *p, *n;

	for (p = route_list.next, n = p->next; p != &route_list; p = n, n = p->next) {
		rep = container_of(p, struct route_entry, re_next);
		free(rep);
	}
	route_list.next = &route_list;
}

static void test_basic(void)
{
	unsigned long ret;

	route_add(42, 1);
	route_add(56, 3);
	route_add(17, 7);

	ret = route_lookup(42);
	printf("lookup(42) = %lu (expect 1) %s\n", ret, ret == 1 ? "OK" : "FAIL");

	ret = route_lookup(56);
	printf("lookup(56) = %lu (expect 3) %s\n", ret, ret == 3 ? "OK" : "FAIL");

	ret = route_lookup(99);
	printf("lookup(99) = %lu (expect ULONG_MAX) %s\n", ret, ret == ULONG_MAX ? "OK" : "FAIL");

	route_del(56);
	ret = route_lookup(56);
	printf("after del, lookup(56) = %lu (expect ULONG_MAX) %s\n", ret, ret == ULONG_MAX ? "OK" : "FAIL");

	route_clear();
}

int main(void)
{
	printf("=== Sequential Pre-BSD Routing Table ===\n");
	test_basic();
	printf("Done.\n");
	return 0;
}
