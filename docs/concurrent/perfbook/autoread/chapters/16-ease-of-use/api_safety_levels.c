/*
 * Demonstration of Rusty Scale concepts for API safety levels.
 *
 * This program shows various API design points from Chapter 16
 * using user-space equivalents of kernel macros/patterns.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*
 * Level 2/3: BUILD_BUG_ON equivalent -- compile-time check.
 * In user space we use _Static_assert (C11).
 */
#define STATIC_BUILD_BUG_ON(cond, msg) _Static_assert(!(cond), msg)

/*
 * Verify structure layout at compile time.
 * This catches ABI breakage early.
 */
struct my_struct {
	uint32_t field_a;
	uint64_t field_b;
	uint16_t field_c;
};

/* Level 2/3: compiler won't let you get it wrong */
STATIC_BUILD_BUG_ON(sizeof(struct my_struct) != 24,
		    "my_struct size unexpected, check padding");
STATIC_BUILD_BUG_ON(offsetof(struct my_struct, field_b) != 8,
		    "field_b offset changed, ABI break!");

/*
 * Level 6: WARN_ON_ONCE equivalent -- runtime warning, but only once.
 * In kernel: WARN_ON_ONCE(condition) warns and dumps stack once.
 */
static int warn_once_state = 0;

#define USER_WARN_ON_ONCE(cond)                                         \
	do {                                                            \
		if ((cond) && !warn_once_state) {                       \
			warn_once_state = 1;                            \
			fprintf(stderr,                                   \
				"[WARN_ONCE] %s:%d: condition '" #cond       \
				"' triggered!\n",                        \
				__FILE__, __LINE__);                  \
		}                                                       \
	} while (0)

/*
 * Level 7: malloc-style API -- follow convention and you get it right.
 * The key convention: check return value, free when done.
 */
static void *safe_malloc(size_t size)
{
	void *p = malloc(size);
	if (!p) {
		fprintf(stderr, "malloc(%zu) failed\n", size);
		exit(EXIT_FAILURE);
	}
	return p;
}

/*
 * Level 15/16: The name tells you how NOT to use it, or the obvious use is wrong.
 * Example: smp_mb() in kernel -- many assume it orders everything against
 * everything, but it only provides pairwise ordering.
 *
 * Here we demonstrate a function whose name sounds stronger than it is.
 */

/*
 * Misleading name: sounds like a full fence across all CPUs and devices,
 * but it is only a compiler barrier in this dummy implementation.
 * This mirrors the confusion around smp_mb() semantics.
 */
#define misleading_full_memory_fence() __asm__ __volatile__("" ::: "memory")

static void demonstrate_mb_confusion(void)
{
	int x = 0, y = 0;

	/*
	 * A developer might write this thinking "full fence" guarantees
	 * no reordering at all.  But on real hardware, smp_mb() only
	 * orders prior stores/loads against subsequent stores/loads
	 * pairwise, not globally.
	 */
	x = 1;
	misleading_full_memory_fence();
	y = 1;

	(void)x;
	(void)y;
}

/*
 * Level 20: gets() -- impossible to get right.
 * gets() is removed from C11; fgets() with explicit size is the safe alternative.
 */

/*
 * Safe wrapper: name tells you the limit.
 */
static void safe_read_line(char *buf, size_t size)
{
	if (!fgets(buf, (int)size, stdin)) {
		buf[0] = '\0';
	}
}

/*
 * Level 4: The simplest use is the correct one.
 * Reference counting with automatic cleanup via cleanup attribute.
 */
struct refcnt {
	int count;
};

static inline void refcnt_get(struct refcnt *r)
{
	__atomic_add_fetch(&r->count, 1, __ATOMIC_RELAXED);
}

static inline void refcnt_put(struct refcnt *r)
{
	if (__atomic_sub_fetch(&r->count, 1, __ATOMIC_RELAXED) == 0) {
		printf("  [refcnt] object freed automatically\n");
	}
}

/*
 * Demonstrate each Rusty Scale level with commentary.
 */
static void demo_build_bug_on(void)
{
	printf("\n--- Rusty Scale Level 2/3: BUILD_BUG_ON equivalent ---\n");
	printf("Compile-time assertions ensure structure layout is correct.\n");
	printf("If someone changes 'struct my_struct', compilation fails.\n");
	printf("This is much better than discovering ABI bugs at runtime.\n");
}

static void demo_warn_on_once(void)
{
	printf("\n--- Rusty Scale Level 6: WARN_ON_ONCE equivalent ---\n");
	printf("Runtime check that warns only once to avoid log spam.\n");

	for (int i = 0; i < 5; i++) {
		USER_WARN_ON_ONCE(i == 2);
	}
	printf("(Only one warning printed despite 5 iterations)\n");
}

static void demo_malloc_convention(void)
{
	printf("\n--- Rusty Scale Level 7: malloc convention ---\n");
	printf("Following convention (check return, free after use) works.\n");

	char *buf = safe_malloc(256);
	strcpy(buf, "hello from safe_malloc");
	printf("Buffer content: %s\n", buf);
	free(buf);
}

static void demo_misleading_name(void)
{
	printf("\n--- Rusty Scale Level 15/16: Misleading API name ---\n");
	printf("smp_mb() sounds like a 'full memory barrier', but its\n");
	printf("actual semantics are pairwise ordering, not global.\n");
	printf("Many developers assume stronger guarantees than provided.\n");
	demonstrate_mb_confusion();
}

static void demo_impossible_to_get_right(void)
{
	printf("\n--- Rusty Scale Level 20: gets() impossible to get right ---\n");
	printf("The C standard removed gets() in C11 because it has no\n");
	printf("way to prevent buffer overflow. Use fgets() with a size.\n");

	char buf[16];
	printf("Enter a short line (safe via fgets): ");
	fflush(stdout);
	safe_read_line(buf, sizeof(buf));
	printf("Read: %s\n", buf);
}

static void demo_simplest_use_correct(void)
{
	printf("\n--- Rusty Scale Level 4: Simplest use is correct ---\n");
	printf("Reference counting: get on use, put when done.\n");

	struct refcnt r = { .count = 1 };
	refcnt_get(&r);
	refcnt_get(&r);
	refcnt_put(&r);
	refcnt_put(&r);
	refcnt_put(&r); /* count reaches 0, object 'freed' */
}

int main(void)
{
	printf("Rusty Scale API Safety Demo -- Chapter 16: Ease of Use\n");
	printf("=======================================================\n");
	printf("\nThis program demonstrates Rusty Russell's API design\n");
	printf("scale using user-space equivalents of kernel patterns.\n");

	demo_build_bug_on();
	demo_warn_on_once();
	demo_malloc_convention();
	demo_simplest_use_correct();
	demo_misleading_name();
	demo_impossible_to_get_right();

	printf("\n=== Summary ===\n");
	printf("Good API design makes misuse difficult or impossible.\n");
	printf("BUILD_BUG_ON catches bugs at compile time.\n");
	printf("WARN_ON_ONCE catches them at runtime without spam.\n");
	printf("Clear names and simple defaults reduce cognitive load.\n");
	printf("Avoid APIs where the obvious use is wrong or dangerous.\n");

	return 0;
}
