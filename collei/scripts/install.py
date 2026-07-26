from __future__ import annotations

import re
import shutil
import tempfile
import uuid
from dataclasses import dataclass
from pathlib import Path
from string import Template
from typing import Generic, Protocol, TypeVar

from commands import CommandRunner
from disks import create_missing_disk, create_standard_boot_disks
from errors import ColleiError
from runtime import ColleiContext, VmRuntime
from ui import choose, confirm

FEDORA_KS_LABEL = "KICKSTART"
INSTALLER_PXEBOOT_FILES = ("images/pxeboot/vmlinuz", "images/pxeboot/initrd.img")
FEDORA_KICKSTART_TEMPLATE = """#version=F44
# Fedora Server 自动安装 kickstart 模板
# 由 collei-install.py -f 渲染

text
lang en_US.UTF-8
keyboard us
timezone ${timezone} --utc

rootpw --plaintext ${root_password}
user --name=${user_name} --groups=wheel --plaintext --password=${user_password}

network --bootproto=dhcp --device=link --hostname=${hostname}

ignoredisk --only-use=vda
clearpart --all --initlabel
part /boot/efi --fstype=efi --size=600 --ondisk=vda --fsoptions="umask=0077,shortname=winnt"
part /boot --fstype=xfs --size=1024 --ondisk=vda
part pv.01 --size=1 --grow --ondisk=vda
volgroup fedora pv.01
logvol / --vgname=fedora --size=1 --grow --name=root
bootloader --location=mbr --boot-drive=vda

firstboot --disable
selinux --permissive
firewall --enabled --ssh
services --enabled=sshd

%packages
@^server-product-environment
@standard
openssh-server
curl
wget
vim-enhanced
rsync
%end

%post --log=/root/ks-post.log
echo "%wheel ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/wheel-nopasswd
chmod 440 /etc/sudoers.d/wheel-nopasswd
${ssh_pubkey_block}
echo "collei-auto-install finished at $$(date -Iseconds)" > /etc/.collei-installed
%end

poweroff
"""

OPENEULER_KICKSTART_TEMPLATE = """#version=openEuler 24.03
# openEuler 24.03 自动安装 kickstart 模板
# 由 collei-install.py -o 渲染

text
lang en_US.UTF-8
keyboard --vckeymap=us --xlayouts='us'
timezone ${timezone} --isUtc

rootpw --plaintext ${root_password}
user --name=${user_name} --groups=wheel --plaintext --password=${user_password}

network --bootproto=dhcp --device=link --onboot=yes --hostname=${hostname}

ignoredisk --only-use=vda
clearpart --all --initlabel
part /boot/efi --fstype=efi --size=600 --ondisk=vda --fsoptions="umask=0077,shortname=winnt"
part /boot --fstype=xfs --size=1024 --ondisk=vda
part pv.01 --size=1 --grow --ondisk=vda
volgroup openeuler pv.01
logvol / --vgname=openeuler --size=1 --grow --name=root
bootloader --location=mbr --boot-drive=vda

firstboot --disable
selinux --permissive
firewall --enabled --ssh
services --enabled=sshd

%packages
@core
@base
openssh-server
curl
wget
vim
rsync
%end

%post --log=/root/ks-post.log
echo "%wheel ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/wheel-nopasswd
chmod 440 /etc/sudoers.d/wheel-nopasswd
sed -i 's/^#*UseDNS.*/UseDNS no/' /etc/ssh/sshd_config
echo "UseDNS no" >> /etc/ssh/sshd_config
sed -i 's/ONBOOT=no/ONBOOT=yes/' /etc/sysconfig/network-scripts/ifcfg-*
${ssh_pubkey_block}
echo "collei-auto-install finished at $$(date -Iseconds)" > /etc/.collei-installed
%end

poweroff
"""


