#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import platform
import re
import shlex
import shutil
import socket
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Protocol, Sequence

from commands import CommandRunner
from disks import (
    DEFAULT_BOOT_SIZE,
    create_missing_disk,
)
from errors import ColleiError, ColleiHelp, UnsupportedNativeConfiguration
from host_setup import (
    prepare_native_host,
    prepare_novnc,
    prepare_ovs_tap,
)
from kernel import kernel_image
from launch_options import LaunchOptions
from qemu import QemuCommand
from runtime import ColleiContext, VmRuntime
from tasks import add_background_task, exec_task_follow
from ui import print_banner
from vfio import pci_bind_to_vfio
from virtme import VirtmeSetup

# collei.py 只负责启动虚拟机。
#
# collei-action.py 负责操作单个虚拟机，collei-global.py 负责操作全部虚拟机。
# 启动流程必须保留 collei.sh 中便于内核/KVM 调试的 setup_* 结构；迁移期间尚未
# 改造的 setup_* 仍由原脚本编译参数，不能用一个“通用配置”把特殊调试模式抹掉。


class QemuProfile(Protocol):
    def kernel_args(self) -> str: ...

    @property
    def initramfs(self) -> Path | None: ...

    def rootfs_arguments(self) -> tuple[str, ...]: ...

    def manual_console_arguments(self) -> tuple[str, ...]: ...

    def mode(self) -> str: ...


def validate_launch_options(vm: VmRuntime, options: LaunchOptions) -> None:
    if options.nbd_target and options.migration != "defer":
        raise ColleiError("--nbd-target requires deferred incoming migration")
    if options.migration != "defer":
        return
    passthrough_options = tuple(
        name for name in ("vfio", "sriov") if vm.config.options.enabled(name)
    )
    if not passthrough_options:
        return
    configured = ", ".join(f"opt/{name}" for name in passthrough_options)
    raise ColleiError(
        f"rk -a does not support passthrough VM {vm.config.name}: {configured}"
    )


def print_help() -> None:
    print(
        """usage: collei.py [OPTIONS]

启动当前默认 VM。默认 VM 由 ~/.config/collei/last 指向，VM 配置位于 opt/。

迁移和恢复：
  -a  启动普通热迁移目标
      使用 -incoming defer -only-migratable；必须已经有且只有一个 source QEMU。
      配置了 opt/vfio 或 opt/sriov 的直通 VM 会直接报错。
      --nbd-target  内部选项：目标 QEMU 使用独立存储 slot，由 migrate_nbd 调用。
  -l  从当前 VM 目录的 vmstate.img 恢复
      对应 save_vm_file，启用 mapped-ram 后使用
      -incoming file:... -only-migratable。
  -L  启动 CPR reboot 恢复目标
      对应 save_vm_cpr/load_vm_cpr，使用 -incoming defer -only-migratable。
  -T  启动 CPR transfer 目标
      监听 tcp:0:44444 和 /tmp/cpr.sock；必须已经有且只有一个 source QEMU。

CPR 有三种模式：
  cpr-transfer  类似热迁移，但 VM 状态和内存都通过迁移通道传输。
  cpr-exec      基于 exec，只替换 QEMU binary，不需要启动另一个目标。
  cpr-reboot    将状态保存后，再通过 -L 启动目标恢复。

调试：
  -d  使用 GDB 启动 QEMU 或 Firecracker，并保持在前台
  -s  启动后暂停 guest，通过 VM 目录下的 gdb.socket 调试 guest kernel
      --foreground  强制在前台运行，供需要直接管理 QEMU 生命周期的 action 使用
  -E  启动 EFI application 测试
      要求 opt/bios=ovmf，并且不能配置 opt/kernel 直接启动内核。

创建 VM：
      collei-install.py -i/-x/-v/-V  只构建 VM 目录并更新默认 VM，不启动 QEMU

检查：
      --dry-run  只构造并打印命令，不准备 host、不改写文件、不启动 VM
  -h, --help           显示此帮助

常用示例：
  collei.py                  启动当前 VM
  collei.py -s               等待 GDB 调试 guest kernel
  collei.py -a               启动另一个 QEMU 作为普通热迁移目标
  collei.py --dry-run        检查生成的完整命令"""
    )


def _disk_format(path: Path) -> str:
    # 不用 qemu-img info；如果 QEMU 正在运行，会有 file lock 的问题。
    with path.open("rb") as image:
        return "qcow2" if image.read(4) == b"QFI\xfb" else "raw"


def _initramfs(kernel_dir: Path) -> Path:
    matches = sorted(kernel_dir.parent.glob(f"*{kernel_dir.name}.raw.zst"))
    if len(matches) != 1:
        raise UnsupportedNativeConfiguration(
            f"expected one initramfs for {kernel_dir.name}, found {len(matches)}"
        )
    return matches[0]


def _host_iommu_group(device: str) -> str | None:
    group = Path("/sys/bus/pci/devices") / device / "iommu_group"
    try:
        return group.resolve(strict=True).name
    except OSError:
        return None


def _boot_disks(vm: VmRuntime) -> list[tuple[str, str, str | None]]:
    value = vm.config.options.get("disk")
    if value is None:
        raise UnsupportedNativeConfiguration(
            "Python setup_basic_storage requires opt/disk"
        )
    result: list[tuple[str, str, str | None]] = []
    for line in value.splitlines():
        fields = line.split()
        if len(fields) not in {2, 3}:
            raise UnsupportedNativeConfiguration(f"invalid disk layout: {line}")
        name, drive = fields[:2]
        if drive not in {"virtio-blk", "virtio-scsi", "nvme", "ide"}:
            raise UnsupportedNativeConfiguration(
                f"Python setup_basic_storage does not support drive={drive}"
            )
        result.append((name, drive, fields[2] if len(fields) == 3 else None))
    return result


