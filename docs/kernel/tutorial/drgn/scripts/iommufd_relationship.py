#!/usr/bin/env drgn
"""
Dump iommufd core object relationships for a running userspace process.

Typical host-side usage with a QEMU process using VFIO cdev + iommufd:

    sudo drgn -k ./iommufd_relationship.py --pid $(cat ~/data/hack/vm/yyds-nv/s/pid)
    sudo drgn -k ./iommufd_relationship.py --comm qemu-system-x86_64

The script finds file descriptors whose struct file::private_data is a
struct iommufd_ctx and then walks:

    iommufd_ctx
      -> objects xarray
      -> groups xarray
      -> iommufd_group.pasid_attach
      -> iommufd_attach.hwpt / device_array
      -> IOAS.hwpt_list / io_pagetable.domains
"""

import argparse

from drgn import Object, cast, container_of
from drgn.helpers.linux import for_each_task, list_for_each_entry
from drgn.helpers.linux.xarray import xa_for_each

OBJECT_TYPE_NAMES = {
    0: "ANY/NONE",
    1: "DEVICE",
    2: "HWPT_PAGING",
    3: "HWPT_NESTED",
    4: "IOAS",
    5: "ACCESS",
    6: "FAULT",
    7: "VIOMMU",
    8: "VDEVICE",
    9: "VEVENTQ",
    10: "HW_QUEUE",
    11: "SELFTEST",
}

IOMMU_NO_PASID = 0


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


def ptr_or_null(obj):
    value = obj_value(obj)
    return "0x0" if value == 0 else f"{value:#x}"


def real_ptr(obj):
    value = obj_value(obj)
    return value >= 4096 and value % 4 == 0


def c_string(obj, default="?"):
    try:
        return obj.string_().decode("utf-8", errors="replace")
    except Exception:
        return default


def int_field(obj, name, default=0):
    try:
        return getattr(obj, name).value_()
    except Exception:
        return default


def bool_field(obj, name, default=False):
    try:
        return bool(getattr(obj, name).value_())
    except Exception:
        return default


def refcount_value(ref, default="?"):
    for path in (
        ("refs", "counter"),
        ("refcount", "refs", "counter"),
        ("counter",),
    ):
        cur = ref
        try:
            for name in path:
                cur = getattr(cur, name)
            return cur.value_()
        except Exception:
            continue
    return default


def task_comm(task):
    return c_string(task.comm)


def task_pid(task):
    return int_field(task, "pid", -1)


def fdtable_for_task(task):
    files = task.files
    if obj_value(files) == 0:
        return None
    try:
        return files.fdt
    except Exception:
        return None


def iter_task_files(task, max_fds):
    fdt = fdtable_for_task(task)
    if fdt is None or obj_value(fdt) == 0:
        return

    try:
        nr = min(fdt.max_fds.value_(), max_fds)
        fd_array = fdt.fd
    except Exception:
        return

    for fd in range(nr):
        try:
            filep = fd_array[fd]
        except Exception:
            continue
        if obj_value(filep) == 0:
            continue
        yield fd, filep


def maybe_iommufd_ctx(filep):
    try:
        private_data = filep.private_data
    except Exception:
        return None
    if not real_ptr(private_data):
        return None

    try:
        ictx = cast("struct iommufd_ctx *", private_data)
        if obj_value(ictx.file) != obj_value(filep):
            return None
        return ictx
    except Exception:
        return None


def iter_xarray(xarray):
    try:
        yield from xa_for_each(xarray)
        return
    except Exception:
        pass

    try:
        yield from xa_for_each(xarray.address_of_())
    except Exception:
        return


def obj_type_name(obj):
    return OBJECT_TYPE_NAMES.get(int_field(obj, "type", -1), f"type={int_field(obj, 'type', -1)}")


def dev_name(dev):
    if obj_value(dev) == 0:
        return "(null)"

    for expr in (
        lambda: dev.kobj.name,
        lambda: dev.init_name,
    ):
        try:
            name = c_string(expr(), "")
            if name:
                return name
        except Exception:
            pass
    return "?"


def driver_name(dev):
    try:
        if obj_value(dev.driver) == 0:
            return "(none)"
        return c_string(dev.driver.name)
    except Exception:
        return "?"


def iommu_group_id(group):
    try:
        return group.id.value_()
    except Exception:
        return "?"


def domain_summary(domain):
    if obj_value(domain) == 0:
        return "0x0"

    pieces = [ptr(domain)]
    for name in ("type", "cookie_type"):
        try:
            pieces.append(f"{name}={getattr(domain, name).value_()}")
        except Exception:
            pass
    for name in ("ops", "owner", "iommufd_hwpt"):
        try:
            pieces.append(f"{name}={ptr(getattr(domain, name))}")
        except Exception:
            pass
    return " ".join(pieces)