class VmInstaller:
    gitignore = "*.qcow2\n*.qcow2.bak\nimg/\ninstall/\ndump\n.bash_history\n"

    def __init__(
        self,
        context: ColleiContext,
        runner: CommandRunner,
        *,
        initialize_git: bool = True,
    ) -> None:
        self.context = context
        self.runner = runner
        self.initialize_git = initialize_git

    def next_guest_id(self) -> int:
        guest_ids = [
            vm.config.guest_id
            for vm in self.context.list_vms()
            if vm.config.options.get("id") is not None
        ]
        # 为什么不是 0 而是 10, 参考 setup_vsock。
        return max(guest_ids, default=10) + 1

    def validate_new_vm(self, name: str) -> Path:
        if not re.fullmatch(r"[A-Za-z0-9_.-]+", name):
            raise ColleiError(f"invalid VM name: {name}")
        vm_dir = self.context.global_config.vm_root / name
        if vm_dir.exists():
            raise ColleiError(f"{name} is duplicated to {vm_dir}")
        return vm_dir

    def create_vm_layout(self, name: str) -> Path:
        vm_dir = self.validate_new_vm(name)
        print(f"install to {vm_dir}")
        (vm_dir / "opt").mkdir(parents=True)
        (vm_dir / "img").mkdir()
        (vm_dir / "s").mkdir()
        (vm_dir / "t").mkdir()
        # 在这里是一个好思路，不断的:
        # 默认新安装的是 ovmf
        (vm_dir / "opt/bios").write_text("ovmf_binary\n")
        (vm_dir / "opt/id").write_text(f"{self.next_guest_id()}\n")
        (vm_dir / "opt/uuid").write_text(f"{uuid.uuid4()}\n")
        return vm_dir

    def create_standard_disks(
        self,
        vm_dir: Path,
        *,
        disk_count: int,
        raw: bool = False,
        boot_size: str | None = None,
    ) -> None:
        create_standard_boot_disks(
            vm_dir,
            self.runner,
            disk_count=disk_count,
            raw=raw,
            boot_size=boot_size,
        )

        disk_lines = [
            "# supporte type :",
            "#\t\tnvme",
            "# \tide",
            "# \tvirtio-blk",
            "# \tvirtio-scsi",
        ]
        for index in range(1, disk_count + 1):
            disk_lines.append(f"boot{index} virtio-blk {index}")
        (vm_dir / "opt/disk").write_text("\n".join(disk_lines) + "\n")

        # setup_scsi_hba 默认提供两个调试盘，保持与原启动流程一致。
        image_dir = vm_dir / "img"
        image_dir.mkdir(parents=True, exist_ok=True)
        for index in (1, 2):
            create_missing_disk(self.runner, image_dir / f"virtio-scsi_{index}", "10G")

    def init_git(self, vm_dir: Path, git_add_args: list[str] | None = None) -> None:
        (vm_dir / ".gitignore").write_text(self.gitignore)
        self.runner.run(["git", "init"], cwd=vm_dir)
        self.runner.run(git_add_args or ["git", "add", "-A"], cwd=vm_dir)
        self.runner.run(["git", "commit", "-m", "init"], cwd=vm_dir)

    def build_kickstart_iso(self, ks_content: str, output: Path) -> None:
        with tempfile.TemporaryDirectory(prefix="collei-ks-") as temporary:
            source = Path(temporary)
            (source / "ks.cfg").write_text(ks_content)
            self.runner.run(
                [
                    "genisoimage",
                    "-o",
                    output,
                    "-R",
                    "-J",
                    "-V",
                    FEDORA_KS_LABEL,
                    source,
                ]
            )
        if not output.is_file():
            raise ColleiError(f"failed to create kickstart ISO: {output}")

    @staticmethod
    def default_ssh_pubkey() -> str:
        for name in ("id_ed25519.pub", "id_rsa.pub"):
            path = Path.home() / ".ssh" / name
            if path.is_file():
                return path.read_text().strip()
        return ""

    @staticmethod
    def ssh_pubkey_block(user_name: str, ssh_pubkey: str) -> str:
        if not ssh_pubkey:
            return "# 未提供 SSH 公钥"
        home = f"/home/{user_name}"
        return "\n".join(
            [
                f"mkdir -p {home}/.ssh",
                f"chmod 700 {home}/.ssh",
                f"cat > {home}/.ssh/authorized_keys <<'EOF_COLLEI_SSH_KEY'",
                ssh_pubkey,
                "EOF_COLLEI_SSH_KEY",
                f"chmod 600 {home}/.ssh/authorized_keys",
                f"chown -R {user_name}:{user_name} {home}/.ssh",
            ]
        )

    def finish(self, vm_dir: Path) -> VmRuntime:
        link = self.context.global_config.default_vm_link
        link.unlink(missing_ok=True)
        link.symlink_to(vm_dir)
        if self.initialize_git:
            self.init_git(vm_dir)
        return self.context.vm(name=vm_dir.name)