@dataclass(frozen=True)
class ColleiQemuBuilder:
    """按原 collei.sh setup_* 顺序构造 QEMU argv。"""

    context: ColleiContext
    vm: VmRuntime
    profile: QemuProfile | None = None
    sriov_vf: str | None = None
    efi_application: bool = False
    dry_run: bool = False

    @property
    def image_dir(self) -> Path:
        return self.vm.image_directory

    @property
    def monitor_dir(self) -> Path:
        return self.vm.directory / self.vm.which_qemu

    def _ensure_disk(self, path: Path, size: str = "10G", fmt: str = "qcow2") -> None:
        """非 dry-run 时幂等创建缺失的磁盘镜像。"""
        if not self.dry_run:
            create_missing_disk(CommandRunner(), path, size, fmt)

    def _add_disk(
        self,
        argv: list[str],
        name: str,
        *devices: str,
        path: Path | None = None,
        size: str = "10G",
        fmt: str | None = None,
        aio: bool = False,
        read_only: bool = False,
        create: bool = True,
    ) -> None:
        """创建磁盘(幂等)、追加 -device 和稳定的 file + format -blockdev。

        node-name 固定为 <name>-file / <name>,source、target、NBD export、
        mirror job 都能引用同一个逻辑名字。path 缺省为 image_dir / name;
        create=False 用于 ISO 等必须已存在、不能自动创建的后端。
        """
        disk = path if path is not None else self.image_dir / name
        if create:
            self._ensure_disk(disk, size, fmt or "qcow2")

        for device in devices:
            argv.extend(["-device", device])
        if fmt is None:
            # 与 _ensure_disk 默认创建的格式一致;文件已存在时探测真实格式。
            fmt = _disk_format(disk) if disk.exists() else "qcow2"
        file_opts = f"driver=file,node-name={name}-file,filename={disk}"
        if aio:
            file_opts += ",aio=native,cache.direct=on"
        format_opts = f"driver={fmt},node-name={name},file={name}-file,discard=unmap"
        if read_only:
            file_opts += ",read-only=on"
            format_opts += ",read-only=on"
        argv.extend(["-blockdev", file_opts, "-blockdev", format_opts])

    def _ensure_nbd_export(self, index: int, backing: Path) -> Path:
        """用 qemu-nbd 把 multipath 后端文件导出到 unix socket，幂等。"""
        sock = self.monitor_dir / f"mpath{index}.nbd"
        if self.dry_run or self._nbd_alive(sock):
            return sock
        sock.unlink(missing_ok=True)
        qemu_nbd = self.context.repo.parent.parent / "qemu" / "build" / "qemu-nbd"
        # 多个 qemu-nbd 导出同一文件，locking=off 绕开镜像写锁冲突
        image_opts = (
            f"driver=raw,file.driver=file,file.filename={backing},file.locking=off"
        )
        CommandRunner().run(
            [qemu_nbd, "--fork", f"--socket={sock}", "--image-opts", image_opts]
        )
        for _ in range(50):
            if self._nbd_alive(sock):
                return sock
            time.sleep(0.1)
        raise ColleiError(f"qemu-nbd socket not ready: {sock}")

    @staticmethod
    def _nbd_alive(sock: Path) -> bool:
        try:
            socket.socket(socket.AF_UNIX).connect(str(sock))
        except OSError:
            return False
        return True

    def boot_disks(self) -> list[tuple[str, str, str | None]]:
        if self.vm.config.options.get("disk") is not None:
            return _boot_disks(self.vm)
        disk_num = self.vm.config.options.get("disk_num")
        if disk_num is not None:
            try:
                count = int(disk_num)
            except ValueError as error:
                raise UnsupportedNativeConfiguration(
                    f"invalid opt/disk_num={disk_num}"
                ) from error
            return [
                (f"boot{index}", "virtio-blk", "1" if index == 1 else None)
                for index in range(1, count + 1)
            ]
        # 只验证安装介质启动时可以没有目标盘；后续可通过 action 添加磁盘。
        if self.vm.config.options.get("iso") is not None:
            return []
        if self.profile is not None and self.profile.mode() == "vmtest":
            disks = sorted(path.name for path in self.image_dir.glob("boot[1-9]"))
            if disks:
                return [
                    (name, "virtio-blk", str(index))
                    for index, name in enumerate(disks, 1)
                ]
        raise UnsupportedNativeConfiguration(
            "Python setup_basic_storage requires opt/disk"
        )

    def validate(self) -> None:
        if platform.machine() != "x86_64":
            raise UnsupportedNativeConfiguration(
                "Python setup_* currently supports x86_64"
            )
        if self.efi_application:
            if self.vm.config.options.get("bios") != "ovmf":
                raise UnsupportedNativeConfiguration(
                    "-E requires opt/bios=ovmf, matching collei.sh"
                )
            if self.vm.config.options.get("kernel") is not None:
                raise UnsupportedNativeConfiguration(
                    "-E cannot be combined with direct kernel boot"
                )
        bridge = self.context.global_config.directory.get("bridge") or "ovs"
        if bridge not in {"ovs", "no"}:
            raise UnsupportedNativeConfiguration(
                f"Python setup_network does not support bridge={bridge}"
            )
        bios = self.vm.config.options.get("bios")
        if bios not in {
            None,
            "seabios",
            "ovmf",
            "ovmf_binary",
            "ovmf_binary_secure",
        }:
            raise UnsupportedNativeConfiguration(f"unsupported bios={bios}")
        virtio_blk = self.vm.config.options.get("virtio_blk")
        if virtio_blk not in {None, "1"}:
            raise UnsupportedNativeConfiguration(f"unsupported virtio_blk={virtio_blk}")
        display = self.vm.config.options.get("display")
        if display not in {None, "virtio-gpu"}:
            raise UnsupportedNativeConfiguration(f"unsupported display={display}")

    def build(self) -> QemuCommand:
        self.validate()
        qemu = self.context.repo.parent.parent / "qemu" / "build" / "qemu-system-x86_64"
        argv = [str(qemu)]
        self.setup_storage(argv)
        self.setup_mem_cpu(argv)
        self.setup_basic_storage(argv)
        self.setup_kernel(argv)
        self.setup_network(argv)
        self.setup_vsock(argv)
        self.setup_hct(argv)
        self.setup_machine(argv)
        self.setup_monitor(argv)
        self.setup_initrd(argv)
        self.setup_balloon(argv)
        self.setup_bios(argv)
        self.setup_vfio(argv)
        self.setup_fs_share(argv)
        self.setup_iso(argv)
        self.setup_ipmi(argv)
        self.setup_accel(argv)
        self.setup_edu(argv)
        self.setup_pidfile(argv)
        self.setup_cpu_model(argv)
        self.setup_display_and_chardev(argv)
        self.setup_audio(argv)
        self.setup_pcie_port(argv)
        self.setup_rng(argv)
        self.setup_misc(argv)
        self.setup_uuid(argv)
        self.setup_input_and_usb(argv)
        self.setup_trace(argv)
        return QemuCommand(tuple(argv))

    def setup_storage(self, argv: list[str]) -> None:
        # 也许这是最佳的办法了：
        # 1. 让 virtio-scsi 作为 scsi1.0。
        # 2. 所有 channel=0，然后用 lun 区分设备。
        argv.extend(["-device", "virtio-scsi,id=scsi1"])
        for index in (1, 2):
            disk_id = f"virtio-scsi_{index}"
            self._add_disk(
                argv,
                disk_id,
                f"scsi-hd,drive={disk_id},bus=scsi1.0,channel=0,scsi-id={index},lun=0,id={disk_id}",
            )

        if self.vm.config.options.get("virtio_blk") == "1":
            # iothread 需要和具体 virtio-blk 设备绑定。
            argv.extend(["-object", "iothread,id=virtio_blk_io0"])
            self._add_disk(
                argv,
                "virtio_blk_1",
                "virtio-blk,drive=virtio_blk_1,id=virtio_blk_1,iothread=virtio_blk_io0,num-queues=2",
                aio=True,
            )
        self.setup_multipath(argv)
        self.setup_nvme(argv)
        self.setup_nvme_host(argv)
        self.setup_sata(argv)
        self.setup_qemu_storage_daemon(argv)

    def setup_multipath(self, argv: list[str]) -> None:
        if self.vm.config.options.get("multipath") != "1":
            return
        # guest 内 dm-multipath 测试：两条路径指向同一个 raw 后端文件，
        # 相同 serial 让 guest 识别为同一 LUN。
        backing = self.image_dir / "multipath_backing"
        self._ensure_disk(backing, size="25G", fmt="raw")
        for index in (1, 2):
            disk_id = f"multipath_{index}"
            # iothread 线程名是 "IO <id>"，pthread 名字上限 15 字符，超了命名会静默失败
            io_id = f"mp_io{index}"
            nbd_sock = self._ensure_nbd_export(index, backing)
            argv.extend(
                [
                    "-object",
                    f"iothread,id={io_id}",
                    "-device",
                    f"virtio-scsi-pci,id={disk_id}_hba,iothread={io_id}",
                    "-device",
                    f"scsi-hd,drive={disk_id},bus={disk_id}_hba.0,channel=0,scsi-id=0,lun=0,serial=MULTIPATH,id={disk_id}",
                    "-blockdev",
                    f"driver=nbd,node-name={disk_id},server.type=unix,server.path={nbd_sock},cache.direct=on,discard=unmap",
                ]
            )

    def setup_nvme(self, argv: list[str]) -> None:
        mode = self.vm.config.options.get("nvme")
        if mode is None:
            return
        if mode == "basic":
            self.setup_nvme_basic(argv)
        elif mode == "multipath":
            self.setup_nvme_multipath(argv)
        elif mode == "sriov":
            self.setup_nvme_sriov(argv)
        elif mode == "many":
            self.setup_many_nvme(argv)
        else:
            raise UnsupportedNativeConfiguration(f"unsupported nvme={mode}")

    def setup_nvme_basic(self, argv: list[str]) -> None:
        # serial 是 NVMe 控制器身份；重复 serial 会导致 guest 拒绝控制器。
        for index in (1, 2):
            serial = f"collei-{self.vm.config.guest_id:04d}-{index}"
            self._add_disk(
                argv,
                f"nvme_basic{index}",
                f"nvme,drive=nvme_basic{index},max_ioqpairs=14,serial={serial},id=nvme_b{index}",
                aio=True,
            )

    def setup_nvme_multipath(self, argv: list[str]) -> None:
        # NVMe multipath 是同一 subsystem 内的多个 controller 共享一个
        # namespace；每条 path 不能创建独立的 drive/nvme-ns。
        self._add_disk(
            argv,
            "nvme_mpath",
            "nvme-subsys,id=nvme-subsys-0,nqn=subsys0",
            "nvme,serial=deadbeef,subsys=nvme-subsys-0,id=nc1",
            "nvme,serial=deadbeef,subsys=nvme-subsys-0,id=nc2",
            "nvme-ns,drive=nvme_mpath,bus=nc1,nsid=1,shared=on",
            path=self.image_dir / "nvme1",
        )

    def setup_nvme_sriov(self, argv: list[str]) -> None:
        self._add_disk(
            argv,
            "nvme3",
            "pcie-root-port,slot=3,id=pcie_port.3",
            "nvme-subsys,id=subsys0",
            "nvme,serial=deadbeef,subsys=subsys0,sriov_max_vfs=1,sriov_vq_flexible=2,sriov_vi_flexible=1,bus=pcie_port.3",
            "nvme-ns,drive=nvme3,nsid=1",
            path=self.image_dir / "nvme1",
        )

    def setup_many_nvme(self, argv: list[str]) -> None:
        for index in range(10):
            # NVMe 不支持 iothread；每个控制器使用独立 backing file。
            # serial 必须确定性生成，source 和 target 才一致。
            serial = f"collei-{self.vm.config.guest_id:04d}-many{index}"
            self._add_disk(
                argv,
                f"many_nvme{index}",
                f"nvme,drive=many_nvme{index},max_ioqpairs=96,serial={serial}",
            )

    def setup_nvme_host(self, argv: list[str]) -> None:
        device = self.vm.config.options.get("nvme_host")
        if device is None:
            return
        # block/nvme.c 使用物理 NVMe 作为后端，再向 guest 暴露 virtio-blk。
        argv.extend(
            [
                "-blockdev",
                f"driver=nvme,node-name=disk_backend,device={device},namespace=1",
                "-device",
                "virtio-blk-pci,drive=disk_backend",
            ]
        )

    def setup_qemu_storage_daemon(self, argv: list[str]) -> None:
        if self.vm.config.options.get("qsd") is None:
            return
        disk = self.image_dir / "img_qsd"
        self._ensure_disk(disk)
        socket = self.monitor_dir / "qsd.sock"
        if not self.dry_run:
            socket.unlink(missing_ok=True)
            qsd = (
                self.context.repo.parent.parent
                / "qemu/build/storage-daemon/qemu-storage-daemon"
            )
            add_background_task(
                self.context,
                CommandRunner(),
                [
                    qsd,
                    "--blockdev",
                    f"driver=file,node-name=file,filename={disk}",
                    "--export",
                    f"type=vhost-user-blk,id=export,node-name=file,addr.type=unix,addr.path={socket},num-queues=1,writable=on",
                    "--pidfile",
                    self.vm.directory / self.vm.which_qemu / "qsd.pid",
                ],
                vm=self.vm,
                group="qemu",
                label="qsd",
            )
        # 内存必须是共享 memfd，不然 vhost-user 恢复 vring 会失败。
        argv.extend(
            [
                "-chardev",
                f"socket,id=qsd,path={socket},reconnect=10",
                "-device",
                "vhost-user-blk-pci,id=blk1,num-queues=1,chardev=qsd",
            ]
        )

    def setup_sata(self, argv: list[str]) -> None:
        if not self.vm.config.options.enabled("sata"):
            return
        # AHCI 模式下，两个 ide-hd 分别挂到 ahci.0 和 ahci.1。
        argv.extend(["-device", "ahci,id=ahci"])
        for index in (1, 2):
            self._add_disk(
                argv,
                f"sata{index}",
                f"ide-hd,drive=sata{index},bus=ahci.{index - 1}",
            )

    def setup_basic_storage(self, argv: list[str]) -> None:
        # 自动解析 opt/disk；默认不配置 bootindex。
        boot_disks = self.boot_disks()
        for index, (name, drive, bootindex) in enumerate(boot_disks):
            if drive == "virtio-blk":
                device = f"virtio-blk-pci,drive={name},id={name}"
            elif drive == "virtio-scsi":
                device = (
                    "scsi-hd,bus=scsi1.0,channel=0,scsi-id=0,"
                    f"lun={index},drive={name},id={name}"
                )
            elif drive == "nvme":
                # serial 必须确定性生成，source 和 target 才一致。
                serial = f"collei-{self.vm.config.guest_id:04d}-{name}"
                device = f"nvme,drive={name},serial={serial},id={name}"
            else:
                device = f"ide-hd,drive={name},id={name}"
            if bootindex is not None:
                device += f",bootindex={bootindex}"
            self._add_disk(argv, name, device, size=DEFAULT_BOOT_SIZE)
        configured = [name for name, _, _ in boot_disks]
        actual = sorted(path.name for path in self.image_dir.glob("boot[1-9]"))
        expected_after_create = sorted(set(actual) | set(configured))
        if sorted(configured) != expected_after_create:
            raise UnsupportedNativeConfiguration("boot disks and opt/disk do not match")

    def setup_mem_cpu(self, argv: list[str]) -> None:
        if not self.dry_run and self.vm.config.options.enabled("hugetlb"):
            ram = self.vm.config.options.integer("ram", 8)
            target_pages = ram * 512
            hugetlb = Path("/sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages")
            current = int(hugetlb.read_text())
            if current < target_pages:
                CommandRunner().run(
                    ["sudo", "tee", hugetlb], input_text=f"{target_pages}\n"
                )
        if self.vm.config.options.enabled("hack_memory_cpu"):
            self.setup_memory_explicit(argv)
            return
        cpu_num = self.vm.config.options.integer("smp", os.cpu_count() or 1)
        max_cpu_num = max(os.cpu_count() or cpu_num, cpu_num)
        ram = self.vm.config.options.integer("ram", 8)
        # 为了实现热插和 vhost 共享，这是最简单的内存配置。
        hugetlb = "true" if self.vm.config.options.enabled("hugetlb") else "false"
        argv.extend(
            [
                "-smp",
                f"{cpu_num},maxcpus={max_cpu_num}",
                "-m",
                f"{ram}G,slots=8,maxmem=256G",
                "-object",
                f"memory-backend-memfd,id=mem0,size={ram}G,prealloc=off,share=on,hugetlb={hugetlb}",
                "-numa",
                "node,nodeid=0,memdev=mem0",
            ]
        )

    def setup_memory_explicit(self, argv: list[str]) -> None:
        # 这个不仅复杂，而且颠覆对于计算机的理解，具体讨论见 docs/qemu/cpu-topo.md。
        socket_num = 2
        numa_num = self.vm.config.options.integer("numa_num", 2)
        cpu_num = self.vm.config.options.integer("smp", 8)
        max_cpu_num = cpu_num if self.vm.config.options.enabled("numa_num") else 8
        if cpu_num > max_cpu_num or max_cpu_num % socket_num:
            raise UnsupportedNativeConfiguration(
                f"invalid explicit CPU topology: smp={cpu_num}, maxcpus={max_cpu_num}"
            )
        ram = self.vm.config.options.integer("ram", 16)
        if ram % numa_num:
            raise UnsupportedNativeConfiguration(
                f"ram={ram}G cannot be divided into numa_num={numa_num}"
            )
        node_ram = ram // numa_num
        argv.extend(
            [
                "-smp",
                f"{cpu_num},maxcpus={max_cpu_num},sockets={socket_num},dies=1,cores={max_cpu_num // socket_num},threads=1",
                "-m",
                f"{ram}G,slots=7,maxmem=256G",
            ]
        )
        for node in range(numa_num):
            argv.extend(
                [
                    "-object",
                    f"memory-backend-memfd,id=mem{node},size={node_ram}G,prealloc=off,share=on",
                    "-numa",
                    f"node,nodeid={node},memdev=mem{node}",
                ]
            )
        # NUMA 内存分布和 CPU socket 分布不用耦合，但 OS 会观察这种关联。
        for cpu in range(max_cpu_num):
            argv.extend(
                [
                    "-numa",
                    f"cpu,node-id={cpu % numa_num},socket-id={cpu % socket_num},core-id={cpu // socket_num}",
                ]
            )
        if numa_num == 16:
            for source in range(numa_num):
                for target in range(source + 1, numa_num):
                    source_l3, target_l3 = source // 2, target // 2
                    if source_l3 == target_l3:
                        latency = 11
                    elif source // 8 == target // 8:
                        latency = 12
                    else:
                        latency = 32
                    argv.extend(
                        ["-numa", f"dist,src={source},dst={target},val={latency}"]
                    )

    def setup_kernel(self, argv: list[str]) -> None:
        kernel_value = self.vm.config.options.get("kernel")
        if kernel_value is None:
            if self.vm.config.options.get("cmdline"):
                raise UnsupportedNativeConfiguration("opt/cmdline requires opt/kernel")
            return
        kernel_dir = Path(kernel_value)
        if self.profile is not None:
            kernel_args = self.profile.kernel_args()
        else:
            cmdline = self.vm.config.options.get("cmdline") or ""
            kernel_args = (
                "  oops=panic panic=0 nokaslr apparmor=0 selinux=0 preempt=full "
                "systemd.unified_cgroup_hierarchy=1  mitigations=off  "
                "rcutree.sysrq_rcu=1  crashkernel=512M  loglevel=8 "
                f"zswap.enabled=0  {cmdline} "
            )
        argv.extend(["-kernel", str(kernel_image(kernel_dir)), "-append", kernel_args])

    def setup_network(self, argv: list[str]) -> None:
        if self.efi_application:
            # ipxe 会阻塞 EFI application 测试。
            argv.extend(["-net", "none"])
            return
        guest_id = self.vm.config.guest_id
        tap = f"vif_{self.vm.which_qemu}_{guest_id}_0"
        level = self.context.global_config.directory.integer("level", 0)
        mac = f"52:54:00:{level:02x}:{guest_id:02x}:00"
        if self.context.global_config.directory.get("bridge") != "no":
            argv.extend(
                [
                    "-device",
                    f"virtio-net,netdev={tap},mac={mac},iommu_platform=on,disable-legacy=on",
                    "-netdev",
                    f"tap,ifname={tap},id={tap},script=no,downscript=no,vhost=on",
                ]
            )
        # 总是把用户态网络放到最后，这样在虚拟机中一眼就可以看到。
        argv.extend(["-device", "virtio-net,netdev=net1"])
        user_net = (
            "user,id=net1,hostfwd="
            f"tcp:127.0.0.1:{self.vm.tcp_port('ssh')}-:22,"
            f"hostname={self.vm.directory.name}"
        )
        if Path("/usr/sbin/smbd").is_file():
            user_net += f",smb={Path.home()}/"
        argv.extend(["-netdev", user_net])

    def setup_vsock(self, argv: list[str]) -> None:
        if not self.vm.config.options.enabled("vsock"):
            return
        cid = self.vm.vsock_cid
        argv.extend(
            [
                "-device",
                f"vhost-vsock-pci,id=vhost-vsock-pci0,guest-cid={cid}",
            ]
        )

    def setup_hct(self, argv: list[str]) -> None:
        value = self.vm.config.options.get("hct")
        if value is None:
            return
        if not self.dry_run:
            runner = CommandRunner()
            for device_uuid in value.splitlines():
                sysfs = Path("/sys/bus/mdev/devices") / device_uuid
                if not sysfs.is_dir():
                    create = Path(
                        "/sys/devices/virtual/hct/hct/mdev_supported_types/hct-1/create"
                    )
                    if not create.is_file():
                        raise ColleiError(
                            f"HCT mdev create interface is missing: {create}"
                        )
                    runner.run(["sudo", "tee", create], input_text=f"{device_uuid}\n")
                    vendor_use = sysfs / "vendor/use"
                    if vendor_use.is_file():
                        runner.run(["sudo", "tee", vendor_use], input_text="1\n")
                iommu_group = (sysfs / "iommu_group").resolve().name
                vfio_group = Path("/dev/vfio") / iommu_group
                if vfio_group.exists() and vfio_group.stat().st_uid != os.getuid():
                    runner.run(["sudo", "chown", os.environ["USER"], vfio_group])
        for index, device_uuid in enumerate(value.splitlines()):
            argv.extend(
                [
                    "-device",
                    f"hct,id=hct{index},sysfsdev=/sys/bus/mdev/devices/{device_uuid}",
                ]
            )

    def setup_machine(self, argv: list[str]) -> None:
        if self.vm.config.options.get("win") == "11":
            argv.extend(["-machine", "q35,smm=on"])
            return
        iommu = self.vm.config.options.get("iommu")
        machine = (
            "q35,hpet=off,smm=off"
            if iommu in {"intel", "amd"}
            else "pc,hpet=off,smm=off"
        )
        # cpr 需要 aux-ram-share，它和上面的机器类型没有耦合。
        argv.extend(["-machine", machine, "-machine", "aux-ram-share=on"])
        # IOMMU 是模拟设备；Intel host 也可以测试 AMD IOMMU，反之亦然。
        if iommu == "intel":
            argv.extend(
                [
                    "-device",
                    "intel-iommu,device-iotlb=on,intremap=on,caching-mode=on,x-pasid-mode=on,x-scalable-mode=on",
                ]
            )
        elif iommu == "amd":
            argv.extend(["-device", "amd-iommu,intremap=on"])
        elif iommu == "virtio":
            argv.extend(["-device", "virtio-iommu-pci"])
        elif iommu is not None:
            raise UnsupportedNativeConfiguration(f"unsupported iommu={iommu}")
        self.setup_pci_topology(argv, machine)
        if self.profile is not None:
            argv.extend(self.profile.rootfs_arguments())

    def setup_pci_topology(self, argv: list[str], machine: str) -> None:
        mode = self.vm.config.options.get("pci")
        if mode is None:
            return
        if mode == "bridge":
            argv.extend(["-device", "pci-bridge,id=mybridge,chassis_nr=1"])
            for index in range(2):
                # serial 必须确定性生成，source 和 target 才一致。
                serial = f"collei-{self.vm.config.guest_id:04d}-nvme{index}"
                self._add_disk(
                    argv,
                    f"nvme{index}",
                    f"nvme,drive=nvme{index},serial={serial},bus=mybridge,addr=0x1",
                    path=self.image_dir / "nvme1",
                )
            argv.extend(
                [
                    "-device",
                    "pci-bridge,id=bridge0,chassis_nr=1",
                    "-device",
                    "pci-bridge,id=bridge1,chassis_nr=2",
                ]
            )
        elif mode == "root_bus":
            if machine.startswith("pc,"):
                argv.extend(
                    [
                        "-device",
                        "pxb,id=bridge2,bus=pci.0,bus_nr=3",
                        "-device",
                        "pxb,id=bridge3,bus=pci.0,bus_nr=8",
                    ]
                )
            else:
                argv.extend(
                    [
                        "-device",
                        "pxb-pcie,id=pcie.1,bus_nr=2,bus=pcie.0",
                        "-device",
                        "pxb-pcie,id=pcie.2,bus_nr=8,bus=pcie.0",
                    ]
                )
        elif mode == "root_complex":
            bus = "pci.0" if machine.startswith("pc,") else "pcie.0"
            argv.extend(["-device", f"ioh3420,id=root_port1,bus={bus}"])
        elif mode == "switch":
            argv.extend(
                [
                    "-device",
                    "ioh3420,id=root_port1,bus=pci.0",
                    "-device",
                    "x3130-upstream,id=upstream1,bus=root_port1",
                    "-device",
                    "xio3130-downstream,id=downstream1,bus=upstream1,chassis=9",
                    "-device",
                    "virtio-scsi-pci,bus=downstream1",
                    "-device",
                    "xio3130-downstream,id=downstream2,bus=upstream1,chassis=10",
                    "-device",
                    "virtio-scsi-pci,bus=downstream2",
                ]
            )
        else:
            raise UnsupportedNativeConfiguration(f"unsupported pci={mode}")

    def setup_monitor(self, argv: list[str]) -> None:
        # qmp-no-pretty 才可以被 kvm-dmesg 识别，所以单独建立一个。
        # -mon 已废弃，使用 -object monitor-qmp/monitor-hmp。
        for number, name, monitor in (
            (4, "qmp-no-pretty", "monitor-qmp,id=mon4,chardev=mon4"),
            (3, "qmp-shell", "monitor-qmp,id=mon3,chardev=mon3"),
            (2, "hmp", "monitor-hmp,id=mon2,chardev=mon2,readline=on"),
            (1, "qmp", "monitor-qmp,id=mon1,chardev=mon1,pretty=on"),
        ):
            argv.extend(
                [
                    "-chardev",
                    f"socket,id=mon{number},path={self.monitor_dir / name},server=on,wait=off",
                    "-object",
                    monitor,
                ]
            )

    def setup_initrd(self, argv: list[str]) -> None:
        kernel_value = self.vm.config.options.get("kernel")
        if kernel_value is None:
            return
        configured = self.vm.config.options.get("initrd")
        initramfs: Path | None = (
            Path(configured)
            if configured is not None
            else self.profile.initramfs
            if self.profile is not None
            else _initramfs(Path(kernel_value))
        )
        if initramfs is not None:
            argv.extend(["-initrd", str(initramfs)])

    def setup_balloon(self, argv: list[str]) -> None:
        # TODO iommu_platform 到底是什么
        # https://www.reddit.com/r/qemu_kvm/comments/1nby753/how_to_properly_use_iommu_platform_with_virtio/
        # virtio-balloon 不能带 ,iommu_platform=on
        # qemu-system-x86_64: -device virtio-balloon,id=balloon0,deflate-on-oom=true,iommu_platform=on: VIRTIO_F_IOMMU_PLATFORM was supported by neither legacy nor transitional device
        # 目前看，网卡是带的，这个的确是关联到 virtio 的 flags VIRTIO_F_ACCESS_PLATFORM 功能的
        #
        # 参考 hw/virtio/virtio-balloon.c:virtio_balloon_properties 一共定义四个开关和 iothread
        arg = ""
        arg += ",deflate-on-oom=true"
        arg += ",free-page-reporting=true"  # 自动移除内存
        # arg += ",page-poison=false"
        arg += ",free-page-hint=true"  # 优化热迁移，依赖 iothread
        argv.extend(
            [
                "-device",
                f"virtio-balloon,id=balloon0,iothread=io_balloon{arg}",
                "-object",
                "iothread,id=io_balloon",
            ]
        )
        # argv.extend(["-device", "virtio-balloon-pci,id=balloon0"])

    def setup_bios(self, argv: list[str]) -> None:
        bios_root = self.context.repo.parent.parent / "bios"
        mode = self.vm.config.options.get("bios")
        if mode is None:
            mode = (
                "ovmf_binary_secure"
                if self.vm.config.options.get("win") == "11"
                else "ovmf_binary"
                if self.vm.config.options.enabled("win")
                else "seabios"
            )
        if mode == "seabios":
            argv.extend(["-bios", str(bios_root / "seabios/out/bios.bin")])
        elif mode == "ovmf_binary":
            ovmf = bios_root / "ovmf_binary/usr/share/edk2/ovmf"
            # x86 + ovmf_binary 必须用 pflash code/vars 分离的方法。
            argv.extend(
                [
                    "-drive",
                    f"file={ovmf / 'OVMF_CODE.fd'},if=pflash,format=raw,unit=0,readonly=on",
                    "-drive",
                    f"file={self.vm.directory / 'OVMF_VARS.fd'},if=pflash,format=raw,unit=1",
                ]
            )
        elif mode == "ovmf_binary_secure":
            ovmf = bios_root / "ovmf_binary_secure/usr/share/edk2/ovmf"
            argv.extend(
                [
                    "-drive",
                    f"file={ovmf / 'OVMF_CODE.secboot.fd'},if=pflash,format=raw,unit=0,readonly=on",
                    "-drive",
                    f"file={self.vm.directory / 'OVMF_VARS.fd'},if=pflash,format=raw,unit=1",
                ]
            )
        elif mode == "ovmf":
            ovmf = bios_root / "edk2/Build/OvmfX64/DEBUG_GCC/FV"
            argv.extend(
                [
                    "-drive",
                    f"file={ovmf / 'OVMF_CODE.fd'},if=pflash,format=raw,unit=0,readonly=on",
                    "-drive",
                    f"file={self.vm.directory / 'OVMF_VARS.fd'},if=pflash,format=raw,unit=1",
                ]
            )
        else:
            raise UnsupportedNativeConfiguration(f"unsupported bios={mode}")
        # x86 单独使用 debugcon，ARM 复用 ttyAMA0。
        argv.extend(
            [
                "-chardev",
                f"file,path={self.monitor_dir / 'debugcon.log'},id=seabios",
                "-device",
                "isa-debugcon,iobase=0x402,chardev=seabios",
            ]
        )
        if self.efi_application:
            virtual_drive = self.context.repo / "VirtualDrive"
            argv.extend(
                ["-drive", f"file=fat:rw:{virtual_drive},format=raw,media=disk"]
            )

    def setup_vfio(self, argv: list[str]) -> None:
        extra = self.vm.config.options.get("vfio_extra") or ""
        use_iommufd = self.vm.config.options.enabled("iommufd")
        if use_iommufd and "iommufd=" in extra:
            raise UnsupportedNativeConfiguration(
                "iommufd conflicts with vfio_extra iommufd="
            )
        iommufd_counter = 0
        iommufd_by_group: dict[str, str] = {}

        def add_vfio_device(device: str, device_extra: str = "") -> None:
            nonlocal iommufd_counter
            value = f"vfio-pci,host={device}"
            combined_extra = ",".join(item for item in (extra, device_extra) if item)
            if combined_extra:
                value += f",{combined_extra}"
            if use_iommufd:
                group = _host_iommu_group(device) or device
                iommufd = iommufd_by_group.get(group)
                if iommufd is None:
                    iommufd = f"iommufd{iommufd_counter}"
                    iommufd_counter += 1
                    iommufd_by_group[group] = iommufd
                    argv.extend(["-object", f"iommufd,id={iommufd}"])
                value += f",iommufd={iommufd}"
            argv.extend(["-device", value])

        for device in (self.vm.config.options.get("vfio") or "").splitlines():
            add_vfio_device(device)
        if self.vm.config.options.enabled("sriov"):
            if self.sriov_vf is None:
                raise UnsupportedNativeConfiguration(
                    "SR-IOV VF is not prepared; run without --dry-run first"
                )
            add_vfio_device(self.sriov_vf, "rombar=0")

    def setup_fs_share(self, argv: list[str]) -> None:
        if self.profile is not None:
            return
        share_dir = self.vm.config.options.get("share_dir")
        if share_dir is None:
            return
        directory = Path(share_dir)
        if not directory.is_dir():
            raise UnsupportedNativeConfiguration(
                f"share_dir is not a directory: {directory}"
            )
        socket = self.monitor_dir / "vfsd.sock"
        if not self.dry_run:
            virtiofsd = shutil.which("virtiofsd")
            if virtiofsd is None and Path("/usr/libexec/virtiofsd").is_file():
                virtiofsd = "/usr/libexec/virtiofsd"
            if virtiofsd is None:
                raise ColleiError("virtiofsd not found")
            socket.unlink(missing_ok=True)
            add_background_task(
                self.context,
                CommandRunner(),
                [
                    virtiofsd,
                    "--socket-path",
                    socket,
                    "--shared-dir",
                    share_dir,
                    "--allow-direct-io",
                ],
                vm=self.vm,
                group="qemu",
                label="virtiofsd-share",
            )
            for _ in range(50):
                if socket.exists():
                    break
                time.sleep(0.1)
            else:
                raise ColleiError(f"virtiofsd socket was not created: {socket}")
        # 理论上参考 virtio-win 文档即可让 Windows 使用同一共享设备。
        argv.extend(
            [
                "-chardev",
                f"socket,id=char0,path={socket}",
                "-device",
                "vhost-user-fs-pci,queue-size=1024,chardev=char0,tag=myfs",
            ]
        )

    def setup_iso(self, argv: list[str]) -> None:
        value = self.vm.config.options.get("iso")
        if value is None:
            return
        iso_root = Path(self.context.global_config.directory.require("iso"))
        for index, line in enumerate(value.splitlines(), 1):
            fields = line.split()
            if len(fields) not in {1, 2}:
                raise UnsupportedNativeConfiguration(f"invalid opt/iso line: {line}")
            iso = iso_root / fields[0]
            if not iso.is_file():
                raise UnsupportedNativeConfiguration(f"ISO does not exist: {iso}")
            disk_id = f"cd{index}"
            device = (
                f"scsi-cd,bus=scsi1.0,channel=0,scsi-id=20,lun={index},drive={disk_id}"
            )
            if len(fields) == 2:
                device += f",bootindex={fields[1]}"
            self._add_disk(
                argv,
                disk_id,
                device,
                path=iso,
                fmt="raw",
                read_only=True,
                create=False,
            )

    def setup_ipmi(self, argv: list[str]) -> None:
        if not self.vm.config.options.enabled("ipmi"):
            return
        # 配置后 guest 中可以看到 /dev/ipmi0，并使用 ipmitool shell。
        argv.extend(
            [
                "-device",
                "ipmi-bmc-sim,id=virt-bmc",
                "-device",
                "pci-ipmi-kcs,bmc=virt-bmc,id=virt-bmc-pci",
            ]
        )

    def setup_accel(self, argv: list[str]) -> None:
        accel = self.vm.config.options.get("accel") or "kvm"
        if accel not in {"kvm", "tcg"}:
            raise UnsupportedNativeConfiguration(f"unknown accel={accel}")
        argv.extend(["-accel", accel])

    def setup_edu(self, argv: list[str]) -> None:
        # 相关文档：docs/specs/edu.rst；用于测试简单 PCI 设备。
        argv.extend(["-device", "edu,dma_mask=0xffffffff"])

    def setup_pidfile(self, argv: list[str]) -> None:
        argv.extend(["-pidfile", str(self.monitor_dir / "pid")])

    def setup_cpu_model(self, argv: list[str]) -> None:
        if self.vm.config.options.get("accel") == "tcg":
            return
        model = "host"
        if self.vm.config.options.enabled("win"):
            model = (
                "host,hv_spinlocks=0x1fff,hv_vapic,hv_time,hv_reset,"
                "hv_vpindex,hv_runtime,hv_relaxed"
            )
        argv.extend(["-cpu", model])

    def setup_display_and_chardev(self, argv: list[str]) -> None:
        if self.profile is not None:
            manual_console = self.profile.manual_console_arguments()
            if manual_console:
                argv.extend(manual_console)
                return

        display = self.vm.config.options.get("display")
        if self.vm.config.options.enabled("win"):
            argv.extend(["-vga", "std"])
        else:
            argv.extend(
                [
                    "-device",
                    "virtio-gpu-pci" if display == "virtio-gpu" else "cirrus-vga",
                ]
            )
        main_chardev = (
            f"socket,path={self.monitor_dir / 'main.sock'},id=main_char,server=on,wait=off,mux=on"
            if self.vm.config.options.enabled("hide")
            else "stdio,id=main_char,server=on,wait=off,id=main_char,mux=on"
        )
        argv.extend(
            [
                "-vnc",
                f":{self.vm.tcp_port('vnc') - 5900},password=off",
                "-device",
                "virtio-serial",
                # 这个配置必须放到最前面，让这个串口是 ttyS0。
                "-chardev",
                main_chardev,
                "-serial",
                "chardev:main_char",
                "-device",
                "virtconsole,chardev=main_char",
                "-object",
                "monitor-hmp,id=mon_main,chardev=main_char,readline=on",
                # 配合 action 中 connect_to_pty 使用。
                "-chardev",
                "pty,mux=on,id=char_pty",
                "-device",
                "virtconsole,chardev=char_pty",
                "-serial",
                "chardev:char_pty",
                # vmtest 必须有一个 qga，不然 init.sh 中的 qga 报错。
                "-chardev",
                f"socket,path={self.monitor_dir / 'qga.sock'},server=on,wait=off,id=qga0",
                "-device",
                "virtserialport,chardev=qga0,name=org.qemu.guest_agent.0",
                # /dev/vport6p3 不能作为 console，保留为独立 vport。
                "-chardev",
                f"socket,path={self.monitor_dir / 'vport.sock'},server=on,wait=off,id=vport",
                "-device",
                "virtserialport,chardev=vport,name=org.qemu.vport.0",
            ]
        )

    def setup_audio(self, argv: list[str]) -> None:
        if not self.vm.config.options.enabled("audio"):
            return
        # 默认关闭；启用后使用 virtio-sound + host ALSA。
        argv.extend(
            [
                "-device",
                "virtio-sound-pci,audiodev=my_audiodev",
                "-audiodev",
                "alsa,id=my_audiodev",
            ]
        )

    def setup_pcie_port(self, argv: list[str]) -> None:
        # 给热插拔使用。
        argv.extend(["-device", "pcie-root-port,id=root_port_1"])

    def setup_rng(self, argv: list[str]) -> None:
        # Windows 11 一定需要 rng，其他 VM 也保持一致。
        argv.extend(
            [
                "-object",
                "rng-random,id=rng0,filename=/dev/urandom",
                "-device",
                "virtio-rng-pci,rng=rng0",
            ]
        )

    def setup_misc(self, argv: list[str]) -> None:
        # -no-user-config 不加载默认配置文件；-nodefaults 不添加默认设备。
        # debug-threads=on 让 gdb 中显示 QEMU thread 名称。
        argv.extend(
            [
                "-no-user-config",
                "-nodefaults",
                "-name",
                "guest=martins3,debug-threads=on",
            ]
        )
        if self.vm.config.options.enabled("no_reboot"):
            argv.append("-no-reboot")

    def setup_uuid(self, argv: list[str]) -> None:
        argv.extend(["-uuid", self.vm.config.options.require("uuid")])

    def setup_input_and_usb(self, argv: list[str]) -> None:
        if self.profile is not None and self.profile.mode() == "manual":
            return
        # qemu-xhci 下同时保留 USB 键盘和 tablet；virtme manual 不需要图形输入。
        argv.extend(
            [
                "-device",
                "virtio-keyboard",
                "-usb",
                "-device",
                "qemu-xhci,p2=8,p3=8,id=usb",
                "-device",
                "usb-kbd,id=input0,bus=usb.0,port=2",
                "-device",
                "usb-tablet,id=input1,bus=usb.0,port=3",
            ]
        )
        if self.vm.config.options.get("win") == "11":
            # Windows 11 需要 TPM 2.0 和 secure pflash。
            argv.extend(
                [
                    "-chardev",
                    f"socket,id=chrtpm,path={self.monitor_dir / 'swtpm-sock'}",
                    "-tpmdev",
                    "emulator,id=tpm0,chardev=chrtpm",
                    "-device",
                    "tpm-tis,tpmdev=tpm0",
                    "-global",
                    "driver=cfi.pflash01,property=secure,value=on",
                ]
            )

    def setup_trace(self, argv: list[str]) -> None:
        tracepoint = []
        script_dir = Path(__file__).resolve().parent / "trace"
        trace_files = (
            "mig.txt",
            "vfio.txt",
            "misc.txt",
        )
        for name in trace_files:
            trace_file = script_dir / name
            if not trace_file.is_file():
                continue

            for line in trace_file.read_text().splitlines():
                line = line.strip()
                if line and not line.startswith("#"):
                    tracepoint.append(line)
        for tp in tracepoint:
            argv.extend(["--trace", tp])


