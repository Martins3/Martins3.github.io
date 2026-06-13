#!/usr/bin/env drgn
"""
dump 当前机器 block mq 关键对象关系

用法:
    ./drgn-wrapper.sh -c /proc/kcore ./blk_mq_relationship.py
    ./drgn-wrapper.sh -c /proc/kcore ./blk_mq_relationship.py vda
    ./drgn-wrapper.sh -c /proc/kcore ./blk_mq_relationship.py vda vdb
    ./drgn-wrapper.sh -c /proc/kcore ./blk_mq_relationship.py --all
"""

import sys

from drgn.helpers.linux import for_each_disk, list_for_each_entry

CTX_SAMPLE_LIMIT = 4
RQ_SAMPLE_LIMIT = 3

RQ_STATE_NAMES = {
    0: "IDLE",
    1: "IN_FLIGHT",
    2: "COMPLETE",
}


def ptr(obj):
    return f"{obj.value_():#x}"


def decode_name(char_array):
    return char_array.string_().decode("utf-8", errors="replace")


def rq_state_name(state):
    return RQ_STATE_NAMES.get(state, f"UNKNOWN({state})")


def should_skip_disk(name, include_all, selected):
    if selected:
        return name not in selected
    if include_all:
        return False
    return name.startswith("loop") or name.startswith("ram")


def collect_disks():
    disks = []

    for disk in for_each_disk(prog):
        name = decode_name(disk.disk_name)
        disks.append((name, disk))

    disks.sort(key=lambda item: item[0])
    return disks


def queue_name_map(disks):
    result = {}

    for name, disk in disks:
        if disk.queue.value_():
            result[disk.queue.value_()] = name

    return result


def shared_queue_names(tag_set, qname_by_ptr):
    names = []

    for queue in list_for_each_entry(
        "struct request_queue", tag_set.tag_list.address_of_(), "tag_set_list"
    ):
        qptr = queue.value_()
        name = qname_by_ptr.get(qptr, "unknown")
        names.append(f"{name}({qptr:#x})")

    return names


def summarize_requests(tags):
    allocated = 0
    active = 0
    samples = []
    nr_tags = tags.nr_tags.value_()

    for tag in range(nr_tags):
        rq = tags.rqs[tag]
        if rq.value_() == 0:
            continue

        allocated += 1
        state = rq.state.value_()
        is_active = state != 0 or rq.mq_hctx.value_() != 0
        if is_active:
            active += 1

        if len(samples) < RQ_SAMPLE_LIMIT and (is_active or not samples):
            samples.append((tag, rq))

    return allocated, active, samples


def print_request_sample(queue, hctx, rq_tag, rq):
    state = rq.state.value_()
    print(f"      request[tag={rq_tag}]      {ptr(rq)}")
    print(f"        rq.q                    {ptr(rq.q)} match_queue={rq.q.value_() == queue.value_()}")
    print(f"        rq.mq_ctx               {ptr(rq.mq_ctx)}")
    print(f"        rq.mq_hctx              {ptr(rq.mq_hctx)} match_hctx={rq.mq_hctx.value_() == hctx.value_()}")
    print(f"        rq.state                {rq_state_name(state)}")
    print(f"        rq.tag/internal_tag     {rq.tag.value_()} / {rq.internal_tag.value_()}")
    print(f"        rq.cmd_flags            {rq.cmd_flags.value_():#x}")


def print_ctx_samples(hctx):
    hctx_type = hctx.type.value_()
    nr_ctx = hctx.nr_ctx.value_()
    sample_count = min(nr_ctx, CTX_SAMPLE_LIMIT)

    if sample_count == 0:
        print("      ctxs                     none")
        return

    print(f"      ctxs                     sample={sample_count}/{nr_ctx}")
    for idx in range(sample_count):
        ctx = hctx.ctxs[idx]
        index_hw = ctx.index_hw[hctx_type].value_()
        mapped_hctx = ctx.hctxs[hctx_type]
        print(
            "        "
            f"ctx[{idx}] {ptr(ctx)} "
            f"cpu={ctx.cpu.value_()} "
            f"index_hw={index_hw} "
            f"hctx_match={mapped_hctx.value_() == hctx.value_()}"
        )