class VirtmeInstaller(VmInstaller):
    """对应 collei.sh 的 install_vm_dir + setup_opt_dir_virtme。"""

    def install(self, name: str, *, disk_count: int = 1) -> VmRuntime:
        vm_dir = self.create_vm_layout(name)
        options = vm_dir / "opt"

        # virtme-ng 使用 virtio-fs 共享 rootfs
        # 基础配置
        (options / "virtme").write_text("1\n")
        (options / "virtme_mode").write_text("manual\n")
        kernel = Path.home() / "data" / "kernel" / "linux-build"
        (options / "kernel").write_text(f"{kernel.resolve()}\n")
        # 启用可写 overlay ，不然很多命令执行都会报错
        (options / "virtme_rw").write_text("1\n")

        # 可选配置
        # virtme_exec: 启动时执行的脚本

        self.create_standard_disks(vm_dir, disk_count=disk_count)
        return self.finish(vm_dir)


class VmtestInstaller(VmInstaller):
    """对应 install_vm_dir + setup_opt_dir_vmtest。"""

    def install(
        self,
        name: str,
        *,
        disk_count: int = 1,
        raw: bool = False,
    ) -> VmRuntime:
        vm_dir = self.create_vm_layout(name)
        cmdline = (
            " rootfstype=9p rootflags=trans=virtio,cache=mmap,msize=1048576 rw"
            " init=/tmp/martins3/init.sh  loglevel=7 raid=noautodetect "
            " printk.devkmsg=on"
        )
        (vm_dir / "opt/cmdline").write_text(f"{cmdline}\n")
        (vm_dir / "opt/vmtest").write_text("1\n")
        (vm_dir / "opt/install").write_text("1\n")
        (vm_dir / "opt/kernel").write_text(
            f"{Path.home() / 'data/kernel/linux-vmtest'}\n"
        )
        self.create_standard_disks(vm_dir, disk_count=disk_count, raw=raw)
        return self.finish(vm_dir)


class IsoInstaller(VmInstaller):
    """对应 choose_vm_dir_for_iso + install_vm_dir。"""

    def choose_iso(self) -> Path:
        iso_root = Path(self.context.global_config.directory.require("iso"))
        images = sorted(iso_root.glob("*.iso"))
        if not images:
            raise ColleiError(f"[{iso_root}] is empty")
        selected = self.runner.run(
            ["fzf"], capture=True, input_text="".join(f"{image}\n" for image in images)
        )
        iso = Path(selected.stdout.strip())
        if iso not in images:
            raise ColleiError(f"invalid ISO selection: {iso}")
        return iso

    def validate_iso(self, iso: Path) -> Path:
        iso_root = Path(self.context.global_config.directory.require("iso")).resolve()
        iso = iso.resolve()
        if not iso.is_file() or iso.parent != iso_root:
            raise ColleiError(f"invalid ISO: {iso}")
        return iso

    def install(
        self,
        name: str,
        iso: Path,
        *,
        disk_count: int = 1,
        raw: bool = False,
    ) -> VmRuntime:
        iso = self.validate_iso(iso)
        vm_dir = self.create_vm_layout(name)
        (vm_dir / "opt/iso").write_text(f"{iso.name} 0\n")
        lower_name = iso.name.lower()
        if "win" in lower_name:
            (vm_dir / "opt/win").write_text(
                "11\n" if "win11" in lower_name else "unknown\n"
            )
        self.create_standard_disks(vm_dir, disk_count=disk_count, raw=raw)
        return self.finish(vm_dir)