@dataclass(frozen=True)
class BuildrootQemuBuilder:
    context: ColleiContext
    vm: VmRuntime

    def build(self) -> QemuCommand:
        buildroot_value = self.vm.config.options.get("buildroot")
        if buildroot_value is None:
            raise UnsupportedNativeConfiguration("opt/buildroot is missing")
        buildroot = Path(buildroot_value)
        kernel = buildroot / "output/images/bzImage"
        rootfs = buildroot / "output/images/rootfs.ext2"
        if not kernel.is_file() or not rootfs.is_file():
            raise UnsupportedNativeConfiguration(
                f"buildroot images are incomplete: {buildroot / 'output/images'}"
            )

        qemu = self.context.repo.parent.parent / "qemu/build/qemu-system-x86_64"
        common = ColleiQemuBuilder(self.context, self.vm)
        argv = [str(qemu)]
        common.setup_mem_cpu(argv)
        argv.extend(
            [
                "-drive",
                f"file={rootfs},if=virtio,format=raw",
                "-kernel",
                str(kernel),
                "-append",
                "rootwait root=/dev/vda console=tty1 console=ttyS0",
            ]
        )

        # Buildroot 没有 host bridge 配置，只保留 user network 便于快速启动。
        user_net = (
            "user,id=net1,hostfwd="
            f"tcp:127.0.0.1:{self.vm.tcp_port('ssh')}-:22,"
            f"hostname={self.vm.config.name}"
        )
        if Path("/usr/sbin/smbd").is_file():
            user_net += f",smb={Path.home()}/"
        argv.extend(["-device", "virtio-net-pci,netdev=net1", "-netdev", user_net])
        common.setup_machine(argv)
        common.setup_monitor(argv)
        common.setup_balloon(argv)
        common.setup_bios(argv)
        common.setup_accel(argv)
        common.setup_edu(argv)
        common.setup_pidfile(argv)
        common.setup_cpu_model(argv)
        common.setup_display_and_chardev(argv)
        common.setup_pcie_port(argv)
        common.setup_rng(argv)
        common.setup_misc(argv)
        common.setup_uuid(argv)
        common.setup_input_and_usb(argv)
        common.setup_trace(argv)
        return QemuCommand(tuple(argv))


