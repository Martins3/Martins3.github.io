# ============================================
# virtme 模式支持
# ============================================
from __future__ import annotations

import getpass
import os
import shutil
import stat
import subprocess
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path

from commands import CommandRunner
from errors import ColleiError
from initramfs import create_device_nodes, is_statically_linked, pack_cpio_zst
from kernel import kernel_release
from runtime import ColleiContext, VmRuntime
from serial import build_serial_layout_for_options
from tasks import add_background_task


@dataclass(frozen=True)
class VirtmeSetup:
    context: ColleiContext
    vm: VmRuntime

    # 检测是否启用 virtme 模式
    @property
    def enabled(self) -> bool:
        return self.vm.config.options.enabled("virtme")

    def __post_init__(self) -> None:
        if not self.enabled:
            raise ColleiError("virtme is not enabled")

    def network_enabled(self) -> bool:
        value = self.vm.config.options.get("network")
        if value is None or value.strip().lower() in {"1", "auto"}:
            return True
        if value.strip().lower() in {"0", "off"}:
            return False
        raise ColleiError("virtme network must be auto, off, 1, or 0")

    # virtme 模式的 kernel cmdline 设置
    def kernel_args(self) -> str:
        options = self.vm.config.options

        # 基础参数
        args = ["rootfstype=virtiofs", "root=ROOTFS", "console=tty0"]

        # 主机名
        args.append(f"virtme_hostname={self.vm.config.name}")

        # 控制台
        layout = build_serial_layout_for_options(self.vm.qemu_directory, options)
        # virtme 的交互 shell 必须接到实际可交互的 stdio 通道。配置同时
        # 声明 serial stdio 和 hvc socket 时，优先使用 serial；否则 shell
        # 会被放到 hvc0.socket，启动终端只能看到内核日志。
        stdio_console = next(
            (
                channel.console_name
                for channel in layout.channels
                if channel.interactive and channel.backend == "stdio"
            ),
            None,
        )
        args.append(f"virtme_console={stdio_console or layout.primary_console}")
        if layout.first("virtserialport") is not None:
            args.append("virtme_qga=1")
            qemu_ga = shutil.which("qemu-ga")
            if qemu_ga is None:
                raise ColleiError("qemu-ga is required for virtme QGA")
            args.append(f"virtme_qga_bin={qemu_ga}")

        # 用户设置
        user = options.get("user") or os.environ.get("SUDO_USER") or getpass.getuser()
        args.append(f"virtme_user={user}")
        args.append("virtme_empty_passwords=1")

        # root 用户标记 (如果以 root 运行)
        if os.getuid() == 0:
            args.append("virtme_root_user=1")

        # 默认继承启动 collei 时的 host 当前目录；config.ini cwd 可显式覆盖。
        cwd = options.get("cwd") or os.getcwd()
        args.append(f"virtme_chdir={cwd}")

        # overlay 可写目录配置
        overlays: list[str] = []
        if options.enabled("virtme_rw"):
            # 默认的可写目录
            overlays.extend(("/etc", "/home", "/var"))

        # 额外的可写目录 (逗号分隔)
        extra = options.get("virtme_rw_overlay")
        if extra is not None:
            overlays.extend(directory.strip() for directory in extra.split(","))
        for index, directory in enumerate(item for item in overlays if item):
            args.append(f"virtme_rw_overlay{index}={directory}")

        # 模块链接 (如果 kernel_dir 设置)。build/build.sh 已经用
        # make modules_install INSTALL_MOD_PATH=<kernel>.mod 装好了全部模块，
        # 直接把 guest 的 /lib/modules/<kver> 指过去即可。
        kernel_value = options.get("kernel")
        if kernel_value is not None:
            kernel_dir = Path(kernel_value)
            if kernel_dir.is_dir():
                modules = (
                    kernel_dir.parent
                    / f"{kernel_dir.name}.mod"
                    / "lib"
                    / "modules"
                    / kernel_release(kernel_dir)
                )
                if modules.is_dir():
                    args.append(f"virtme_link_mods={modules}")

        # 其他常用参数
        args.extend(("nokaslr", "mitigations=off", "loglevel=8"))

        # 按 MAC 区分两张默认网卡，不依赖 ens4/ens5 等随 PCI 拓扑变化的名称。
        # 用户态后端负责 DHCP、默认路由和 DNS；tap/vhost 只配置 /16 connected route。
        if self.network_enabled():
            # setup_network() 中用户态后端是第一个未显式指定 MAC 的 NIC，因此使用
            # QEMU qemu_macaddr_default_if_unset() 分配的首地址。
            args.append("virtme_net_dhcp=52:54:00:12:34:56")
            if self.context.global_config.options.get("bridge") != "no":
                level = self.context.global_config.options.integer("level", 0)
                guest = self.vm.config.guest_id
                vhost_mac = f"52:54:00:{level:02x}:{guest:02x}:00"
                args.append(f"virtme_net_static={vhost_mac},10.0.{guest}.{level}/16")

        # 用户自定义参数
        cmdline = options.get("cmdline")
        if cmdline is not None:
            args.append(cmdline)

        # Fedora 的 sudo 是 ---s--x--x (4111)，virtiofsd 读不到导致 guest 里
        # 无法 exec。prepare_sudo() 会在 host 侧准备 4755 的副本，guest init
        # 负责 bind-mount 覆盖 /usr/bin/sudo。
        args.append(f"virtme_sudo_bin={self._sudo_copy}")

        # VSOCK SSH 支持
        if options.enabled("vsock"):
            args.append(f"virtme.vsock_cid={self.vm.vsock_cid}")

        return " " + " ".join(args) + " "

    @property
    def _sudo_copy(self) -> Path:
        return self.vm.qemu_directory / "sudo.bin"

    # 在 host 侧准备一份可读的 sudo 副本 (需要 sudo 权限安装为 root:root 4755)。
    # 放在 VM 目录下，guest 通过 virtiofs 共享可以读到。
    # 克隆 VM 会丢失 root 所有权和 setuid 位，不能只按修改时间复用副本。
    def prepare_sudo(self, runner: CommandRunner) -> None:
        source = Path("/usr/bin/sudo")
        if not source.is_file():
            return
        target = self._sudo_copy
        if target.is_file():
            target_stat = target.stat()
            if (
                target_stat.st_mtime >= source.stat().st_mtime
                and target_stat.st_uid == 0
                and target_stat.st_gid == 0
                and stat.S_IMODE(target_stat.st_mode) == 0o4755
            ):
                return
        self._sudo_copy.parent.mkdir(parents=True, exist_ok=True)
        runner.run(
            [
                "sudo",
                "install",
                "-m",
                "4755",
                "-o",
                "root",
                "-g",
                "root",
                str(source),
                str(self._sudo_copy),
            ]
        )

    # make modules_install 默认会创建 build -> 构建树 的符号链接，
    # build/build.sh 出于打包考虑把它删了 (rm -rf .../build)。恢复它，
    # guest 里的 drgn 就能通过标准搜索路径
    # /lib/modules/<kver>/build/vmlinux 找到带 DWARF 的 vmlinux。
    def prepare_debuginfo(self) -> None:
        kernel_value = self.vm.config.options.get("kernel")
        if kernel_value is None:
            return
        kernel_dir = Path(kernel_value)
        if not kernel_dir.is_dir() or not (kernel_dir / "vmlinux").is_file():
            return
        modules = (
            kernel_dir.parent
            / f"{kernel_dir.name}.mod"
            / "lib"
            / "modules"
            / kernel_release(kernel_dir)
        )
        build = modules / "build"
        if modules.is_dir() and not build.exists():
            build.symlink_to(kernel_dir)

    @property
    def _initramfs_path(self) -> Path:
        return self.vm.qemu_directory / "virtme-initramfs.cpio.zst"

    def initramfs(self, kernel_dir: Path, configured: Path | None) -> Path:
        del kernel_dir
        return configured if configured is not None else self._initramfs_path

    # 设置 virtme rootfs 共享 (virtio-fs)
    def rootfs_arguments(self) -> tuple[str, ...]:
        socket = self.vm.qemu_directory / "virtme.sock"
        return (
            "-chardev",
            f"socket,id=virtme_root,path={socket}",
            "-device",
            "vhost-user-fs-pci,chardev=virtme_root,tag=ROOTFS",
        )

    def prepare_rootfs(self, runner: CommandRunner) -> None:
        # 1. 确定共享目录 (默认是 /)
        share_root = self.vm.config.options.get("share_root") or "/"

        # 2. 启动 virtiofsd (rootfs)
        socket = self.vm.qemu_directory / "virtme.sock"
        virtiofsd = shutil.which("virtiofsd")
        if virtiofsd is None and Path("/usr/libexec/virtiofsd").is_file():
            virtiofsd = "/usr/libexec/virtiofsd"
        if virtiofsd is None:
            raise ColleiError("virtiofsd not found, please install virtiofsd")
        add_background_task(
            self.context,
            runner,
            [
                virtiofsd,
                "--socket-path",
                socket,
                "--shared-dir",
                share_root,
                "--sandbox",
                "none",
                "--cache",
                "always",
                "--no-announce-submounts",
            ],
            vm=self.vm,
            label="virtiofsd-rootfs",
        )
        if not runner.dry_run:
            for _ in range(50):
                if socket.exists():
                    break
                time.sleep(0.1)
            else:
                raise ColleiError(f"virtiofsd socket was not created: {socket}")

        # 3. QEMU 参数由 rootfs_arguments() 设置。

    def prepare(self, runner: CommandRunner) -> None:
        self.generate_initramfs()
        self.prepare_sudo(runner)
        self.prepare_debuginfo()
        self.prepare_rootfs(runner)

    def fallback_boot_disks(self) -> list[tuple[str, str, str | None]] | None:
        return None

    def share_directory(self) -> str | None:
        return None

    @property
    def force_foreground(self) -> bool:
        return False

    # 生成 virtme initramfs
    def generate_initramfs(self) -> Path:
        kernel_value = self.vm.config.options.get("kernel")
        kernel_dir = Path(kernel_value) if kernel_value is not None else None
        init_script = self.context.repo / "virtme" / "virtme-init-loader.sh"
        if not init_script.is_file():
            raise ColleiError(f"virtme init not found at {init_script}")
        rust_init = self._build_rust_init()

        # 查找 busybox (优先静态链接版本)。提前到缓存检查之前，
        # 因为 busybox 路径也是缓存输入之一。
        busybox = next(
            (
                Path(binary)
                for name in ("busybox-static", "busybox.static", "busybox")
                if (binary := shutil.which(name)) is not None
            ),
            None,
        )
        if busybox is None:
            raise ColleiError(
                "busybox not found; please install a static BusyBox package"
            )
        if not is_statically_linked(busybox):
            raise ColleiError(
                f"busybox at {busybox} is not statically linked; "
                "please install a static BusyBox"
            )

        # 模块源文件路径 (单次目录遍历，见 _find_modules)
        modules = (
            self._find_modules(kernel_dir)
            if kernel_dir is not None and kernel_dir.is_dir()
            else {}
        )

        # 缓存: 输入 (init 脚本、busybox、模块、机器类型/vsock 选项) 没变化时
        # 直接复用，避免每次启动都重新 cpio+zstd。
        stamp = self._initramfs_stamp(init_script, rust_init, busybox, modules)
        stamp_path = self._initramfs_path.parent / f"{self._initramfs_path.name}.stamp"
        if (
            self._initramfs_path.is_file()
            and stamp_path.is_file()
            and stamp_path.read_text() == stamp
        ):
            return self._initramfs_path

        # 1. 创建目录结构
        with tempfile.TemporaryDirectory(prefix="collei-virtme-") as temporary:
            root = Path(temporary)
            for directory in (
                "bin",
                "dev",
                "proc",
                "sys",
                "newroot",
                "run",
                "lib/modules",
                "tmp",
            ):
                (root / directory).mkdir(parents=True, exist_ok=True)

            # 2. 复制 busybox
            shutil.copy2(busybox, root / "bin/busybox")

            # 创建第一阶段 loader 所需的 BusyBox applet 链接。
            commands = (
                "sh mount umount switch_root insmod modprobe mkdir mknod sleep "
                "uname cp cat chmod echo ln printf"
            ).split()
            for command in commands:
                (root / f"bin/{command}").symlink_to("busybox")

            # 3. 创建设备节点
            create_device_nodes(root)

            # 4. 复制 init 脚本
            shutil.copy2(init_script, root / "init")
            (root / "init").chmod(0o755)

            # 第一阶段 loader 在 ROOTFS 的私有 /tmp 中安装 Rust init，
            # 并通过 switch_root 将其作为 PID 1 启动。
            shutil.copy2(rust_init, root / "bin/virtme-ng-init.out")
            (root / "bin/virtme-ng-init.out").chmod(0o755)

            # 5. 复制必要内核模块 (如果内核目录可用)
            self._copy_modules(root, modules)

            # 6. 打包为 cpio.zst
            pack_cpio_zst(root, self._initramfs_path)
        stamp_path.write_text(stamp)
        return self._initramfs_path

    def _build_rust_init(self) -> Path:
        crate = self.context.repo / "virtme" / "virtme-ng-init"
        manifest = crate / "Cargo.toml"
        lockfile = crate / "Cargo.lock"
        sources = [
            manifest,
            lockfile,
            crate / "virtme-udhcpc-script",
            *sorted((crate / "src").glob("*")),
        ]
        missing = [path for path in sources if not path.is_file()]
        if missing:
            raise ColleiError(f"missing Rust init source: {missing[0]}")

        cargo_binary = crate / "target/release/virtme-ng-init"
        binary = crate / "target/release/virtme-ng-init.out"
        newest_source = max(path.stat().st_mtime_ns for path in sources)
        if binary.is_file() and binary.stat().st_mtime_ns >= newest_source:
            return binary

        cargo = shutil.which("cargo")
        if cargo is None:
            raise ColleiError("cargo not found, cannot build the Rust virtme init")
        completed = subprocess.run(
            [
                cargo,
                "build",
                "--release",
                "--locked",
                "--manifest-path",
                str(manifest),
            ],
            check=False,
        )
        if completed.returncode != 0 or not cargo_binary.is_file():
            raise ColleiError("failed to build the Rust virtme init")

        shutil.copy2(cargo_binary, binary)
        strip = shutil.which("strip")
        if strip is not None:
            subprocess.run([strip, str(binary)], check=True)
        return binary

    # 需要的 initramfs 模块列表: (输出名, 源模块名)。
    # 注意: virtiofs 模块文件名是 virtiofs.ko，但加载时可能用 virtio_fs。
    # 注意: virtiofs 依赖 fuse，需要先加载 fuse。
    def _module_list(self) -> list[tuple[str, str]]:
        machine = self.vm.config.options.get("machine") or "pc"
        modules = [("fuse", "fuse"), ("virtio_fs", "virtiofs"), ("overlay", "overlay")]
        if machine != "microvm":
            modules[0:0] = [
                ("virtio_pci_modern_dev", "virtio_pci_modern_dev"),
                ("virtio_pci_legacy_dev", "virtio_pci_legacy_dev"),
                ("virtio_pci", "virtio_pci"),
            ]
        if self.vm.config.options.enabled("vsock"):
            modules.extend(
                (name, name)
                for name in (
                    "vsock",
                    "vmw_vsock_virtio_transport_common",
                    "vmw_vsock_virtio_transport",
                )
            )
        return modules

    def _initramfs_stamp(
        self,
        init_script: Path,
        rust_init: Path,
        busybox: Path,
        modules: dict[str, Path],
    ) -> str:
        parts = [
            f"{init_script}:{init_script.stat().st_mtime_ns}",
            f"{busybox}:{busybox.stat().st_mtime_ns}",
            f"machine={self.vm.config.options.get('machine') or 'pc'}",
            f"vsock={self.vm.config.options.enabled('vsock')}",
            f"{rust_init}:{rust_init.stat().st_mtime_ns}",
        ]
        for name in sorted(modules):
            path = modules[name]
            parts.append(f"{name}:{path}:{path.stat().st_mtime_ns}")
        return "\n".join(parts)

    # 在内核构建树中定位需要的模块。逐模块 rglob 会对整棵树做 N 次全量
    # 扫描 (~1.2s)，这里只做一次遍历，按文件名建立索引。
    def _find_modules(self, kernel_dir: Path) -> dict[str, Path]:
        wanted = {source for _, source in self._module_list()}
        found: dict[str, list[Path]] = {}
        for path in kernel_dir.rglob("*.ko*"):
            base = path.name.split(".ko")[0]
            if base in wanted:
                found.setdefault(base, []).append(path)
        release = kernel_release(kernel_dir)
        result: dict[str, Path] = {}
        for output_name, source_name in self._module_list():
            matches = sorted(found.get(source_name, []))
            if not matches:
                continue
            source = next(
                (
                    candidate
                    for candidate in matches
                    if self._module_matches_kernel(candidate, release)
                ),
                None,
            )
            if source is None:
                available = sorted(
                    {
                        vermagic
                        for candidate in matches
                        if (vermagic := self._module_vermagic(candidate)) is not None
                    }
                )
                raise ColleiError(
                    f"no {source_name}.ko matching kernel {release} "
                    f"(available: {', '.join(available) or 'unknown'})"
                )
            result[output_name] = source
        return result

    def _copy_modules(self, root: Path, modules: dict[str, Path]) -> None:
        for output_name, source in modules.items():
            target = root / f"lib/modules/{output_name}.ko"
            if source.suffix == ".zst":
                with target.open("wb") as output:
                    completed = subprocess.run(
                        ["zstd", "-d", "-c", source], stdout=output, check=False
                    )
                if completed.returncode:
                    target.unlink(missing_ok=True)
            else:
                shutil.copy2(source, target)
            # 去掉 .BTF 段: 增量构建的内核树里模块 BTF 可能相对 vmlinux 过期，
            # insmod 会报 "failed to validate module BTF: -22"。没有 .BTF 段
            # 内核会跳过校验，initramfs 里的驱动本来也不需要 BTF。
            objcopy = shutil.which("objcopy")
            if objcopy is not None and target.is_file():
                subprocess.run(
                    [objcopy, "--remove-section", ".BTF", str(target)],
                    check=False,
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL,
                )

    @staticmethod
    def _module_vermagic(module: Path) -> str | None:
        try:
            completed = subprocess.run(
                ["modinfo", "-F", "vermagic", module],
                text=True,
                capture_output=True,
                check=False,
            )
        except OSError:
            pass
        else:
            fields = completed.stdout.split()
            if completed.returncode == 0 and fields:
                return fields[0]
        # openEuler kmod 29 may return success with an empty value for valid modules.
        return VirtmeSetup._elf_modinfo_value(module, "vermagic")

    @staticmethod
    def _elf_modinfo_value(module: Path, field: str) -> str | None:
        print("fallback to objcopy to get vermagic")
        try:
            completed = subprocess.run(
                ["objcopy", "--dump-section", ".modinfo=/dev/stdout", module],
                capture_output=True,
                check=False,
            )
        except OSError:
            return None
        if completed.returncode != 0:
            return None
        prefix = field.encode() + b"="
        for entry in completed.stdout.split(b"\0"):
            if entry.startswith(prefix):
                values = entry[len(prefix) :].split(maxsplit=1)
                if values:
                    return values[0].decode("ascii", errors="replace")
                return None
        return None

    @classmethod
    def _module_matches_kernel(cls, module: Path, release: str) -> bool:
        if not release:
            return "install-" not in module.parts
        vermagic = cls._module_vermagic(module)
        return vermagic is not None and vermagic.startswith(release)
