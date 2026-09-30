# ============================================
# init_run：自定义 init（-kernel/-initrd）
# ============================================
from __future__ import annotations

import os
import shutil
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path

from commands import CommandRunner
from errors import ColleiError
from initramfs import create_device_nodes, is_statically_linked, pack_cpio_zst
from runtime import ColleiContext, VmRuntime


def _static_cc_candidates() -> list[str]:
    # PATH 里的 gcc（例如 nix 工具链）可能缺静态 libc，逐个尝试并退回系统 gcc。
    candidates = [shutil.which("gcc"), shutil.which("cc"), "/usr/bin/gcc"]
    result: list[str] = []
    for candidate in candidates:
        if (
            candidate is not None
            and candidate not in result
            and Path(candidate).is_file()
        ):
            result.append(candidate)
    return result


@dataclass(frozen=True)
class InitRunSetup:
    """custom-init 启动：-kernel/-initrd，程序本身作为 PID 1。

    init_run 开关替换掉整个 init 内容。只要 VM 配置了 kernel（直接 -kernel
    启动）即可使用，与 virtme/vmtest 等模式无关；程序固定为 repo 下的
    init/init.c（构建产物 init/init.out）。
    """

    context: ColleiContext
    vm: VmRuntime

    @property
    def _source(self) -> Path:
        return self.context.repo / "init" / "init.c"

    @property
    def _program(self) -> Path:
        return self.context.repo / "init" / "init.out"

    def prepare_init_program(self) -> Path:
        source, program = self._source, self._program
        if not source.is_file():
            raise ColleiError(f"init_run source missing: {source}")
        if (
            not program.is_file()
            or program.stat().st_mtime_ns < source.stat().st_mtime_ns
        ):
            compilers = _static_cc_candidates()
            if not compilers:
                raise ColleiError("gcc not found, cannot build the init program")
            diagnostics: list[str] = []
            for compiler in compilers:
                completed = subprocess.run(
                    [compiler, "-static", "-O2", "-o", str(program), str(source)],
                    check=False,
                    capture_output=True,
                    text=True,
                )
                if completed.returncode == 0:
                    break
                diagnostics.append(
                    f"{compiler}: {completed.stderr.strip() or 'failed'}"
                )
            else:
                raise ColleiError(
                    f"failed to build {source} (static linking requires "
                    f"glibc-static):\n" + "\n".join(diagnostics)
                )
        if not program.is_file() or not os.access(program, os.X_OK):
            raise ColleiError(f"init program missing or not executable: {program}")
        # 动态链接程序在没有动态加载器的 initramfs 里做 init 只会
        # panic (no working init)，必须在 host 侧就挡掉。
        if not is_statically_linked(program):
            raise ColleiError(f"init program must be statically linked: {program}")
        return program

    @property
    def _initramfs_path(self) -> Path:
        return self.vm.qemu_directory / "init-run-initramfs.cpio.zst"

    def generate_initramfs(self) -> Path:
        program = self.prepare_init_program()
        archive = self._initramfs_path
        # 内容只有一个文件，按二进制 mtime 判断新旧即可，无需 stamp 哈希。
        if (
            archive.is_file()
            and archive.stat().st_mtime_ns >= program.stat().st_mtime_ns
        ):
            return archive
        with tempfile.TemporaryDirectory(prefix="collei-init-run-") as temporary:
            root = Path(temporary)
            create_device_nodes(root)
            shutil.copy2(program, root / "init")
            (root / "init").chmod(0o755)
            pack_cpio_zst(root, archive)
        return archive

    def initramfs(self, kernel_dir: Path, configured: Path | None) -> Path:
        del kernel_dir, configured
        return self._initramfs_path

    def prepare(self, runner: CommandRunner) -> None:
        del runner
        self.generate_initramfs()

    def fallback_boot_disks(self) -> list[tuple[str, str, str | None]] | None:
        return None

    def share_directory(self) -> str | None:
        return None

    @property
    def force_foreground(self) -> bool:
        return False

    def kernel_args(self) -> str:
        # 普通的 init 程序：stdio 即内核 console，不需要 virtme 之类参数。
        args = ["nokaslr", "mitigations=off", "loglevel=8"]
        cmdline = self.vm.config.options.get("cmdline")
        if cmdline is not None:
            args.append(cmdline)
        return " " + " ".join(args) + " "

    def rootfs_arguments(self) -> tuple[str, ...]:
        return ()
