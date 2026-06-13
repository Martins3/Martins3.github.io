/*
 * lockdep_demo.c
 *
 * 用户空间演示 lockdep 风格的锁依赖检测原理。
 * 对应第18章提到的 "lock dependency checker" 工具。
 *
 * Linux 内核中 lockdep (kernel/locking/lockdep.c) 运行时维护
 * 一张有向图：如果某任务曾经以 L1 -> L2 的顺序获取锁，
 * 则添加一条边 L1 -> L2。如果后续发现 L2 -> L1 的获取顺序，
 * 则报告潜在死锁（循环依赖）。
 *
 * 本 demo 用极简化代码展示这一核心思想。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_LOCKS 16
#define MAX_EDGES 256

/* ---------------------------------------------------------------
 * 简化版锁依赖图
 * --------------------------------------------------------------- */
struct lock_node {
	const char *name;
};

struct lock_edge {
	int from;
	int to;
};

static struct lock_node nodes[MAX_LOCKS];
static int nr_nodes = 0;

static struct lock_edge edges[MAX_EDGES];
static int nr_edges = 0;

static pthread_mutex_t graph_lock = PTHREAD_MUTEX_INITIALIZER;

/* 查找或注册锁 */
static int lock_id(const char *name)
{
	for (int i = 0; i < nr_nodes; i++)
		if (strcmp(nodes[i].name, name) == 0)
			return i;
	if (nr_nodes >= MAX_LOCKS) {
		fprintf(stderr, "Too many locks\n");
		exit(1);
	}
	nodes[nr_nodes].name = name;
	return nr_nodes++;
}

/* 检查是否已存在从 -> to 的边 */
static int has_edge(int from, int to)
{
	for (int i = 0; i < nr_edges; i++)
		if (edges[i].from == from && edges[i].to == to)
			return 1;
	return 0;
}

/* 添加边 */
static void add_edge(int from, int to)
{
	if (has_edge(from, to))
		return;
	if (nr_edges >= MAX_EDGES) {
		fprintf(stderr, "Too many edges\n");
		exit(1);
	}
	edges[nr_edges].from = from;
	edges[nr_edges].to   = to;
	nr_edges++;
}

/* DFS 检测环 */
static int visited[MAX_LOCKS];
static int rec_stack[MAX_LOCKS];

static int dfs_cycle(int v)
{
	visited[v] = 1;
	rec_stack[v] = 1;

	for (int i = 0; i < nr_edges; i++) {
		if (edges[i].from != v)
			continue;
		int u = edges[i].to;
		if (!visited[u]) {
			if (dfs_cycle(u))
				return 1;
		} else if (rec_stack[u]) {
			return 1; /* 发现环 */
		}
	}

	rec_stack[v] = 0;
	return 0;
}

static int check_deadlock(void)
{
	memset(visited, 0, sizeof(visited));
	memset(rec_stack, 0, sizeof(rec_stack));
	for (int i = 0; i < nr_nodes; i++)
		if (!visited[i] && dfs_cycle(i))
			return 1;
	return 0;
}

/* ---------------------------------------------------------------
 * 运行时记录锁获取顺序（模拟 lockdep 的 __lock_acquire）
 * --------------------------------------------------------------- */
#define MAX_HELD 8
static __thread const char *held_locks[MAX_HELD];
static __thread int nr_held = 0;

void demo_lock(const char *name)
{
	pthread_mutex_lock(&graph_lock);

	int me = lock_id(name);

	/* 记录当前任务持有 me 之前所有已持锁 -> me 的依赖 */
	for (int i = 0; i < nr_held; i++) {
		int from = lock_id(held_locks[i]);
		if (from == me)
			continue;
		/* 如果反向边已存在，说明曾经以 me->from 的顺序获取过 */
		if (has_edge(me, from)) {
			fprintf(stderr,
				"[LOCKDEP] ======================================\n"
				"[LOCKDEP] Possible deadlock detected!\n"
				"[LOCKDEP] Previous order: %s -> %s\n"
				"[LOCKDEP] Current  order: %s -> %s\n"
				"[LOCKDEP] ======================================\n",
				name, held_locks[i],
				held_locks[i], name);
			/* 内核 lockdep 会继续运行并报告更多上下文，这里继续 */
		}
		add_edge(from, me);
	}

	if (check_deadlock()) {
		/* 全局环检测（更严重的场景） */
		fprintf(stderr,
			"[LOCKDEP] Global deadlock cycle detected involving %s\n",
			name);
	}

	pthread_mutex_unlock(&graph_lock);

	if (nr_held < MAX_HELD)
		held_locks[nr_held++] = name;
}

