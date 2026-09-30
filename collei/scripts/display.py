"""Shared QEMU display output and guest graphics device settings."""

from __future__ import annotations

from dataclasses import dataclass

from errors import UnsupportedNativeConfiguration
from runtime import VmRuntime


# FIXME 没搞懂为什么需要这个，莫名其妙的封装
@dataclass(frozen=True)
class DisplaySettings:
    output: str
    gpu: str

    @classmethod
    def from_vm(cls, vm: VmRuntime, host_arch: str) -> DisplaySettings:
        options = vm.config.options
        if options.get("display_backend") is not None:
            raise UnsupportedNativeConfiguration(
                "display_backend was removed; use display=vnc, gtk, or none"
            )

        output = options.get("display") or "vnc"
        if output not in {"vnc", "gtk", "none"}:
            raise UnsupportedNativeConfiguration(
                f"unsupported display={output}; use vnc, gtk, or none"
            )

        gpu = options.get("gpu") or "virtio-gpu"
        if gpu not in {"virtio-gpu", "std", "cirrus", "none"}:
            raise UnsupportedNativeConfiguration(
                f"unsupported gpu={gpu}; use virtio-gpu, std, cirrus, or none"
            )
        if host_arch == "aarch64" and gpu in {"std", "cirrus"}:
            raise UnsupportedNativeConfiguration(
                f"gpu={gpu} is only supported on x86_64"
            )
        return cls(output, gpu)

    def qemu_arguments(self, vm: VmRuntime) -> tuple[str, ...]:
        args: list[str] = []
        if self.gpu == "virtio-gpu":
            # Windows installation media needs a VGA-compatible device before
            # its virtio GPU driver is installed. Keep existing Linux hardware.
            device = (
                "virtio-vga" if vm.config.options.enabled("win") else "virtio-gpu-pci"
            )
            args.extend(("-device", device))
        elif self.gpu == "std":
            args.extend(("-vga", "std"))
        elif self.gpu == "cirrus":
            args.extend(("-device", "cirrus-vga"))

        if self.output == "vnc":
            args.extend(("-vnc", f":{vm.tcp_port('vnc') - 5900},password=off"))
        else:
            args.extend(("-display", self.output))
        return tuple(args)
