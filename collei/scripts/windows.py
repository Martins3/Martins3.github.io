from __future__ import annotations

import shutil
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Protocol

from commands import CommandRunner
from errors import ColleiError, UnsupportedNativeConfiguration
from qemu import QemuCommand
from runtime import ColleiContext, VmRuntime
from tasks import add_background_task


class WindowsQemuBuilder(Protocol):
    @property
    def efi_application(self) -> bool: ...

    @property
    def monitor_dir(self) -> Path: ...

    def validate(self) -> None: ...

    def setup_storage(self, argv: list[str]) -> None: ...

    def setup_mem_cpu(self, argv: list[str]) -> None: ...

    def setup_basic_storage(self, argv: list[str]) -> None: ...

    def setup_kernel(self, argv: list[str]) -> None: ...

    def setup_vsock(self, argv: list[str]) -> None: ...

    def setup_hct(self, argv: list[str]) -> None: ...

    def setup_machine(self, argv: list[str]) -> None: ...

    def setup_monitor(self, argv: list[str]) -> None: ...

    def setup_initrd(self, argv: list[str]) -> None: ...

    def setup_balloon(self, argv: list[str]) -> None: ...

    def setup_vfio(self, argv: list[str]) -> None: ...

    def setup_fs_share(self, argv: list[str]) -> None: ...

    def setup_iso(self, argv: list[str]) -> None: ...

    def setup_ipmi(self, argv: list[str]) -> None: ...

    def setup_accel(self, argv: list[str]) -> None: ...

    def setup_edu(self, argv: list[str]) -> None: ...

    def setup_pidfile(self, argv: list[str]) -> None: ...

    def setup_audio(self, argv: list[str]) -> None: ...

    def setup_pcie_port(self, argv: list[str]) -> None: ...

    def setup_rng(self, argv: list[str]) -> None: ...

    def setup_misc(self, argv: list[str]) -> None: ...

    def setup_pstore(self, argv: list[str]) -> None: ...

    def setup_uuid(self, argv: list[str]) -> None: ...

    def setup_trace(self, argv: list[str]) -> None: ...


