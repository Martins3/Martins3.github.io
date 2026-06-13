#!/usr/bin/env drgn
"""
IRQ 层级分析脚本 - 遍历 irq_desc -> irq_data -> parent_data 链

使用方法:
    drgn -s /path/to/vmlinux -c /proc/kcore ./irq_hierarchy.py <virq>

    或者通过 wrapper:
    ./drgn-wrapper.sh -c /proc/kcore ./irq_hierarchy.py <virq>

参数:
    virq: 虚拟中断号 (例如: 16, 32, 100 等)

示例输出:
    === IRQ 层级分析: virq=100 ===

    [Level 0] Current Domain
      irq_desc: 0xffff888100123456
      virq: 100
      domain: gpio_domain (GPIO Controller)
      hwirq: 5
      chip: gpio_chip

    [Level 1] Parent Domain
      domain: gic_domain (ARM GIC)
      hwirq: 32
      chip: gic_chip

    [Level 2] Root Domain
      domain: gic_domain (ARM GIC) - Root
      hwirq: 32
      chip: gic_chip
"""

import sys
from drgn import Object, cast, container_of, NULL
from drgn.helpers.linux import (
    list_for_each_entry,
    radix_tree_lookup,
    mtree_load,
)

def get_irq_desc(prog, virq):
    """获取指定 virq 的 irq_desc - 使用 sparse_irqs maple tree"""
    try:
        sparse_irqs = prog.variable("sparse_irqs")
        desc_ptr = mtree_load(sparse_irqs, virq)
        if desc_ptr.value_() != 0:
            # mtree_load 返回 void*，需要转换为 struct irq_desc*
            return cast("struct irq_desc *", desc_ptr)
    except Exception as e:
        print(f"mtree_load() failed: {e}")

    return None

def get_domain_name(domain):
    """获取 domain 名称"""
    if domain == NULL(prog, "struct irq_domain *"):
        return "NULL"
    try:
        name = domain.name.string_().decode('utf-8', errors='replace')
        return name
    except:
        return "unknown"

def get_chip_name(chip):
    """获取 chip 名称"""
    if chip == NULL(prog, "struct irq_chip *"):
        return "NULL"
    try:
        name = chip.name.string_().decode('utf-8', errors='replace')
        return name
    except:
        return "unknown"

def print_irq_data_info(irq_data, level, is_root=False):
    """打印 irq_data 信息"""
    prefix = f"[Level {level}]"
    if is_root:
        suffix = " - Root (no parent)"
    else:
        suffix = ""

    domain = irq_data.domain
    chip = irq_data.chip

    domain_name = get_domain_name(domain)
    chip_name = get_chip_name(chip)
    hwirq = irq_data.hwirq.value_()

    print(f"\n{prefix} Domain: {domain_name}{suffix}")
    print(f"  irq_data: 0x{irq_data.address_:x}")
    print(f"  irq_common_data: 0x{irq_data.common.address_:x}")
    print(f"  hwirq: {hwirq}")
    print(f"  chip: {chip_name}")

    # 打印 domain 标志
    if domain.value_() != 0:
        flags = domain.flags.value_()
        print(f"  domain flags: 0x{flags:x}")

        # 检查层级标志
        IRQ_DOMAIN_FLAG_HIERARCHY = 1 << 0
        if flags & IRQ_DOMAIN_FLAG_HIERARCHY:
            print(f"    - IRQ_DOMAIN_FLAG_HIERARCHY")

    return domain