void demo_unlock(const char *name)
{
	(void)name;
	if (nr_held > 0)
		nr_held--;
}

/* ---------------------------------------------------------------
 * 测试场景
 * --------------------------------------------------------------- */
static void scenario_no_deadlock(void)
{
	printf("\n--- Scenario 1: Consistent ordering (A -> B) ---\n");
	/* 线程1: A -> B */
	printf("Thread 1: lock A\n");
	demo_lock("lock_A");
	printf("Thread 1: lock B\n");
	demo_lock("lock_B");
	demo_unlock("lock_B");
	demo_unlock("lock_A");

	/* 线程2: A -> B (相同顺序，无死锁) */
	printf("Thread 2: lock A\n");
	demo_lock("lock_A");
	printf("Thread 2: lock B\n");
	demo_lock("lock_B");
	demo_unlock("lock_B");
	demo_unlock("lock_A");
	printf("Result: No deadlock reported [OK]\n");
}

static void scenario_potential_deadlock(void)
{
	printf("\n--- Scenario 2: Inverted ordering (A -> B vs B -> A) ---\n");
	/* 先重置，模拟全新任务 */
	nr_nodes = 0;
	nr_edges = 0;
	nr_held = 0;

	/* 线程1: A -> B */
	printf("Thread 1: lock A\n");
	demo_lock("lock_A");
	printf("Thread 1: lock B\n");
	demo_lock("lock_B");
	demo_unlock("lock_B");
	demo_unlock("lock_A");

	/* 线程2: B -> A (反向，触发报告) */
	printf("Thread 2: lock B\n");
	demo_lock("lock_B");
	printf("Thread 2: lock A\n");
	demo_lock("lock_A");
	demo_unlock("lock_A");
	demo_unlock("lock_B");
	printf("Result: Deadlock warning expected above [OK]\n");
}

static void scenario_circular_three(void)
{
	printf("\n--- Scenario 3: Circular dependency (A -> B -> C -> A) ---\n");
	nr_nodes = 0;
	nr_edges = 0;
	nr_held = 0;

	/* 线程1: A -> B */
	printf("Thread 1: lock A then B\n");
	demo_lock("lock_A");
	demo_lock("lock_B");
	demo_unlock("lock_B");
	demo_unlock("lock_A");

	/* 线程2: B -> C */
	printf("Thread 2: lock B then C\n");
	demo_lock("lock_B");
	demo_lock("lock_C");
	demo_unlock("lock_C");
	demo_unlock("lock_B");

	/* 线程3: C -> A (闭合环路) */
	printf("Thread 3: lock C then A\n");
	demo_lock("lock_C");
	demo_lock("lock_A");  /* 这里会检测到 A->B->C->A 的环 */
	demo_unlock("lock_A");
	demo_unlock("lock_C");
	printf("Result: Circular deadlock warning expected above [OK]\n");
}

int main(void)
{
	printf("=== lockdep-style deadlock detection demo ===\n");
	printf("This mimics kernel/locking/lockdep.c logic in userspace.\n");

	scenario_no_deadlock();
	scenario_potential_deadlock();
	scenario_circular_three();

	printf("\n--- Summary ---\n");
	printf("Lockdep maintains a directed graph of lock acquisition orders.\n");
	printf("A new edge from L1 -> L2 is added whenever a task holds L1\n");
	printf("and then acquires L2.  If L2 -> L1 already exists, a potential\n");
	printf("deadlock is reported, even if it has not actually triggered yet.\n");
	printf("This is exactly how Linux kernel lockdep works (see check_deadlock()).\n");

	return 0;
}