def pasid_name(pasid):
    return "NO_PASID" if pasid == IOMMU_NO_PASID else str(pasid)


def print_iopt_domains(ioas):
    print("      iopt.domains")
    count = 0
    for domain_id, domain in iter_xarray(ioas.iopt.domains):
        if not real_ptr(domain):
            continue
        count += 1
        print(f"        domain_xa[{domain_id}] {domain_summary(cast('struct iommu_domain *', domain))}")
    if count == 0:
        print("        (none)")


def print_ioas(ioas):
    print(f"    iommufd_ioas            {ptr(ioas)}")
    print(f"      id                    {ioas.obj.id.value_()}")
    print(f"      users/wait_cnt        {refcount_value(ioas.obj.users)} / {refcount_value(ioas.obj.wait_cnt)}")
    print(f"      iopt                  {ptr(ioas.iopt.address_of_())}")
    print(f"      iova_alignment        {ioas.iopt.iova_alignment.value_():#x}")
    print(f"      area_itree.rb_root    {ptr_or_null(ioas.iopt.area_itree.rb_root.rb_node)}")
    print(f"      reserved_itree.root   {ptr_or_null(ioas.iopt.reserved_itree.rb_root.rb_node)}")
    print_iopt_domains(ioas)
    print("      hwpt_list")

    count = 0
    for hwpt in list_for_each_entry(
        "struct iommufd_hwpt_paging", ioas.hwpt_list.address_of_(), "hwpt_item"
    ):
        count += 1
        print(
            "        "
            f"hwpt={ptr(hwpt)} id={hwpt.common.obj.id.value_()} "
            f"domain={ptr(hwpt.common.domain)} "
            f"auto={bool(hwpt.auto_domain.value_())} "
            f"nest_parent={bool(hwpt.nest_parent.value_())}"
        )
    if count == 0:
        print("        (none)")


def print_device(idev):
    dev = idev.dev
    igroup = idev.igroup
    group = igroup.group if obj_value(igroup) else Object(prog, "struct iommu_group *", 0)

    print(f"    iommufd_device         {ptr(idev)}")
    print(f"      id                   {idev.obj.id.value_()}")
    print(f"      users/wait_cnt       {refcount_value(idev.obj.users)} / {refcount_value(idev.obj.wait_cnt)}")
    print(f"      struct device        {ptr(dev)} name={dev_name(dev)} driver={driver_name(dev)}")
    print(f"      iommufd_group        {ptr(igroup)} iommu_group={ptr(group)} id={iommu_group_id(group)}")
    print(f"      enforce_cc           {bool(idev.enforce_cache_coherency.value_())}")
    print(f"      vdevice              {ptr_or_null(idev.vdev)} destroying={bool(idev.destroying.value_())}")


def print_hwpt_paging(hwpt):
    print(f"    iommufd_hwpt_paging    {ptr(hwpt)}")
    print(f"      id                   {hwpt.common.obj.id.value_()}")
    print(f"      users/wait_cnt       {refcount_value(hwpt.common.obj.users)} / {refcount_value(hwpt.common.obj.wait_cnt)}")
    print(f"      ioas                 {ptr(hwpt.ioas)} ioas_id={hwpt.ioas.obj.id.value_() if obj_value(hwpt.ioas) else '?'}")
    print(f"      iommu_domain         {domain_summary(hwpt.common.domain)}")
    print(
        "      flags                "
        f"auto_domain={bool(hwpt.auto_domain.value_())} "
        f"enforce_cc={bool(hwpt.enforce_cache_coherency.value_())} "
        f"nest_parent={bool(hwpt.nest_parent.value_())} "
        f"pasid_compat={bool(hwpt.common.pasid_compat.value_())}"
    )


def print_hwpt_nested(hwpt):
    print(f"    iommufd_hwpt_nested    {ptr(hwpt)}")
    print(f"      id                   {hwpt.common.obj.id.value_()}")
    print(f"      users/wait_cnt       {refcount_value(hwpt.common.obj.users)} / {refcount_value(hwpt.common.obj.wait_cnt)}")
    print(f"      parent               {ptr(hwpt.parent)}")
    print(f"      viommu               {ptr_or_null(hwpt.viommu)}")
    print(f"      iommu_domain         {domain_summary(hwpt.common.domain)}")


def print_generic_object(obj):
    print(f"    {obj_type_name(obj):<21} {ptr(obj)} id={obj.id.value_()}")
    print(f"      users/wait_cnt       {refcount_value(obj.users)} / {refcount_value(obj.wait_cnt)}")


def collect_objects(ictx):
    objects = {}
    for index, entry in iter_xarray(ictx.objects):
        if not real_ptr(entry):
            continue
        try:
            obj = cast("struct iommufd_object *", entry)
            objects[obj.id.value_()] = obj
        except Exception:
            continue
    return objects