@dataclass(frozen=True)
class WindowsSetup:
    context: ColleiContext
    vm: VmRuntime

    def prepare(self, runner: CommandRunner) -> None:
        if self.vm.config.options.get("win") != "11":
            return
        bios_root = self.context.repo.parent.parent / "bios"
        ovmf = bios_root / "ovmf_binary_secure/usr/share/edk2/ovmf"
        code = ovmf / "OVMF_CODE.secboot.fd"
        variables = ovmf / "OVMF_VARS.secboot.fd"
        if not code.is_file() or not variables.is_file():
            raise UnsupportedNativeConfiguration(
                f"secure OVMF firmware is incomplete: {ovmf}"
            )
        local_variables = self.vm.directory / "OVMF_VARS.fd"
        if not local_variables.exists():
            shutil.copy2(variables, local_variables)

        # 不知道为什么有时 swtpm 无法随着 QEMU 自动结束；每个 VM 使用独立 socket。
        tpm = self.vm.directory / "tpm"
        tpm.mkdir(exist_ok=True)
        socket = self.vm.directory / self.vm.which_qemu / "swtpm-sock"
        socket.unlink(missing_ok=True)
        add_background_task(
            self.context,
            runner,
            [
                "swtpm",
                "socket",
                "--tpmstate",
                f"dir={tpm}",
                "--ctrl",
                f"type=unixio,path={socket}",
                "--log",
                "level=20",
                "--tpm2",
            ],
            vm=self.vm,
            group="qemu",
            label="swtpm",
        )
        if not runner.dry_run:
            for _ in range(50):
                if socket.exists():
                    return
                time.sleep(0.1)
            raise ColleiError(f"swtpm socket was not created: {socket}")


