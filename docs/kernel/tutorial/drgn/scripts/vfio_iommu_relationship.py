#!/usr/bin/env drgn
"""
Dump VFIO group, IOMMU group, and IOMMU domain relationships.

Usage:
    sudo drgn -c /proc/kcore ./vfio_iommu_relationship.py
    sudo drgn -c /proc/kcore ./vfio_iommu_relationship.py 0000:01:00.0 0000:02:00.0

When no BDF is given, all PCI devices bound to vfio-pci are printed.
"""

import argparse

from drgn import cast
from drgn.helpers.linux import for_each_pci_dev, list_for_each_entry

VFIO_PCI_DRIVER = "vfio-pci"


def obj_value(obj, default=0):
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
    bus = c_string(pdev.bus.name).removeprefix("PCI Bus ")
    devfn = pdev.devfn.value_()
    return f"{bus}:{devfn >> 3:02x}.{devfn & 7}"


def driver_name(dev):
    try:
        if obj_value(dev.driver) == 0:
            return "(none)"
        return c_string(dev.driver.name)
    except Exception:
        return "?"


def bool_field(obj, name, default=False):
    try:
        return bool(getattr(obj, name).value_())
    except Exception:
        return default


def int_field(obj, name, default=0):
    try:
        return getattr(obj, name).value_()
    except Exception:
        return default


def ptr_field(obj, name):
    try:
        return ptr(getattr(obj, name))
    except Exception:
        return "?"


def domain_summary(domain):
    if obj_value(domain) == 0:
        return "0x0"

    pieces = [ptr(domain)]
    for name in ("type", "cookie_type", "ops", "owner"):
        try:
            field = getattr(domain, name)
        except Exception:
            continue
        if name in ("ops", "owner"):
            pieces.append(f"{name}={ptr(field)}")
        else:
            pieces.append(f"{name}={field.value_()}")
    return " ".join(pieces)


def print_iommu_group(group):
    if obj_value(group) == 0:
        print("  iommu_group              0x0")
        return

    print(f"  iommu_group              {ptr(group)} id={group.id.value_()}")
    print(f"    owner_cnt              {group.owner_cnt.value_()} owner={ptr(group.owner)}")
    print(f"    default_domain         {domain_summary(group.default_domain)}")
    print(f"    active_domain          {domain_summary(group.domain)}")
    print(f"    blocking_domain        {domain_summary(group.blocking_domain)}")
    print("    group devices")

    for group_dev in list_for_each_entry(
        "struct group_device", group.devices.address_of_(), "list"
    ):
        print(
            "      "
            f"{c_string(group_dev.name)} "
            f"group_device={ptr(group_dev)} "
            f"dev={ptr(group_dev.dev)} "
            f"blocked={bool(group_dev.blocked.value_())}"
        )


def print_vfio_device(pdev, containers):
    dev = pdev.dev
    if obj_value(dev.driver_data) == 0:
        print("  vfio_device              no driver_data")
        return

    core = cast("struct vfio_pci_core_device *", dev.driver_data)
    vdev = core.vdev
    group = vdev.group
    container = group.container
    containers[obj_value(container)] = container

    print(f"  vfio_pci_core_device     {ptr(core)}")
    print(f"    vfio_device            {ptr(vdev.address_of_())}")
    print(f"    vfio_device.dev        {ptr(vdev.dev)}")
    print(f"    vfio_group             {ptr(group)}")
    print(
        "    state                  "
        f"index={int_field(vdev, 'index')} "
        f"open_count={int_field(vdev, 'open_count')} "
        f"kvm={ptr_field(vdev, 'kvm')}"
    )
    print(
        "    iommufd                "
        f"device={ptr_field(vdev, 'iommufd_device')} "
        f"attached={bool_field(vdev, 'iommufd_attached')} "
        f"cdev_opened={bool_field(vdev, 'cdev_opened')}"
    )
    print(
        "    vfio_group detail      "
        f"iommu_group={ptr(group.iommu_group)} "
        f"container={ptr(container)} "
        f"container_users={group.container_users.value_()} "
        f"type={group.type.value_()} "
        f"iommufd={ptr(group.iommufd)}"
    )


def print_pci_device(pdev, containers):
    dev = pdev.dev
    print(f"\nPCI {bdf_name(pdev)}")
    print(f"  pci_dev                  {ptr(pdev)}")
    print(f"  struct device            {ptr(dev.address_of_())}")
    print(f"  driver                   {driver_name(dev)}")
    print(f"  driver_data              {ptr(dev.driver_data)}")
    print_iommu_group(dev.iommu_group)

    if driver_name(dev) == VFIO_PCI_DRIVER:
        print_vfio_device(pdev, containers)


def print_container(container):
    if obj_value(container) == 0:
        return

    print(f"\nVFIO container {ptr(container)}")
    print(f"  iommu_driver             {ptr(container.iommu_driver)}")
    print(f"  iommu_data               {ptr(container.iommu_data)}")
    print(f"  noiommu                  {bool(container.noiommu.value_())}")
    print("  container groups")

    for group in list_for_each_entry(
        "struct vfio_group", container.group_list.address_of_(), "container_next"
    ):
        print(
            "    "
            f"vfio_group={ptr(group)} "
            f"iommu_group={ptr(group.iommu_group)} "
            f"id={group.iommu_group.id.value_()} "
            f"container_users={group.container_users.value_()} "
            f"iommufd={ptr(group.iommufd)}"
        )

    try:
        iommu = cast("struct vfio_iommu *", container.iommu_data)
    except Exception as error:
        print(f"  vfio_iommu               unavailable: {error}")
        return

    print(f"  vfio_iommu_type1         {ptr(iommu)}")
    print(
        "    state                  "
        f"dma_avail={iommu.dma_avail.value_()} "
        f"pgsize_bitmap={iommu.pgsize_bitmap.value_():#x} "
        f"num_non_pinned_groups={iommu.num_non_pinned_groups.value_()} "
        f"v2={bool(iommu.v2.value_())}"
    )
    print("    vfio_domain list")

    for domain in list_for_each_entry(
        "struct vfio_domain", iommu.domain_list.address_of_(), "next"
    ):
        print(
            "      "
            f"vfio_domain={ptr(domain)} "
            f"iommu_domain={domain_summary(domain.domain)} "
            f"enforce_cache_coherency={bool(domain.enforce_cache_coherency.value_())}"
        )
        for group in list_for_each_entry(
            "struct vfio_iommu_group", domain.group_list.address_of_(), "next"
        ):
            print(
                "        "
                f"vfio_iommu_group={ptr(group)} "
                f"iommu_group={ptr(group.iommu_group)} "
                f"id={group.iommu_group.id.value_()} "
                f"pinned_page_dirty_scope={bool(group.pinned_page_dirty_scope.value_())}"
            )


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bdf", nargs="*", help="PCI BDF, e.g. 0000:01:00.0")
    parser.add_argument(
        "--all",
        action="store_true",
        help="print all PCI devices instead of only selected/vfio-pci devices",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    selected = set(args.bdf)
    containers = {}
    seen = 0

    for pdev in sorted(for_each_pci_dev(prog), key=bdf_name):
        name = bdf_name(pdev)
        drv = driver_name(pdev.dev)

        if selected and name not in selected:
            continue
        if not selected and not args.all and drv != VFIO_PCI_DRIVER:
            continue

        seen += 1
        print_pci_device(pdev, containers)

    if seen == 0:
        print("No matching PCI devices found.")
        return

    for container in containers.values():
        print_container(container)


if __name__ == "__main__":
    main()
