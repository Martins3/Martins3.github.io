#!/usr/bin/env drgn
"""
Dump the DMA map ops used by PCI devices.

Usage:
    sudo drgn -c /proc/kcore ./pci_dma_ops.py
    sudo drgn -c /proc/kcore ./pci_dma_ops.py 0000:01:00.0 0000:02:00.0
    sudo drgn -c /proc/kcore ./pci_dma_ops.py --verbose

This mirrors the usual get_dma_ops() path: prefer dev->dma_ops, then show the
arch fallback when the device field is NULL and the fallback pointer is visible.
"""

import argparse

from drgn.helpers.linux import for_each_pci_dev


ARCH_DMA_OPS_SYMBOLS = (
    "dma_ops",
    "dma_direct_ops",
    "kh40000_dma_direct_ops",
    "sw64_dma_direct_ops",
)


def obj_value(obj, default=0):
    if isinstance(obj, int):
        return obj

    try:
        value = obj.value_()
        if not isinstance(value, dict):
            return value
    except Exception:
        pass

    try:
        return obj.address_
    except Exception:
        return default


def ptr(obj):
    return f"{obj_value(obj):#x}"


def c_string(obj, default="?"):
    try:
        return obj.string_().decode("utf-8", errors="replace")
    except Exception:
        return default


def bdf_name(pdev):
    dev_name = c_string(pdev.dev.kobj.name, "")
    if dev_name:
        return dev_name

    bus = c_string(pdev.bus.name).removeprefix("PCI Bus ")
    devfn = pdev.devfn.value_()
    if not bus:
        domain = int_field(pdev.bus, "domain_nr", 0)
        bus = f"{domain:04x}:{pdev.bus.number.value_():02x}"
    return f"{bus}:{devfn >> 3:02x}.{devfn & 7}"


def driver_name(dev):
    try:
        if obj_value(dev.driver) == 0:
            return "(none)"
        return c_string(dev.driver.name)
    except Exception:
        return "?"


def field(obj, name):
    try:
        return getattr(obj, name)
    except Exception:
        return None


def int_field(obj, name, default=None):
    value = field(obj, name)
    if value is None:
        return default
    try:
        return value.value_()
    except Exception:
        return default


def symbol_name(addr):
    value = obj_value(addr)
    if value == 0:
        return "NULL"

    try:
        symbol = prog.symbol(value)
    except Exception:
        return f"{value:#x}"

    name = symbol.name
    if symbol.address == value:
        return name
    return f"{name}+0x{value - symbol.address:x}"


def symbol_with_ptr(addr):
    value = obj_value(addr)
    if value == 0:
        return "NULL"
    return f"{symbol_name(value)}@{value:#x}"


def variable_address(name):
    try:
        var = prog.variable(name)
    except Exception:
        return None

    type_name = str(var.type_)
    if "*" in type_name:
        if obj_value(var) == 0:
            return None
        return var

    try:
        return var.address_of_()
    except Exception:
        return None


def arch_dma_ops():
    for name in ARCH_DMA_OPS_SYMBOLS:
        addr = variable_address(name)
        if addr is not None and obj_value(addr) != 0:
            if name == "dma_ops":
                return addr, "arch dma_ops"
            return addr, f"arch {name}"
    return None, "arch fallback"


def device_dma_ops(dev):
    ops = field(dev, "dma_ops")
    if ops is not None and obj_value(ops) != 0:
        return ops, "dev.dma_ops", ops

    archdata = field(dev, "archdata")
    archdata_ops = field(archdata, "dma_ops") if archdata is not None else None
    if archdata_ops is not None and obj_value(archdata_ops) != 0:
        return archdata_ops, "dev.archdata.dma_ops", ops

    fallback_ops, source = arch_dma_ops()
    if fallback_ops is not None:
        return fallback_ops, source, ops

    if ops is None:
        return None, "no dev.dma_ops field", ops
    return None, "dev.dma_ops is NULL", ops