def traverse_irq_hierarchy(prog, virq):
    """遍历 IRQ 层级结构"""
    print(f"=== IRQ 层级分析: virq={virq} ===")
    print("=" * 60)

    # 获取 irq_desc
    irq_desc = get_irq_desc(prog, virq)
    if irq_desc is None:
        print(f"错误: 无法找到 irq_desc[{virq}]")
        return

    print(f"\nirq_desc: 0x{irq_desc.value_():x}")
    print(f"virq: {virq}")

    # 检查是否有 action
    action = irq_desc.action
    if action != NULL(prog, "struct irqaction *"):
        try:
            action_name = action.name.string_().decode('utf-8', errors='replace')
            print(f"action name: {action_name}")
        except:
            print(f"action: 0x{action.value_():x} (name unavailable)")
    else:
        print("action: NULL (no handler registered)")

    # 从 irq_desc 获取内嵌的 irq_data
    # 注意：irq_desc.irq_data 是内嵌结构体，不是指针
    # 使用 address_ 属性获取其地址
    irq_data = irq_desc.irq_data

    print("\n" + "-" * 60)
    print("遍历 irq_data -> parent_data 链:")
    print("-" * 60)

    level = 0
    visited = set()  # 防止循环

    while True:
        # 获取当前 irq_data 的地址（用于循环检测）
        irq_data_addr = irq_data.address_

        # 检查循环
        if irq_data_addr in visited:
            print(f"\n[警告] 检测到循环! irq_data 0x{irq_data_addr:x} 已访问过")
            break
        visited.add(irq_data_addr)

        # 检查是否有 parent_data
        try:
            parent_data = irq_data.parent_data
            has_parent = parent_data.value_() != 0
        except:
            has_parent = False

        # 打印当前 irq_data 信息
        print_irq_data_info(irq_data, level, is_root=not has_parent)

        # 移动到 parent
        if has_parent:
            # parent_data 是指针，需要解引用
            irq_data = parent_data[0]
            level += 1

            # 安全检查，防止无限循环
            if level > 10:
                print("\n[警告] 层级太深 (>10)，可能存在异常")
                break
        else:
            break

def list_all_irqs(prog):
    """列出系统中所有 irq_desc 的基本信息"""
    print("=== 系统中所有 IRQ 概要 ===")
    print()

    # 获取 NR_IRQS
    try:
        nr_irqs = prog.variable("NR_IRQS").value_()
    except:
        nr_irqs = 1024  # 默认值

    print(f"NR_IRQS: {nr_irqs}")
    print()

    # 显示前 32 个和已注册的中断
    count = 0
    max_show = 32

    for i in range(min(nr_irqs, 1024)):  # 限制最多检查1024个
        try:
            desc = get_irq_desc(prog, i)
            if desc is None:
                continue

            action = desc.action

            # 只显示有 handler 的前32个，以及所有有 handler 的
            if action != NULL(prog, "struct irqaction *") or i < max_show:
                irq_data = desc.irq_data
                domain = irq_data.domain
                hwirq = irq_data.hwirq.value_()

                domain_name = get_domain_name(domain)

                if action != NULL(prog, "struct irqaction *"):
                    try:
                        name = action.name.string_().decode('utf-8', errors='replace')
                        status = f"[{name}]"
                    except:
                        status = "[handler]"
                else:
                    status = "(no handler)"

                print(f"  virq {i:4d}: hwirq={hwirq:4d} domain={domain_name:20s} {status}")
                count += 1

                if count >= 50:  # 最多显示50个
                    print("  ... (更多中断省略)")
                    break

        except Exception as e:
            continue

def main():
    # drgn 脚本会自动提供 prog 变量
    global prog

    # 检查参数
    if len(sys.argv) < 2:
        print("IRQ 层级分析脚本")
        print()
        print("用法:")
        print(f"  drgn -c /proc/kcore {sys.argv[0]} <virq>")
        print(f"  drgn -c /proc/kcore {sys.argv[0]} --list")
        print()
        print("参数:")
        print("  virq    : 虚拟中断号 (数字)")
        print("  --list  : 列出所有中断概要")
        print()
        print("示例:")
        print(f"  drgn -c /proc/kcore {sys.argv[0]} 16")
        print(f"  drgn -c /proc/kcore {sys.argv[0]} --list")
        sys.exit(1)

    arg = sys.argv[1]

    if arg == "--list" or arg == "-l":
        list_all_irqs(prog)
    else:
        try:
            virq = int(arg)
            traverse_irq_hierarchy(prog, virq)
        except ValueError:
            print(f"错误: 无效的 virq 参数 '{arg}'")
            print("请提供一个数字作为虚拟中断号，或使用 --list 列出所有中断")
            sys.exit(1)

if __name__ == "__main__":
    main()