class KickstartConfig(Protocol):
    """KickstartAutoInstaller 基类实际用到的 config 字段（只读）。"""

    @property
    def disk_size(self) -> str: ...
    @property
    def disk_count(self) -> int: ...
    @property
    def user(self) -> str: ...
    @property
    def source_label(self) -> str: ...
    @property
    def no_reboot(self) -> bool: ...


ConfigT = TypeVar("ConfigT", bound=KickstartConfig)


class KickstartAutoInstaller(VmInstaller, Generic[ConfigT]):
    install_name = "auto"
    install_temp_prefix = "collei-auto-install-"

    # 由子类 __init__ 赋值，类型为子类各自的 config dataclass。
    config: ConfigT

    def choose_iso(self) -> Path:
        raise NotImplementedError

    def default_vm_name(self, iso: Path) -> str:
        raise NotImplementedError

    def install_cmdline(self, source_label: str) -> str:
        raise NotImplementedError

    def render_kickstart(self, *, iso: Path, hostname: str) -> str:
        raise NotImplementedError

    @staticmethod
    def anaconda_cmdline(source_label: str, *extra_args: str) -> str:
        source = source_label.replace(" ", r"\x20")
        return " ".join(
            (
                "inst.ks=hd:LABEL=KICKSTART:/ks.cfg",
                f"inst.stage2=hd:LABEL={source}",
                f"inst.repo=hd:LABEL={source}",
                *extra_args,
            )
        )

    def validate_iso(self, iso: Path) -> Path:
        iso_root = Path(self.context.global_config.directory.require("iso")).resolve()
        iso = iso.resolve()
        if not iso.is_file() or iso.parent != iso_root:
            raise ColleiError(f"invalid {self.install_name} ISO: {iso}")
        return iso

    def get_iso_volume_id(self, iso: Path) -> str:
        result = self.runner.run(["isoinfo", "-d", "-i", iso], capture=True)
        for line in result.stdout.splitlines():
            if line.startswith("Volume id:"):
                return line.split(":", 1)[1].strip()
        raise ColleiError(f"cannot read ISO Volume id: {iso}")

    def extract_installer(self, iso: Path, dest: Path) -> tuple[Path, Path]:
        dest.mkdir(parents=True, exist_ok=True)
        self.runner.run(["7z", "x", iso, f"-o{dest}", *INSTALLER_PXEBOOT_FILES])
        kernel = dest / INSTALLER_PXEBOOT_FILES[0]
        initrd = dest / INSTALLER_PXEBOOT_FILES[1]
        if not kernel.is_file() or not initrd.is_file():
            raise ColleiError(
                f"failed to extract {self.install_name} installer kernel/initrd"
            )
        return kernel, initrd

    def copy_install_media(self, vm_dir: Path, *, kernel: Path, initrd: Path) -> Path:
        install_dir = vm_dir / "install"
        install_dir.mkdir(parents=True, exist_ok=True)
        shutil.copy2(kernel, install_dir / "vmlinuz")
        shutil.copy2(initrd, install_dir / "initrd.img")
        return install_dir

    def finish(self, vm_dir: Path) -> VmRuntime:
        link = self.context.global_config.default_vm_link
        link.unlink(missing_ok=True)
        link.symlink_to(vm_dir)
        if self.initialize_git:
            self.init_git(
                vm_dir,
                [
                    "git",
                    "add",
                    "-A",
                    "--",
                    ".",
                    ":!opt/kernel",
                    ":!opt/initrd",
                    ":!opt/cmdline",
                    ":!opt/install",
                    ":!opt/iso",
                ],
            )
        return self.context.vm(name=vm_dir.name)

    def write_install_options(
        self,
        vm_dir: Path,
        *,
        iso: Path,
        install_dir: Path,
        source_label: str,
        no_reboot: bool = False,
    ) -> None:
        (install_dir / "once").write_text("1\n")
        opt = vm_dir / "opt"
        (opt / "kernel").write_text(f"{install_dir / 'vmlinuz'}\n")
        (opt / "initrd").write_text(f"{install_dir / 'initrd.img'}\n")
        (opt / "iso").write_text(f"{iso.name}\n{install_dir / 'ks.iso'}\n")
        (opt / "cmdline").write_text(f"{self.install_cmdline(source_label)}\n")
        (opt / "user").write_text(f"{self.config.user}\n")
        (opt / "bg").write_text("1\n")
        (opt / "display").write_text("virtio-gpu\n")
        (opt / "install").write_text("1\n")
        if no_reboot:
            (opt / "no_reboot").write_text("1\n")

    def install(self) -> VmRuntime:
        iso = self.choose_iso()
        name = self.default_vm_name(iso)
        print(f"{self.install_name} ISO: {iso}")
        print(f"VM name: {name}")

        iso = self.validate_iso(iso)
        source_label = self.config.source_label or self.get_iso_volume_id(iso)
        vm_dir = self.create_vm_layout(name)
        self.create_standard_disks(
            vm_dir,
            disk_count=self.config.disk_count,
            raw=False,
            boot_size=self.config.disk_size,
        )
        with tempfile.TemporaryDirectory(prefix=self.install_temp_prefix) as temporary:
            kernel, initrd = self.extract_installer(iso, Path(temporary) / "iso")
            install_dir = self.copy_install_media(vm_dir, kernel=kernel, initrd=initrd)
        self.build_kickstart_iso(
            self.render_kickstart(iso=iso, hostname=name),
            install_dir / "ks.iso",
        )
        self.write_install_options(
            vm_dir,
            iso=iso,
            install_dir=install_dir,
            source_label=source_label,
            no_reboot=self.config.no_reboot,
        )
        return self.finish(vm_dir)