@dataclass(frozen=True)
class FirecrackerSetup:
    context: ColleiContext
    vm: VmRuntime

    @property
    def api_socket(self) -> Path:
        return self.vm.directory / "firecracker.socket"

    @property
    def config_file(self) -> Path:
        return self.vm.directory / "fire.json"

    @property
    def disk_socket(self) -> Path:
        return self.vm.directory / "disk.socket"

    def kernel_args(self) -> str:
        cmdline = self.vm.config.options.get("cmdline") or ""
        return (
            "oops=panic panic=0 nokaslr apparmor=0 selinux=0 preempt=full "
            "systemd.unified_cgroup_hierarchy=1 mitigations=off "
            "rcutree.sysrq_rcu=1 crashkernel=512M loglevel=8 "
            f"zswap.enabled=0 {cmdline}"
        )

    def build(self, *, write_config: bool = True) -> QemuCommand:
        kernel_value = self.vm.config.options.get("kernel")
        if kernel_value is None:
            raise UnsupportedNativeConfiguration(
                "firecracker can't boot without bzImage"
            )
        kernel_dir = Path(kernel_value)
        # Firecracker x86 kernel loader 需要 ELF vmlinux；QEMU 才使用 bzImage。
        kernel = (
            kernel_dir / "vmlinux"
            if platform.machine() == "x86_64"
            else kernel_image(kernel_dir)
        )
        initramfs = _initramfs(kernel_dir)
        boot = self.vm.directory / "img/boot1"
        if not kernel.is_file() or not initramfs.is_file() or not boot.is_file():
            raise UnsupportedNativeConfiguration(
                "firecracker boot files are incomplete"
            )

        template = self.context.repo / "firecracker/fire.json"
        config = json.loads(template.read_text())
        config["boot-source"]["kernel_image_path"] = str(kernel)
        config["boot-source"]["initrd_path"] = str(initramfs)
        config["boot-source"]["boot_args"] = self.kernel_args()
        config["drives"][0]["path_on_host"] = str(boot)
        config["drives"][1]["socket"] = str(self.disk_socket)

        if self.context.global_config.directory.get("bridge") == "no":
            config["network-interfaces"] = []
        else:
            guest_id = self.vm.config.guest_id
            level = self.context.global_config.directory.integer("level", 0)
            config["network-interfaces"][0]["host_dev_name"] = (
                f"vif_{self.vm.which_qemu}_{guest_id}_0"
            )
            config["network-interfaces"][0]["guest_mac"] = (
                f"52:54:00:{level:02x}:{guest_id:02x}:00"
            )

        config["vsock"]["uds_path"] = str(self.vm.directory / "vsock.socket")
        config["logger"]["log_path"] = str(self.vm.directory / "logs")
        config["metrics"]["metrics_path"] = str(self.vm.directory / "metrics")
        config["machine-config"]["vcpu_count"] = self.vm.config.options.integer(
            "smp", os.cpu_count() or 1
        )
        config["machine-config"]["mem_size_mib"] = (
            self.vm.config.options.integer("ram", 8) * 1024
        )
        if platform.machine() == "aarch64":
            config["machine-config"]["smt"] = False
        if write_config:
            self.config_file.write_text(json.dumps(config, indent=2) + "\n")

        firecracker = (
            self.context.repo.parent.parent
            / "firecracker/build/cargo_target/debug/firecracker"
        )
        return QemuCommand(
            (
                str(firecracker),
                "--api-sock",
                str(self.api_socket),
                "--config-file",
                str(self.config_file),
            )
        )

    def prepare(self, runner: CommandRunner) -> None:
        self.api_socket.unlink(missing_ok=True)
        (self.vm.directory / "vsock.socket").unlink(missing_ok=True)
        self.disk_socket.unlink(missing_ok=True)
        (self.vm.directory / "logs").touch()
        (self.vm.directory / "metrics").touch()

        monitor = self.vm.directory / self.vm.which_qemu
        monitor.mkdir(parents=True, exist_ok=True)
        (monitor / "vif_counter").write_text("0\n")
        if self.context.global_config.directory.get("bridge") != "no":
            prepare_ovs_tap(self.context, self.vm, runner)

        # 测试 Firecracker 的 vhost-user-blk。
        disk = self.vm.directory / "img/firecracker-vhost-user-blk.img"
        if not disk.exists():
            disk.parent.mkdir(exist_ok=True)
            with disk.open("wb") as image:
                image.truncate(10 * 1024**3)
        vhost_user_blk = (
            self.context.repo.parent.parent
            / "qemu/build/contrib/vhost-user-blk/vhost-user-blk"
        )
        add_background_task(
            self.context,
            runner,
            [
                vhost_user_blk,
                f"--socket-path={self.disk_socket}",
                f"--blk-file={disk}",
            ],
            vm=self.vm,
            group="qemu",
            label="vhost-user-blk",
        )
        if not runner.dry_run:
            for _ in range(50):
                if self.disk_socket.exists():
                    return
                time.sleep(0.1)
            raise ColleiError(
                f"vhost-user-blk socket was not created: {self.disk_socket}"
            )


