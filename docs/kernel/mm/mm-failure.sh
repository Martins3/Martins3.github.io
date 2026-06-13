#!/usr/bin/env bash
set -E -e -u -o pipefail

function get_process() {
	pid=$1 # 传入进程ID
	heap_start=0x$(grep heap /proc/"$pid"/maps | cut -d'-' -f1)
	heap_end=0x$(grep heap /proc/"$pid"/maps | cut -d'-' -f2 | cut -d' ' -f1)

	# cat /proc/"$pid"/maps
	# echo $heap_start
	# echo $heap_end

	PAGE_SIZE=$(getconf PAGESIZE)
	for ((ADDR = heap_start; ADDR < heap_end; ADDR = ADDR + PAGE_SIZE)); do
		printf '%s\n' "$ADDR"
		PAGE_NUMBER=$((ADDR / PAGE_SIZE))
		PAGEMAP_ENTRY=$(sudo dd if=/proc/"$pid"/pagemap bs=8 count=1 skip=$PAGE_NUMBER 2>/dev/null \
			| od -t x8 -A n)
		PAGEMAP_ENTRY=$(echo "$PAGEMAP_ENTRY" | xargs)
		echo "$PAGEMAP_ENTRY"
		PFN=$(( PAGEMAP_ENTRY & 0x7fffffffffffff))
		printf "虚拟地址: %x\n" $ADDR
		printf "PFN: %x\n" $PFN
		printf "PFN ADDR: %x\n" $((PFN * PAGE_SIZE))
		break
	done
}

function process_and() {
	pid=62556
	ADDR=0xffff8940b000

pid=66140
ADDR=0xffffa6e7e000
	PAGE_SIZE=$(getconf PAGESIZE)
	PAGE_NUMBER=$((ADDR / PAGE_SIZE))
	PAGEMAP_ENTRY=$(sudo dd if=/proc/"$pid"/pagemap bs=8 count=1 skip=$PAGE_NUMBER 2>/dev/null \
		| od -t x8 -A n)
	PAGEMAP_ENTRY=$(echo "$PAGEMAP_ENTRY" | xargs)
	PFN=$((0x$PAGEMAP_ENTRY & 0x7fffffffffffff))
	printf "虚拟地址: 0x%x\n" $ADDR
	printf "PFN: 0x%x\n" $PFN
	printf "PFN ADDR: 0x%x\n" $((PFN * PAGE_SIZE))

}
process_and

# echo 0x17bcde | sudo tee /sys/kernel/debug/hwpoison/corrupt-pfn