@dataclass(frozen=True)
class FedoraAutoInstallConfig:
    disk_size: str = "350G"
    disk_count: int = 1
    user: str = "martins3"
    user_password: str = "a"
    root_password: str = "fedora"
    timezone: str = "Asia/Shanghai"
    ssh_pubkey: str = ""
    source_label: str = ""
    no_reboot: bool = False


class FedoraAutoInstaller(KickstartAutoInstaller[FedoraAutoInstallConfig]):
    install_name = "Fedora"
    install_temp_prefix = "collei-fedora-install-"

    def __init__(
        self,
        context: ColleiContext,
        runner: CommandRunner,
        *,
        config: FedoraAutoInstallConfig,
        initialize_git: bool = True,
    ) -> None:
        super().__init__(context, runner, initialize_git=initialize_git)
        self.config = config

    def install_cmdline(self, source_label: str) -> str:
        return self.anaconda_cmdline(
            source_label,
            "console=tty0 console=ttyS0,115200 quiet",
        )

    @staticmethod
    def fedora_version(iso: Path) -> int:
        match = re.search(r"Fedora-Server.*-(\d+)-", iso.name)
        if match is None:
            return 0
        return int(match.group(1))

    def choose_iso(self) -> Path:
        iso_root = Path(self.context.global_config.directory.require("iso"))
        images = sorted(
            iso_root.glob("Fedora-Server*.iso"),
            key=lambda image: (self.fedora_version(image), image.name),
        )
        if not images:
            raise ColleiError(f"no Fedora Server ISO in {iso_root}")
        return images[-1]

    def default_vm_name(self, iso: Path) -> str:
        version = self.fedora_version(iso)
        base = f"fedora{version}-auto-real" if version else "fedora-auto-real"
        if not (self.context.global_config.vm_root / base).exists():
            return base
        suffix = 2
        while True:
            candidate = f"{base}-{suffix}"
            if not (self.context.global_config.vm_root / candidate).exists():
                return candidate
            suffix += 1

    def render_kickstart(self, *, iso: Path, hostname: str) -> str:
        del iso
        ssh_pubkey = self.config.ssh_pubkey or self.default_ssh_pubkey()
        return Template(FEDORA_KICKSTART_TEMPLATE).substitute(
            {
                "root_password": self.config.root_password,
                "user_name": self.config.user,
                "user_password": self.config.user_password,
                "ssh_pubkey_block": self.ssh_pubkey_block(self.config.user, ssh_pubkey),
                "hostname": hostname,
                "timezone": self.config.timezone,
            }
        )