@dataclass(frozen=True)
class VmtestSetup:
    context: ColleiContext
    vm: VmRuntime

    def mode(self) -> str:
        return "vmtest"

    def kernel_args(self) -> str:
        cmdline = self.vm.config.options.get("cmdline") or ""
        return (
            "  oops=panic panic=0 nokaslr apparmor=0 selinux=0 preempt=full "
            "systemd.unified_cgroup_hierarchy=1  mitigations=off  "
            "rcutree.sysrq_rcu=1  crashkernel=512M  loglevel=8 "
            f"zswap.enabled=0 console=ttyS0,115200  {cmdline} "
        )

    @property
    def initramfs(self) -> None:
        # vmtest mode don't need initramfs now。
        return None

    def rootfs_arguments(self) -> tuple[str, ...]:
        return (
            "-virtfs",
            "local,id=root,path=/,mount_tag=/dev/root,security_model=none,multidevs=remap",
            "-no-reboot",
        )

    def manual_console_arguments(self) -> tuple[str, ...]:
        monitor = self.vm.directory / self.vm.which_qemu
        return (
            "-display",
            "none",
            "-device",
            "virtio-serial",
            "-chardev",
            "stdio,id=main_char,signal=off",
            "-serial",
            "chardev:main_char",
            "-chardev",
            "pty,id=char_pty",
            "-device",
            "virtconsole,chardev=char_pty",
            "-chardev",
            f"socket,path={monitor / 'qga.sock'},server=on,wait=off,id=qga0",
            "-device",
            "virtserialport,chardev=qga0,name=org.qemu.guest_agent.0",
            "-chardev",
            f"socket,path={monitor / 'vport.sock'},server=on,wait=off,id=vport",
            "-device",
            "virtserialport,chardev=vport,name=org.qemu.vport.0",
        )

    def prepare_init(self, destination: Path = Path("/tmp/martins3/init.sh")) -> Path:
        source = self.context.scripts / "vmtest-init.sh"
        if not source.is_file():
            raise ColleiError(f"vmtest-init.sh not found: {source}")
        destination.parent.mkdir(parents=True, exist_ok=True)
        content = source.read_text()
        bash = shutil.which("bash")
        if bash is None:
            raise ColleiError("bash not found for vmtest init")
        lines = content.splitlines()
        lines[0] = f"#!{bash}"
        content = "\n".join(lines) + "\n"
        content = content.replace("XXXXX", os.environ["PATH"])
        destination.write_text(content)
        destination.chmod(0o755)
        return destination


