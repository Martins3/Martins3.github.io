# ============================================
# virtme 模式支持
# ============================================
from __future__ import annotations

import base64
import getpass
import gzip
import os
import platform
import shutil
import subprocess
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path

from commands import CommandRunner
from errors import ColleiError
from kernel import kernel_release
from runtime import ColleiContext, VmRuntime
from tasks import add_background_task


@dataclass(frozen=True)
class VirtmeSetup:
    context: ColleiContext
    vm: VmRuntime

    # 检测是否启用 virtme 模式
    @property
    def enabled(self) -> bool:
        return self.vm.config.options.enabled("virtme")

    def mode(self) -> str:
        if not self.enabled:
            raise ColleiError("virtme is not enabled")
        mode = self.vm.config.options.get("virtme_mode")
        if mode is None:
            mode = "exec" if self.vm.config.options.enabled("exec") else "manual"
        if mode not in {"manual", "exec"}:
            raise ColleiError(f"unknown virtme_mode: {mode}")
        return mode

    # virtme 模式的 kernel cmdline 设置
    def kernel_args(self) -> str:
        options = self.vm.config.options

        # 基础参数
        args = ["rootfstype=virtiofs", "root=ROOTFS"]

        # 主机名
        args.append(f"virtme_hostname={self.vm.config.name}")

        # 控制台
        if platform.machine() == "x86_64":
            args.extend(("virtme_console=ttyS0", "console=ttyS0,115200n8"))
        else:
            args.extend(("virtme_console=ttyAMA0", "console=ttyAMA0,115200n8"))

        # 用户设置
        user = options.get("user") or os.environ.get("SUDO_USER") or getpass.getuser()
        args.append(f"virtme_user={user}")

        # root 用户标记 (如果以 root 运行)
        if os.getuid() == 0:
            args.append("virtme_root_user=1")

        # 工作目录 (可选)
        cwd = options.get("cwd")
        if cwd is not None:
            args.append(f"virtme_chdir={cwd}")

        # overlay 可写目录配置
        overlays: list[str] = []
        if options.enabled("virtme_rw"):
            # 默认的可写目录
            overlays.extend(("/etc", "/home", "/var", "/tmp"))

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

        # 网络配置 (如果启用)
        if options.enabled("network"):
            args.extend(("virtme.dhcp", "net.ifnames=0", "biosdevname=0"))

        # 脚本执行 (如果配置了 exec)
        exec_script = options.get("exec")
        if exec_script is not None:
            exec_path = Path(exec_script)
            if exec_path.is_file():
                exec_script = exec_path.read_text()
            encoded = base64.b64encode(exec_script.encode()).decode()
            args.append(f"virtme.exec=`{encoded}`")

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
        return self.vm.directory / self.vm.which_qemu / "sudo.bin"

    # 在 host 侧准备一份可读的 sudo 副本 (需要 sudo 权限安装为 root:root 4755)。
    # 放在 VM 目录下，guest 通过 virtiofs 共享可以读到。
    # 仅当副本不存在或 host 的 sudo 更新时才重新安装，避免每次启动都要输密码。
    def prepare_sudo(self, runner: CommandRunner) -> None:
        source = Path("/usr/bin/sudo")
        if not source.is_file():
            return
        target = self._sudo_copy
        if target.is_file() and target.stat().st_mtime >= source.stat().st_mtime:
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
    def initramfs(self) -> Path:
        return self.vm.directory / self.vm.which_qemu / "virtme-initramfs.cpio.gz"

    # 设置 virtme rootfs 共享 (virtio-fs)
    def rootfs_arguments(self) -> tuple[str, ...]:
        socket = self.vm.directory / self.vm.which_qemu / "virtme.sock"
        return (
            "-chardev",
            f"socket,id=virtme_root,path={socket}",
            "-device",
            "vhost-user-fs-pci,chardev=virtme_root,tag=ROOTFS",
        )

    def manual_console_arguments(self) -> tuple[str, ...]:
        if self.mode() != "manual":
            return ()
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

    def prepare_rootfs(self, runner: CommandRunner) -> None:
        # 1. 确定共享目录 (默认是 /)
        share_root = self.vm.config.options.get("share_root") or "/"

        # 2. 启动 virtiofsd (rootfs)
        socket = self.vm.directory / self.vm.which_qemu / "virtme.sock"
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

    # 生成 virtme initramfs
    def generate_initramfs(self) -> Path:
        kernel_value = self.vm.config.options.get("kernel")
        kernel_dir = Path(kernel_value) if kernel_value is not None else None
        init_script = self.context.repo / "virtme" / "virtme-init.sh"
        if not init_script.is_file():
            raise ColleiError(f"virtme-init.sh not found at {init_script}")

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

            # 2. 查找 busybox (优先静态链接版本)
            busybox = next(
                (
                    Path(binary)
                    for name in ("busybox-static", "busybox.static", "busybox")
                    if (binary := shutil.which(name)) is not None
                ),
                None,
            )
            if busybox is None:
                raise ColleiError("busybox not found, please install busybox-static")
            shutil.copy2(busybox, root / "bin/busybox")

            # 创建常用命令链接
            for command in (
                "sh mount umount switch_root insmod modprobe mkdir mknod sleep "
                "uname cp cat chmod echo ln printf base64 setsid cttyhack"
            ).split():
                (root / f"bin/{command}").symlink_to("busybox")

            # 3. 创建设备节点
            # 如果 devtmpfs 不可用，需要这些基本设备。普通用户无法 mknod 时，
            # initramfs 启动后仍可由 devtmpfs 提供，所以保持原脚本的容错行为。
            for name, mode, major, minor in (
                ("null", 0o666, 1, 3),
                ("zero", 0o666, 1, 5),
                ("random", 0o666, 1, 8),
                ("urandom", 0o666, 1, 9),
                ("console", 0o622, 5, 1),
                ("kmsg", 0o660, 1, 11),
            ):
                try:
                    os.mknod(
                        root / f"dev/{name}", 0o20000 | mode, os.makedev(major, minor)
                    )
                except PermissionError:
                    pass

            # 4. 复制 init 脚本
            shutil.copy2(init_script, root / "init")
            (root / "init").chmod(0o755)

            # 5. 复制必要内核模块 (如果内核目录可用)
            if kernel_dir is not None and kernel_dir.is_dir():
                self._copy_modules(root, kernel_dir)

            # 6. 打包为 cpio.gz
            self.initramfs.parent.mkdir(parents=True, exist_ok=True)
            find = subprocess.Popen(
                ["find", ".", "-print0"], cwd=root, stdout=subprocess.PIPE
            )
            if find.stdout is None:
                raise ColleiError("cannot read find output")
            cpio = subprocess.Popen(
                ["cpio", "--null", "-o", "--format=newc"],
                cwd=root,
                stdin=find.stdout,
                stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL,
            )
            find.stdout.close()
            if cpio.stdout is None:
                raise ColleiError("cannot read cpio output")
            with gzip.open(self.initramfs, "wb") as archive:
                shutil.copyfileobj(cpio.stdout, archive)
            cpio.stdout.close()
            if cpio.wait() or find.wait():
                raise ColleiError("failed to create virtme initramfs")
        return self.initramfs

    def _copy_modules(self, root: Path, kernel_dir: Path) -> None:
        # 注意: virtiofs 模块文件名是 virtiofs.ko，但加载时可能用 virtio_fs。
        # 注意: virtiofs 依赖 fuse，需要先加载 fuse。
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
        release = kernel_release(kernel_dir)
        for output_name, source_name in modules:
            matches = sorted(kernel_dir.rglob(f"{source_name}.ko*"))
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
        completed = subprocess.run(
            ["modinfo", "-F", "vermagic", module],
            text=True,
            capture_output=True,
            check=False,
        )
        if completed.returncode != 0:
            return None
        return completed.stdout.split()[0] if completed.stdout.split() else None

    @classmethod
    def _module_matches_kernel(cls, module: Path, release: str) -> bool:
        if not release:
            return "install-" not in module.parts
        vermagic = cls._module_vermagic(module)
        return vermagic is not None and vermagic.startswith(release)