def print_hctx(queue, tag_set, hctx_index, hctx):
    tags = hctx.tags
    same_as_tag_set = False
    allocated = 0
    active = 0
    samples = []

    print(f"    hctx[{hctx_index}]                 {ptr(hctx)}")
    print(f"      queue_num                {hctx.queue_num.value_()}")
    print(f"      type                     {hctx.type.value_()}")
    print(f"      nr_ctx                   {hctx.nr_ctx.value_()}")
    print(f"      queue                    {ptr(hctx.queue)} match_queue={hctx.queue.value_() == queue.value_()}")

    if tags.value_():
        same_as_tag_set = bool(
            tag_set.value_()
            and hctx_index < tag_set.nr_hw_queues.value_()
            and tag_set.tags[hctx_index].value_() == tags.value_()
        )
        allocated, active, samples = summarize_requests(tags)

    print(f"      tags                     {ptr(tags)} same_as_tag_set.tags[{hctx_index}]={same_as_tag_set}")
    print(f"      sched_tags               {ptr(hctx.sched_tags)}")

    if tags.value_():
        print(f"      tags.nr_tags             {tags.nr_tags.value_()}")
        print(f"      tags.nr_reserved_tags    {tags.nr_reserved_tags.value_()}")
        print(f"      tags.active_queues       {tags.active_queues.value_()}")
        print(f"      tags.non_null_rqs        {allocated}")
        print(f"      tags.active_rqs          {active}")

    print_ctx_samples(hctx)

    if samples:
        if active == 0:
            print("      request samples         idle/preallocated pool")
        else:
            print("      request samples         active or recently mapped")
        for rq_tag, rq in samples:
            print_request_sample(queue, hctx, rq_tag, rq)


def print_queue_summary(name, disk, qname_by_ptr):
    queue = disk.queue
    tag_set = queue.tag_set

    print(f"[Disk] {name}")
    print(f"  gendisk                   {ptr(disk)}")
    print(f"  request_queue            {ptr(queue)}")
    print(f"    q.nr_hw_queues          {queue.nr_hw_queues.value_()}")
    print(f"    q.nr_requests           {queue.nr_requests.value_()}")
    print(f"    q.queue_depth           {queue.queue_depth.value_()}  <- 注意: 不一定等于 tag_set.queue_depth")
    print(f"    q.elevator              {ptr(queue.elevator)}")
    print(f"    q.queue_ctx             {ptr(queue.queue_ctx)}")
    print(f"    q.sched_shared_tags     {ptr(queue.sched_shared_tags)}")
    print(f"    q.tag_set               {ptr(tag_set)}")

    if tag_set.value_():
        shared = shared_queue_names(tag_set, qname_by_ptr)
        print(f"    tag_set.nr_hw_queues    {tag_set.nr_hw_queues.value_()}")
        print(f"    tag_set.queue_depth     {tag_set.queue_depth.value_()}")
        print(f"    tag_set.reserved_tags   {tag_set.reserved_tags.value_()}")
        print(f"    tag_set.shared_tags     {ptr(tag_set.shared_tags)}")
        print(f"    tag_set.shared_queues   {', '.join(shared) if shared else '(none)'}")

    print("  object graph")
    print(f"    gendisk({name})")
    print(f"      -> request_queue {ptr(queue)}")
    if tag_set.value_():
        print(f"      -> blk_mq_tag_set {ptr(tag_set)}")

    for hctx_index in range(queue.nr_hw_queues.value_()):
        hctx = queue.queue_hw_ctx[hctx_index]
        if hctx.value_() == 0:
            continue
        print_hctx(queue, tag_set, hctx_index, hctx)

    print("")


include_all = False
selected = set()

for arg in sys.argv[1:]:
    if arg == "--all":
        include_all = True
        continue
    selected.add(arg)

all_disks = collect_disks()
qname_by_ptr = queue_name_map(all_disks)

targets = [
    (name, disk)
    for name, disk in all_disks
    if not should_skip_disk(name, include_all, selected)
]

if not targets:
    print("未找到匹配的 block device")
    print(f"可选设备: {', '.join(name for name, _ in all_disks)}")
    sys.exit(1)

print("=== Block MQ Relationships ===")
print("")
print("目标关系:")
print("  gendisk -> request_queue -> blk_mq_tag_set -> blk_mq_hw_ctx[]")
print("                                                -> blk_mq_tags -> request[]")
print("                                                -> blk_mq_ctx[]")
print("")

for name, disk in targets:
    if disk.queue.value_() == 0:
        print(f"[Disk] {name}")
        print("  无 request_queue")
        print("")
        continue

    print_queue_summary(name, disk, qname_by_ptr)