@dataclass(frozen=True)
class NixosSetup:
    """提取 nixos-rebuild build-vm 生成脚本中的启动参数。

    run-nixos-vm 是一个 shell 脚本，其中 -kernel / -append 用了 shell 的变量默认值
    和命令替换（如 ${NIXPKGS_QEMU_KERNEL_nixos:-...} 和 $(cat .../kernel-params)）。
    这里用 shlex.split 拆分时不会展开这些表达式，必须手动处理后才能传给 QEMU，
    否则会报找不到 kernel 文件或把 $(cat ...) 原样当作内核参数。
    """

    vm: VmRuntime

    @staticmethod
    def _expand_kernel(value: str) -> str:
        # run-nixos-vm 中 -kernel 的值形如：
        #   ${NIXPKGS_QEMU_KERNEL_nixos:-/nix/store/.../kernel}
        # 这里把默认值提取出来作为真实路径。
        match = re.search(r"\$\{NIXPKGS_QEMU_KERNEL_nixos:-([^}]+)\}", value)
        if match:
            return match.group(1)
        return value

    @staticmethod
    def _expand_append(value: str) -> str:
        # run-nixos-vm 中 -append 的值形如：
        #   $(cat /nix/store/.../kernel-params) init=... console=... $QEMU_KERNEL_PARAMS
        # 需要把 $(cat ...) 替换成文件内容，并去掉/展开 $QEMU_KERNEL_PARAMS。
        def read_file(match: re.Match[str]) -> str:
            path = match.group(1)
            try:
                return Path(path).read_text().rstrip("\n")
            except OSError as error:
                raise UnsupportedNativeConfiguration(
                    f"cannot read kernel params from {path}: {error}"
                ) from error

        value = re.sub(r"\$\(cat\s+([^)]+)\)", read_file, value)
        value = value.replace(
            "$QEMU_KERNEL_PARAMS", os.environ.get("QEMU_KERNEL_PARAMS", "")
        )
        return value.strip()

    def arguments(self) -> tuple[str, ...]:
        launcher = self.vm.directory / "result/bin/run-nixos-vm"
        if not launcher.is_file():
            raise UnsupportedNativeConfiguration(
                f"NixOS launcher is missing: {launcher}"
            )
        tokens: list[str] = []
        for line in launcher.read_text().splitlines():
            try:
                tokens.extend(shlex.split(line.rstrip(" \\")))
            except ValueError:
                continue

        result: list[str] = []
        for option in ("-kernel", "-initrd", "-append"):
            try:
                index = tokens.index(option)
                value = tokens[index + 1]
                if option == "-kernel":
                    value = self._expand_kernel(value)
                elif option == "-append":
                    value = self._expand_append(value)
                result.extend([option, value])
            except (ValueError, IndexError) as error:
                raise UnsupportedNativeConfiguration(
                    f"NixOS launcher does not contain {option}"
                ) from error
        for index, token in enumerate(tokens[:-1]):
            if token == "-virtfs" and "store" in tokens[index + 1]:
                result.extend([token, tokens[index + 1]])
                break
        else:
            raise UnsupportedNativeConfiguration(
                "NixOS launcher does not contain the store 9p share"
            )

        vm_dir = self.vm.directory
        result.extend(
            [
                "-drive",
                f"cache=writeback,file={vm_dir / '2.qcow2'},id=drive1,if=none,werror=report",
                "-device",
                "virtio-blk-pci,drive=drive1,serial=root",
                "-virtfs",
                f"local,path={vm_dir / 'shared'},security_model=none,mount_tag=shared",
                "-virtfs",
                f"local,path={vm_dir / 'xchg'},security_model=none,mount_tag=xchg",
            ]
        )
        return tuple(result)


def _first_sriov_vf(interface: str) -> str:
    device = Path("/sys/class/net") / interface / "device"
    if not device.is_dir():
        raise ColleiError(f"SR-IOV interface does not exist: {interface}")
    functions = sorted(device.glob("virtfn*"))
    if not functions:
        raise ColleiError(f"no VF is available on {interface}")
    return functions[0].resolve().name


def prepare_sriov(interface: str, runner: CommandRunner) -> str:
    device = Path("/sys/class/net") / interface / "device"
    total_file = device / "sriov_totalvfs"
    count_file = device / "sriov_numvfs"
    if not total_file.is_file() or not count_file.is_file():
        raise ColleiError(f"{interface} does not support SR-IOV")
    total = int(total_file.read_text())
    if int(count_file.read_text()) == 0:
        runner.run(["sudo", "tee", count_file], input_text=f"{total}\n")
    vf = _first_sriov_vf(interface)
    pci_bind_to_vfio(vf, runner)
    return vf


