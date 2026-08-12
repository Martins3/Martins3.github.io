#ifndef LITMUS_LINUX_TOOLS_COMPAT_H
#define LITMUS_LINUX_TOOLS_COMPAT_H

#include <linux/compiler.h>
#include <asm/barrier.h>

/* litmus7 emits __attribute__((noinline)); do not macro-expand its argument. */
#undef noinline

#endif