@dataclass(frozen=True)
class OpeneulerAutoInstallConfig:
    disk_size: str = "350G"
    disk_count: int = 1
    user: str = "martins3"
    user_password: str = "a"
    root_password: str = "openeuler"
    timezone: str = "Asia/Shanghai"
    ssh_pubkey: str = ""
    source_label: str = ""
    no_reboot: bool = False


class OpeneulerAutoInstaller(KickstartAutoInstaller[OpeneulerAutoInstallConfig]):
    install_name = "openEuler"
    install_temp_prefix = "collei-openeuler-install-"

    def __init__(
        self,
        context: ColleiContext,
        runner: CommandRunner,
        *,
        config: OpeneulerAutoInstallConfig,
        initialize_git: bool = True,
    ) -> None:
        super().__init__(context, runner, initialize_git=initialize_git)
        self.config = config

    def install_cmdline(self, source_label: str) -> str:
        return self.anaconda_cmdline(
            source_label,
            "console=tty0 console=ttyS0,115200 quiet fpi_to_tail=off",
        )

    @staticmethod
    def openeuler_version(iso: Path) -> str:
        match = re.search(r"openEuler-(\d+\.\d+)-LTS(-SP\d+)?", iso.name)
        if match is None:
            return ""
        version = match.group(1).replace(".", "")
        sp = match.group(2) or ""
        return f"{version}{sp.lower().replace('-', '')}"

    def choose_iso(self) -> Path:
        iso_root = Path(self.context.global_config.directory.require("iso"))
        images = sorted(
            iso_root.glob("openEuler-*.iso"),
            key=lambda image: (self.openeuler_version(image), image.name),
        )
        if not images:
            raise ColleiError(f"no openEuler ISO in {iso_root}")
        return images[-1]

    def default_vm_name(self, iso: Path) -> str:
        version = self.openeuler_version(iso)
        base = f"openeuler{version}-auto-real" if version else "openeuler-auto-real"
        if not (self.context.global_config.vm_root / base).exists():
            return base
        suffix = 2
        while True:
            candidate = f"{base}-{suffix}"
            if not (self.context.global_config.vm_root / candidate).exists():
                return candidate
            suffix += 1

    def render_kickstart(self, *, iso: Path, hostname: str) -> str:
        del iso
        ssh_pubkey = self.config.ssh_pubkey or self.default_ssh_pubkey()
        return Template(OPENEULER_KICKSTART_TEMPLATE).substitute(
            {
                "root_password": self.config.root_password,
                "user_name": self.config.user,
                "user_password": self.config.user_password,
                "ssh_pubkey_block": self.ssh_pubkey_block(self.config.user, ssh_pubkey),
                "hostname": hostname,
                "timezone": self.config.timezone,
            }
        )