def _live_qemu_names(vm: VmRuntime) -> list[str]:
    live: list[str] = []
    for name in ("s", "t"):
        pid_file = vm.directory / name / "pid"
        try:
            pid = int(pid_file.read_text().strip())
        except (FileNotFoundError, ValueError):
            continue
        if Path(f"/proc/{pid}/status").is_file():
            live.append(name)
    # 兼容旧布局：根目录 pid 等价于 source qemu。
    if "s" not in live:
        try:
            pid = int((vm.directory / "pid").read_text().strip())
        except (FileNotFoundError, ValueError):
            pid = 0
        if pid and Path(f"/proc/{pid}/status").is_file():
            live.append("s")
    return live


def select_launch_runtime(vm: VmRuntime, options: LaunchOptions) -> VmRuntime:
    """对应 setup_which_qemu 和 migration target 的 s/t 选择。"""
    live = _live_qemu_names(vm)
    target_modes = {"defer", "cpr-transfer"}
    if options.migration in target_modes:
        if not live:
            raise ColleiError("launch source qemu firstly")
        if len(live) == 2:
            raise ColleiError("two qemu is running")
        source = live[0]
        target = "t" if source == "s" else "s"
        if not options.dry_run:
            (vm.directory / "migrate_source").write_text(f"{source}\n")
            (vm.directory / "migrate_target").write_text(f"{target}\n")
        storage_slot = target if options.nbd_target else vm.storage_slot
        return VmRuntime(vm.config, target, vm.live_pids, storage_slot)
    if options.migration in {"file", "cpr"}:
        if live:
            raise ColleiError("qemu is already running")
        return VmRuntime(vm.config, "s", vm.live_pids, vm.storage_slot)
    if live and not options.dry_run:
        raise ColleiError("qemu already launched")
    return vm


def apply_launch_options(
    command: QemuCommand, vm: VmRuntime, options: LaunchOptions
) -> QemuCommand:
    argv = list(command.argv)
    if options.migration == "defer":
        argv.extend(["-incoming", "defer", "-only-migratable"])
    elif options.migration == "file":
        argv.extend(
            [
                "-global",
                "migration.mapped-ram=on",
                "-incoming",
                f"file:{vm.directory / 'vmstate.img'}",
                "-only-migratable",
            ]
        )
    elif options.migration == "cpr":
        argv.extend(["-incoming", "defer", "-only-migratable"])
    elif options.migration == "cpr-transfer":
        argv.extend(
            [
                "-incoming",
                "tcp:0:44444",
                "-incoming",
                '{"channel-type": "cpr", "addr": { "transport": "socket", '
                '"type": "unix", "path": "/tmp/cpr.sock"}}',
            ]
        )

    is_firecracker = vm.config.options.enabled("fire")
    if options.debug_kernel:
        if is_firecracker:
            raise ColleiError("-s is only supported by QEMU")
        argv.extend(
            [
                "-chardev",
                f"socket,path={vm.directory / 'gdb.socket'},server=on,wait=off,id=gdb",
                "-S",
                "-gdb",
                "chardev:gdb",
            ]
        )
    if options.migration is not None and is_firecracker:
        raise ColleiError("migration options are only supported by QEMU")
    if options.gdb:
        prefix = (
            ["gdb", "-ex", "handle SIG34 nostop noprint", "--args"]
            if is_firecracker
            else [
                "gdb",
                "-ex",
                "handle SIGUSR1 nostop noprint",
                "handle SIGPIPE nostop noprint",
                "--args",
            ]
        )
        argv = prefix + argv
    return QemuCommand(tuple(argv))


def prepare_efi_application(context: ColleiContext, vm: VmRuntime) -> None:
    # 对应 add_efi_application_fashion_way：目录会由 QEMU 转换为 FAT disk。
    virtual_drive = context.repo / "VirtualDrive"
    virtual_drive.mkdir(exist_ok=True)
    applications = (
        Path.home() / "data/vn/code/module/gnuefi/hello.efi",
        Path.home() / "data/edk2/Build/Bootloader/DEBUG_GCC/X64/Bootloader.efi",
    )
    existing = [application for application in applications if application.is_file()]
    if not existing:
        raise ColleiError("no EFI application was built")
    for application in existing:
        shutil.copy2(application, virtual_drive / application.name)

    variables = (
        context.repo.parent.parent / "bios/edk2/Build/OvmfX64/DEBUG_GCC/FV/OVMF_VARS.fd"
    )
    if not variables.is_file():
        raise ColleiError(f"OVMF variables are missing: {variables}")
    local_variables = vm.directory / "OVMF_VARS.fd"
    if not local_variables.exists():
        shutil.copy2(variables, local_variables)


def run_in_background(context: ColleiContext, vm: VmRuntime) -> None:
    """后台启动保留 cmd.sh，便于调试生成的 QEMU 命令。"""
    command_script = vm.directory / "cmd.sh"
    handle = add_background_task(
        context,
        CommandRunner(),
        ["bash", command_script],
        vm=vm,
        label="qemu",
        record=True,
    )
    follow = vm.config.options.get("follow")
    if follow == "0":
        return
    exec_task_follow(handle)


def build_qemu_command(
    context: ColleiContext,
    vm: VmRuntime,
    options: LaunchOptions,
    *,
    prepare_host: bool,
) -> QemuCommand:
    """使用 Python setup_* 构造命令；不再隐式回退到 Shell。"""
    if vm.config.options.enabled("fire"):
        firecracker = FirecrackerSetup(context, vm)
        command = firecracker.build(write_config=not options.dry_run)
        if prepare_host:
            firecracker.prepare(CommandRunner())
        command = apply_launch_options(command, vm, options)
        if not options.dry_run:
            command.write_script(vm.directory / "cmd.sh")
        return command
    if vm.config.options.enabled("buildroot"):
        command = BuildrootQemuBuilder(context, vm).build()
        if prepare_host:
            monitor = vm.directory / vm.which_qemu
            monitor.mkdir(parents=True, exist_ok=True)
            for counter in ("hp_mm_counter", "hp_disk_counter", "vif_counter"):
                (monitor / counter).write_text("0\n")
            if "-vnc" in command.argv:
                prepare_novnc(context, vm, CommandRunner())
        command = apply_launch_options(command, vm, options)
        if not options.dry_run:
            command.write_script(vm.directory / "cmd.sh")
        return command
    if vm.config.options.enabled("virtme"):
        profile: QemuProfile | None = VirtmeSetup(context, vm)
    elif vm.config.options.enabled("vmtest"):
        profile = VmtestSetup(context, vm)
    else:
        profile = None
    sriov = vm.config.options.get("sriov")
    sriov_vf = (
        prepare_sriov(sriov, CommandRunner())
        if sriov is not None and prepare_host
        else _first_sriov_vf(sriov)
        if sriov is not None
        else None
    )
    command = ColleiQemuBuilder(
        context,
        vm,
        profile,
        sriov_vf,
        efi_application=options.efi_application,
        dry_run=options.dry_run,
    ).build()
    if (vm.directory / "result/bin/run-nixos-vm").is_file():
        command = QemuCommand(command.argv + NixosSetup(vm).arguments())
    if prepare_host:
        prepare_native_host(context, vm, CommandRunner())
        if "-vnc" in command.argv:
            prepare_novnc(context, vm, CommandRunner())
        if vm.config.options.enabled("win"):
            WindowsSetup(context, vm).prepare(CommandRunner())
        if options.efi_application:
            prepare_efi_application(context, vm)
        if isinstance(profile, VirtmeSetup):
            profile.generate_initramfs()
            profile.prepare_sudo(CommandRunner())
            profile.prepare_debuginfo()
            profile.prepare_rootfs(CommandRunner())
        elif isinstance(profile, VmtestSetup):
            profile.prepare_init()
    command = apply_launch_options(command, vm, options)
    if not options.dry_run:
        command.write_script(vm.directory / "cmd.sh")
    return command


def main(argv: Sequence[str] | None = None) -> int:
    try:
        options = LaunchOptions.parse(sys.argv[1:] if argv is None else argv)
        context = ColleiContext.load()
        vm = context.vm()
        validate_launch_options(vm, options)
        if not options.dry_run:
            color = 112 if vm.which_qemu == "t" else 212
            print_banner(vm.config.name, color=color)
        vm = select_launch_runtime(vm, options)

        command = build_qemu_command(
            context,
            vm,
            options,
            prepare_host=not options.dry_run,
        )
        if options.dry_run:
            print(command.shell_text())
            return 0

        # vmtest 没有网络，必须前台来交互。
        forced_foreground = (
            options.foreground or options.gdb or vm.config.options.enabled("vmtest")
        )
        foreground = forced_foreground
        bg = vm.config.options.get("bg")
        if bg is not None and not forced_foreground:
            if bg not in {"0", "1"}:
                raise ColleiError("bg")
            foreground = bg == "0"
        if foreground:
            CommandRunner().exec(command.argv)
        else:
            run_in_background(context, vm)
        return 0
    except ColleiHelp:
        print_help()
        return 0
    except (ColleiError, OSError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