def iommu_group_summary(dev):
    group = field(dev, "iommu_group")
    if group is None:
        return "group=?"
    if obj_value(group) == 0:
        return "group=NULL"

    try:
        group_id = group.id.value_()
    except Exception:
        group_id = "?"

    default_domain = field(group, "default_domain")
    active_domain = field(group, "domain")
    return (
        f"group={group_id} "
        f"default={domain_summary(default_domain)} "
        f"active={domain_summary(active_domain)}"
    )


def domain_summary(domain):
    if domain is None:
        return "?"
    if obj_value(domain) == 0:
        return "NULL"

    pieces = [ptr(domain)]
    domain_type = int_field(domain, "type")
    if domain_type is not None:
        pieces.append(f"type={domain_type}")

    ops = field(domain, "ops")
    if ops is not None and obj_value(ops) != 0:
        pieces.append(f"ops={symbol_name(obj_value(ops))}")

    return ",".join(pieces)


def pci_id(pdev):
    try:
        return f"{pdev.vendor.value_():04x}:{pdev.device.value_():04x}"
    except Exception:
        return "?:?"


def print_table(rows):
    headers = ("BDF", "PCI_ID", "DRIVER", "DMA_OPS", "SOURCE", "IOMMU")
    widths = [len(header) for header in headers]

    for row in rows:
        for i, item in enumerate(row):
            widths[i] = max(widths[i], len(item))

    fmt = "  ".join(f"{{:<{width}}}" for width in widths)
    print(fmt.format(*headers))
    print(fmt.format(*("-" * width for width in widths)))
    for row in rows:
        print(fmt.format(*row))


def print_verbose(pdev):
    dev = pdev.dev
    ops, source, raw_ops = device_dma_ops(dev)

    print(f"\nPCI {bdf_name(pdev)}")
    print(f"  pci_dev                  {ptr(pdev)}")
    print(f"  struct device            {ptr(dev.address_of_())}")
    print(f"  pci id                   {pci_id(pdev)}")
    print(f"  driver                   {driver_name(dev)}")
    print(f"  raw dev.dma_ops          {symbol_with_ptr(raw_ops) if raw_ops is not None else 'unavailable'}")
    print(f"  effective dma_ops        {symbol_with_ptr(ops)}")
    print(f"  dma_ops source           {source}")
    print(f"  iommu                    {iommu_group_summary(dev)}")

    dma_mask = field(dev, "dma_mask")
    if dma_mask is not None and obj_value(dma_mask) != 0:
        try:
            print(f"  dma_mask                 {dma_mask[0].value_():#x}")
        except Exception:
            print(f"  dma_mask                 ptr={ptr(dma_mask)}")

    coherent_dma_mask = int_field(dev, "coherent_dma_mask")
    if coherent_dma_mask is not None:
        print(f"  coherent_dma_mask        {coherent_dma_mask:#x}")


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bdf", nargs="*", help="PCI BDF, e.g. 0000:01:00.0")
    parser.add_argument("-v", "--verbose", action="store_true", help="print one detailed block per device")
    return parser.parse_args()


def main():
    args = parse_args()
    selected = set(args.bdf)
    rows = []
    seen = set()

    for pdev in sorted(for_each_pci_dev(prog), key=bdf_name):
        name = bdf_name(pdev)
        if selected and name not in selected:
            continue

        seen.add(name)
        if args.verbose:
            print_verbose(pdev)
            continue

        dev = pdev.dev
        ops, source, _raw_ops = device_dma_ops(dev)
        rows.append(
            (
                name,
                pci_id(pdev),
                driver_name(dev),
                symbol_with_ptr(ops),
                source,
                iommu_group_summary(dev),
            )
        )

    if selected:
        missing = selected - seen
        for name in sorted(missing):
            print(f"No matching PCI device: {name}")

    if rows:
        print_table(rows)
    elif not args.verbose:
        print("No matching PCI devices found.")


if __name__ == "__main__":
    main()
