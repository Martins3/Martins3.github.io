#!/usr/bin/env bash
set -x
set -E -e -u -o pipefail

# copy from : https://access.redhat.com/solutions/2850631

# 基本思路 : 过滤一个类型的 slab ，例如 kmalloc-8k ，然后看这个都是从哪里分配的

if [[ $# -eq 0 ]]; then
	echo "Usage: $0 <slab_cache_name> [timer]"
	echo "Note, the cache names can be found in '/proc/slabinfo'."
	exit 1
fi

SLAB=$1
TIMER=${2-0}
[ "$TIMER" -ge 0 ] 2>/dev/null || TIMER=10

sudo grep -q "^$SLAB" /proc/slabinfo || {
	echo "Error: No $SLAB slab cache exists!"
	exit 2
}

sudo perf probe -d "kmem_cache_alloc_noprof*" || true
sudo perf probe kmem_cache_alloc_noprof 's->name:string'

if grep probe/kmem_cache_alloc /sys/kernel/debug/tracing/kprobe_events; then
	echo "Error: Failed to add the probe!"
	exit 3
fi

echo "Collecting the data for $TIMER seconds, stand by..."
sudo perf record -a -g -e probe:kmem_cache_alloc --filter "name == \"$SLAB\"" sleep "$TIMER"
sudo perf probe -d kmem_cache_alloc* 2>/dev/null

echo "Creating the archive with debugging symbols..."
sudo perf archive