class NixosInstaller(VmInstaller):
    """对应 choose_vm_dir_for_nixos + create_nixos_rootfs。"""

    def create_rootfs(self, path: Path) -> None:
        # 保留 create_nixos_rootfs 的 raw -> ext4 -> qcow2 流程。
        with tempfile.NamedTemporaryFile() as temporary:
            self.runner.run(["qemu-img", "create", "-f", "raw", temporary.name, "500G"])
            self.runner.run(["mkfs.ext4", "-F", "-L", "nixos", temporary.name])
            self.runner.run(
                [
                    "qemu-img",
                    "convert",
                    "-f",
                    "raw",
                    "-O",
                    "qcow2",
                    temporary.name,
                    path,
                ]
            )

    def install(self, name: str) -> VmRuntime:
        vm_dir = self.create_vm_layout(name)
        (vm_dir / "shared").mkdir()
        (vm_dir / "xchg").mkdir()
        self.create_standard_disks(vm_dir, disk_count=1)
        self.create_rootfs(vm_dir / "2.qcow2")
        self.runner.run(
            [
                "nixos-rebuild",
                "build-vm",
                "-I",
                f"nixos-config={self.context.repo / 'configuration.nix'}",
            ],
            cwd=vm_dir,
        )
        return self.finish(vm_dir)


def install_vm(
    context: ColleiContext,
    mode: str,
    runner: CommandRunner,
    *,
    name: str | None = None,
    iso: Path | None = None,
    config: FedoraAutoInstallConfig | None = None,
    openeuler_config: OpeneulerAutoInstallConfig | None = None,
    disk_count: int = 1,
    raw: bool = False,
    initialize_git: bool = True,
) -> VmRuntime:
    if mode == "virtme":
        if name is None:
            raise ColleiError("virtme install requires name")
        installer = VirtmeInstaller(context, runner, initialize_git=initialize_git)
        return installer.install(name, disk_count=disk_count)
    if mode == "vmtest":
        if name is None:
            raise ColleiError("vmtest install requires name")
        installer = VmtestInstaller(context, runner, initialize_git=initialize_git)
        return installer.install(name, disk_count=disk_count, raw=raw)
    if mode == "iso":
        if name is None or iso is None:
            raise ColleiError("iso install requires name and iso")
        installer = IsoInstaller(context, runner, initialize_git=initialize_git)
        return installer.install(name, iso, disk_count=disk_count, raw=raw)
    if mode == "fedora":
        installer = FedoraAutoInstaller(
            context,
            runner,
            config=config or FedoraAutoInstallConfig(),
            initialize_git=initialize_git,
        )
        return installer.install()
    if mode == "openeuler":
        installer = OpeneulerAutoInstaller(
            context,
            runner,
            config=openeuler_config or OpeneulerAutoInstallConfig(),
            initialize_git=initialize_git,
        )
        return installer.install()
    if mode == "nixos":
        if name is None:
            raise ColleiError("nixos install requires name")
        installer = NixosInstaller(context, runner, initialize_git=initialize_git)
        return installer.install(name)
    raise ColleiError(f"unsupported install mode: {mode}")


def choose_vm_name(
    context: ColleiContext, runner: CommandRunner, candidate: str
) -> str:
    # parameter：备选名称
    # return : new_vm_name（虚拟机名）
    name_file = Path("/tmp/martins3/vm_name")
    name_file.parent.mkdir(parents=True, exist_ok=True)
    name_file.write_text(f"{candidate}\n")
    runner.run(["nvim", name_file])
    fields = name_file.read_text().split()
    if not fields:
        raise ColleiError("empty VM name")
    name = fields[0]
    if (context.global_config.vm_root / name).exists():
        raise ColleiError(f"duplicate VM name: {name}")
    return name


def choose_virtme_vm_name(context: ColleiContext, runner: CommandRunner) -> str:
    return choose_vm_name(context, runner, "virtme")


def choose_disk_layout(runner: CommandRunner) -> tuple[int, bool]:
    del runner
    disk_count = int(choose(("1", "3"), prompt="Disk count"))
    raw = False
    if disk_count == 1:
        raw = confirm("raw boot image")
    return disk_count, raw


def choose_iso(context: ColleiContext, runner: CommandRunner) -> Path:
    installer = IsoInstaller(context, runner, initialize_git=False)
    return installer.choose_iso()