def print_object_table(ictx):
    objects = collect_objects(ictx)
    print("  objects")
    if not objects:
        print("    (none)")
        return

    for obj_id in sorted(objects):
        obj = objects[obj_id]
        obj_type = int_field(obj, "type", -1)
        print(f"  - id={obj_id} type={obj_type_name(obj)}")
        try:
            if obj_type == 1:
                print_device(cast("struct iommufd_device *", obj))
            elif obj_type == 2:
                print_hwpt_paging(cast("struct iommufd_hwpt_paging *", obj))
            elif obj_type == 3:
                print_hwpt_nested(cast("struct iommufd_hwpt_nested *", obj))
            elif obj_type == 4:
                print_ioas(cast("struct iommufd_ioas *", obj))
            else:
                print_generic_object(obj)
        except Exception as error:
            print(f"    decode failed          {error}")


def print_attach(attach):
    print(f"        attach              {ptr(attach)} hwpt={ptr_or_null(attach.hwpt)}")
    if obj_value(attach.hwpt):
        try:
            print(f"          hwpt_id           {attach.hwpt.obj.id.value_()}")
            print(f"          domain            {domain_summary(attach.hwpt.domain)}")
        except Exception:
            pass

    print("          devices")
    count = 0
    for dev_id, entry in iter_xarray(attach.device_array):
        if not real_ptr(entry):
            continue
        count += 1
        try:
            idev = cast("struct iommufd_device *", entry)
            print(
                "            "
                f"dev_xa[{dev_id}] idev={ptr(idev)} id={idev.obj.id.value_()} "
                f"name={dev_name(idev.dev)} driver={driver_name(idev.dev)}"
            )
        except Exception as error:
            print(f"            dev_xa[{dev_id}] {ptr(entry)} decode_failed={error}")
    if count == 0:
        print("            (none)")


def print_groups(ictx):
    print("  groups")
    count = 0
    for group_id, entry in iter_xarray(ictx.groups):
        if not real_ptr(entry):
            continue
        count += 1
        try:
            igroup = cast("struct iommufd_group *", entry)
            print(f"  - group_xa[{group_id}]     iommufd_group={ptr(igroup)}")
            print(f"      iommu_group            {ptr(igroup.group)} id={iommu_group_id(igroup.group)}")
            print(f"      ictx                   {ptr(igroup.ictx)} match={obj_value(igroup.ictx) == obj_value(ictx)}")
            print(f"      sw_msi_start           {igroup.sw_msi_start.value_():#x}")
            print("      pasid_attach")
            attach_count = 0
            for pasid, attach_entry in iter_xarray(igroup.pasid_attach):
                if not real_ptr(attach_entry):
                    continue
                attach_count += 1
                print(f"      - pasid={pasid_name(pasid)}")
                print_attach(cast("struct iommufd_attach *", attach_entry))
            if attach_count == 0:
                print("        (none)")
        except Exception as error:
            print(f"    group decode failed     {error}")
    if count == 0:
        print("    (none)")


def print_ctx(task, fd, filep, ictx):
    print(f"\n[iommufd ctx] task={task_comm(task)} pid={task_pid(task)} fd={fd}")
    print(f"  file                      {ptr(filep)}")
    print(f"  ictx                      {ptr(ictx)}")
    print(f"  ictx.file                 {ptr(ictx.file)}")
    print(f"  vfio_ioas                 {ptr_or_null(ictx.vfio_ioas)}")
    print(f"  account/no_iommu          {ictx.account_mode.value_()} / {ictx.no_iommu_mode.value_()}")
    print_object_table(ictx)
    print_groups(ictx)


def selected_task(task, args):
    if args.pid and task_pid(task) not in args.pid:
        return False
    if args.all_tasks or args.pid:
        return True
    return args.comm in task_comm(task)


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pid", type=int, action="append", help="task pid to scan; may be repeated")
    parser.add_argument("--comm", default="qemu-system", help="task comm substring when --pid is not used")
    parser.add_argument("--all-tasks", action="store_true", help="scan every task")
    parser.add_argument("--max-fds", type=int, default=4096, help="maximum file descriptors to scan per task")
    return parser.parse_args()


def main():
    args = parse_args()
    seen = set()

    for task in for_each_task(prog):
        if not selected_task(task, args):
            continue

        for fd, filep in iter_task_files(task, args.max_fds):
            ictx = maybe_iommufd_ctx(filep)
            if ictx is None:
                continue
            key = (task_pid(task), fd, obj_value(ictx))
            if key in seen:
                continue
            seen.add(key)
            print_ctx(task, fd, filep, ictx)

    if not seen:
        print("No iommufd contexts found.")


if __name__ == "__main__":
    main()