@dataclass(frozen=True)
class WindowsProfile:
    """Windows-specific QEMU hardware and host preparation."""

    context: ColleiContext
    vm: VmRuntime
    common: WindowsQemuBuilder

    def build(self) -> QemuCommand:
        self.common.validate()
        qemu = self.context.repo.parent.parent / "qemu/build/qemu-system-x86_64"
        argv = [str(qemu)]
        self.common.setup_storage(argv)
        self.common.setup_mem_cpu(argv)
        self.common.setup_basic_storage(argv)
        self.common.setup_kernel(argv)
        self._setup_network(argv)
        self.common.setup_vsock(argv)
        self.common.setup_hct(argv)
        self._setup_machine(argv)
        self._setup_rtc(argv)
        self.common.setup_monitor(argv)
        self.common.setup_initrd(argv)
        self.common.setup_balloon(argv)
        self._setup_bios(argv)
        self.common.setup_vfio(argv)
        self.common.setup_fs_share(argv)
        self.common.setup_iso(argv)
        self.common.setup_ipmi(argv)
        self.common.setup_accel(argv)
        self.common.setup_edu(argv)
        self.common.setup_pidfile(argv)
        self._setup_cpu_model(argv)
        self._setup_display_and_chardev(argv)
        self.common.setup_audio(argv)
        self.common.setup_pcie_port(argv)
        self.common.setup_rng(argv)
        self.common.setup_misc(argv)
        self.common.setup_pstore(argv)
        self.common.setup_uuid(argv)
        self._setup_input_and_usb(argv)
        self.common.setup_trace(argv)
        return QemuCommand(tuple(argv))

    # FIXME 明显这个可以简化一下，其实这里只是用了添加几个新的 hostfwd 吧
    def _setup_network(self, argv: list[str]) -> None:
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
        argv.extend(["-device", "virtio-net,netdev=net1"])
        ssh_port = self.vm.tcp_port("ssh")
        rdp_port = self.vm.tcp_port("rdp")
        user_net = (
            "user,id=net1,"
            f"hostfwd=tcp:127.0.0.1:{ssh_port}-:22,"
            f"hostfwd=tcp:127.0.0.1:{rdp_port}-:3389,"
            f"hostfwd=udp:127.0.0.1:{rdp_port}-:3389,"
            f"hostname={self.vm.config.name}"
        )
        argv.extend(["-netdev", user_net])

    def _setup_machine(self, argv: list[str]) -> None:
        if self.vm.config.options.get("win") == "11":
            argv.extend(["-machine", "q35,smm=on"])
            return
        self.common.setup_machine(argv)

    # 不去配置这个，会到直接 windows 开机看到的时区是错误的
    # 根因是 QEMU 默认 RTC 使用 UTC，而 Windows 默认按本地时间读取 RTC
    # -rtc base=localtime
    def _setup_rtc(self, argv: list[str]) -> None:
        argv.extend(["-rtc", "base=localtime"])

    def _bios_mode(self) -> str:
        configured = self.vm.config.options.get("bios")
        if configured is not None:
            return configured
        if self.vm.config.options.get("win") == "11":
            return "ovmf_binary_secure"
        return "ovmf_binary"

    def _setup_bios(self, argv: list[str]) -> None:
        bios_root = self.context.repo.parent.parent / "bios"
        mode = self._bios_mode()
        if mode == "seabios":
            argv.extend(["-bios", str(bios_root / "seabios/out/bios.bin")])
        elif mode == "ovmf_binary":
            ovmf = bios_root / "ovmf_binary/usr/share/edk2/ovmf"
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
        argv.extend(
            [
                "-chardev",
                f"file,path={self.common.monitor_dir / 'debugcon.log'},id=seabios",
                "-device",
                "isa-debugcon,iobase=0x402,chardev=seabios",
            ]
        )
        if self.common.efi_application:
            virtual_drive = self.context.repo / "VirtualDrive"
            argv.extend(
                ["-drive", f"file=fat:rw:{virtual_drive},format=raw,media=disk"]
            )

    def _setup_cpu_model(self, argv: list[str]) -> None:
        if self.vm.config.options.get("accel") == "tcg":
            return
        argv.extend(
            [
                "-cpu",
                "host,hv_spinlocks=0x1fff,hv_vapic,hv_time,hv_reset,"
                "hv_vpindex,hv_runtime,hv_relaxed",
            ]
        )

    def _setup_display_and_chardev(self, argv: list[str]) -> None:
        argv.extend(["-vga", "std"])
        main_chardev = (
            f"socket,path={self.common.monitor_dir / 'main.sock'},id=main_char,server=on,wait=off,mux=on"
            if self.vm.config.options.enabled("hide")
            else "stdio,id=main_char,server=on,wait=off,id=main_char,mux=on"
        )
        argv.extend(
            [
                "-vnc",
                f"127.0.0.1:{self.vm.tcp_port('vnc') - 5900},password=off",
                "-device",
                "virtio-serial",
                "-chardev",
                main_chardev,
                "-serial",
                "chardev:main_char",
                "-device",
                "virtconsole,chardev=main_char",
                "-object",
                "monitor-hmp,id=mon_main,chardev=main_char,readline=on",
                "-chardev",
                "pty,mux=on,id=char_pty",
                "-device",
                "virtconsole,chardev=char_pty",
                "-serial",
                "chardev:char_pty",
                "-chardev",
                f"socket,path={self.common.monitor_dir / 'qga.sock'},server=on,wait=off,id=qga0",
                "-device",
                "virtserialport,chardev=qga0,name=org.qemu.guest_agent.0",
                "-chardev",
                f"socket,path={self.common.monitor_dir / 'vport.sock'},server=on,wait=off,id=vport",
                "-device",
                "virtserialport,chardev=vport,name=org.qemu.vport.0",
            ]
        )

    def _setup_input_and_usb(self, argv: list[str]) -> None:
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
        if self.vm.config.options.get("win") != "11":
            return
        argv.extend(
            [
                "-chardev",
                f"socket,id=chrtpm,path={self.common.monitor_dir / 'swtpm-sock'}",
                "-tpmdev",
                "emulator,id=tpm0,chardev=chrtpm",
                "-device",
                "tpm-tis,tpmdev=tpm0",
                "-global",
                "driver=cfi.pflash01,property=secure,value=on",
            ]
        )

    def prepare(self, runner: CommandRunner) -> None:
        if self.vm.config.options.get("win") != "11":
            return
        ovmf = (
            self.context.repo.parent.parent
            / "bios/ovmf_binary_secure/usr/share/edk2/ovmf"
        )
        code = ovmf / "OVMF_CODE.secboot.fd"
        variables = ovmf / "OVMF_VARS.secboot.fd"
        if not code.is_file() or not variables.is_file():
            raise UnsupportedNativeConfiguration(
                f"secure OVMF firmware is incomplete: {ovmf}"
            )
        local_variables = self.vm.directory / "OVMF_VARS.fd"
        if not local_variables.exists():
            shutil.copy2(variables, local_variables)

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
        if runner.dry_run:
            return
        for _ in range(50):
            if socket.exists():
                return
            time.sleep(0.1)
        raise ColleiError(f"swtpm socket was not created: {socket}")
