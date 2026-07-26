#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import resource
import sys
from collections.abc import Sequence
from dataclasses import dataclass
from pathlib import Path

from commands import CommandRunner
from errors import ColleiError

# echo DMA | sudo tee /sys/kernel/iommu_groups/5/type

@dataclass(frozen=True)
class VfioPaths:
    pci_devices: Path = Path("/sys/bus/pci/devices")
    pci_drivers: Path = Path("/sys/bus/pci/drivers")
    vfio_driver: Path = Path("/sys/bus/pci/drivers/vfio-pci")
    iommu_class: Path = Path("/sys/class/iommu")
    vfio_module: Path = Path("/sys/module/vfio_pci")
    unsafe_interrupts: Path = Path(
        "/sys/module/vfio_iommu_type1/parameters/allow_unsafe_interrupts"
    )
    modules_load_config: Path = Path("/etc/modules-load.d/vfio.conf")
    vfio_devices: Path = Path("/dev/vfio")
    iommufd: Path = Path("/dev/iommu")
    state_directory: Path = Path.home() / ".local/state/collei/vfio"


def _change_owner(path: Path, runner: CommandRunner) -> None:
    if path.exists() and path.stat().st_uid != os.getuid():
        runner.run(["sudo", "chown", os.environ.get("USER", "martins3"), path])


def _iommu_group(device: Path) -> str:
    group = device / "iommu_group"
    if not group.is_symlink():
        raise ColleiError(f"failed to get iommu group of {device.name}")
    return group.resolve().name


def _change_vfio_owners(group: str, paths: VfioPaths, runner: CommandRunner) -> None:
    _change_owner(paths.vfio_devices / group, runner)
    _change_owner(paths.iommufd, runner)
    if paths.iommufd.exists():
        for device in (paths.vfio_devices / "devices").glob("*"):
            _change_owner(device, runner)


def _check_iommu(paths: VfioPaths) -> None:
    if not paths.iommu_class.is_dir() or not any(paths.iommu_class.iterdir()):
        raise ColleiError(
            "iommu is not enabled; run: sudo grubby --update-kernel=ALL "
            '--args="intel_iommu=on iommu=pt"'
        )


def _load_vfio_modules(paths: VfioPaths, runner: CommandRunner) -> None:
    if paths.vfio_module.is_dir():
        return
    runner.run(
        ["sudo", "tee", paths.modules_load_config],
        input_text="vfio vfio-pci\n",
    )
    runner.run(["sudo", "modprobe", "vfio-pci"])


def _allow_virtio_iommu(paths: VfioPaths, runner: CommandRunner) -> None:
    if any("virtio" in iommu.name for iommu in paths.iommu_class.iterdir()):
        runner.run(["sudo", "tee", paths.unsafe_interrupts], input_text="1\n")


def _check_memlock_limit() -> None:
    soft_limit, _hard_limit = resource.getrlimit(resource.RLIMIT_MEMLOCK)
    if soft_limit == resource.RLIM_INFINITY:
        return
    raise ColleiError(
        "VFIO requires unlimited memlock; add '* hard memlock unlimited' and "
        "'* soft memlock unlimited' to /etc/security/limits.conf, then start a "
        "new login session"
    )


def _current_driver(device: Path) -> str | None:
    driver = device / "driver"
    return driver.resolve().name if driver.is_symlink() else None


def _state_file(bdf: str, paths: VfioPaths) -> Path:
    return paths.state_directory / f"{bdf}.driver"


def _record_original_driver(bdf: str, driver: str, paths: VfioPaths) -> None:
    paths.state_directory.mkdir(parents=True, exist_ok=True)
    _state_file(bdf, paths).write_text(f"{driver}\n")


def pci_bind_to_vfio(
    bdf: str,
    runner: CommandRunner,
    *,
    paths: VfioPaths | None = None,
) -> None:
    """Translate vfio.sh:pci_bind_to_vfio for native Python launches."""
    vfio_paths = paths or VfioPaths()
    _check_iommu(vfio_paths)
    _load_vfio_modules(vfio_paths, runner)

    device = vfio_paths.pci_devices / bdf
    if not device.is_dir():
        raise ColleiError(f"invalid PCI BDF: {bdf}")

    group = _iommu_group(device)
    _change_vfio_owners(group, vfio_paths, runner)
    driver_name = _current_driver(device)
    if driver_name == "vfio-pci":
        return
    if driver_name is not None:
        _record_original_driver(bdf, driver_name, vfio_paths)

    _allow_virtio_iommu(vfio_paths, runner)
    _check_memlock_limit()

    driver = device / "driver"
    unbind = driver / "unbind"
    if unbind.is_file():
        runner.run(["sudo", "tee", unbind], input_text=f"{bdf}\n")

    vendor = (device / "vendor").read_text().strip().removeprefix("0x")
    device_id = (device / "device").read_text().strip().removeprefix("0x")
    vendor_device = f"{vendor} {device_id}\n"
    new_id = vfio_paths.vfio_driver / "new_id"
    result = runner.run(["sudo", "tee", new_id], input_text=vendor_device, check=False)
    if result.returncode:
        runner.run(
            ["sudo", "tee", vfio_paths.vfio_driver / "remove_id"],
            input_text=vendor_device,
        )
        runner.run(["sudo", "tee", new_id], input_text=vendor_device)

    _change_vfio_owners(_iommu_group(device), vfio_paths, runner)


def unbind_pci_device(
    bdf: str,
    runner: CommandRunner,
    *,
    paths: VfioPaths | None = None,
) -> str | None:
    vfio_paths = paths or VfioPaths()
    device = vfio_paths.pci_devices / bdf
    if not device.is_dir():
        raise ColleiError(f"invalid PCI BDF: {bdf}")

    driver = _current_driver(device)
    if driver is None:
        return None
    if driver != "vfio-pci":
        _record_original_driver(bdf, driver, vfio_paths)

    unbind = device / "driver/unbind"
    if not unbind.is_file():
        raise ColleiError(f"driver unbind is unavailable: {bdf}")
    runner.run(["sudo", "tee", unbind], input_text=f"{bdf}\n")
    if _current_driver(device) is not None:
        raise ColleiError(f"failed to unbind {bdf} from {driver}")
    return driver


def _resolve_default_driver(
    bdf: str, device: Path, paths: VfioPaths, runner: CommandRunner
) -> str:
    saved = _state_file(bdf, paths)
    if saved.is_file():
        driver = saved.read_text().strip()
        if driver:
            return driver

    modalias_file = device / "modalias"
    if not modalias_file.is_file():
        raise ColleiError(f"PCI modalias is unavailable: {bdf}")
    result = runner.run(
        ["modprobe", "--resolve-alias", modalias_file.read_text().strip()],
        capture=True,
    )
    for module in result.stdout.splitlines():
        module = module.strip()
        if module and module not in {"vfio", "vfio-pci", "vfio_pci"}:
            return module
    raise ColleiError(
        f"cannot resolve the default driver for {bdf}; use --driver explicitly"
    )


def bind_to_default_driver(
    bdf: str,
    runner: CommandRunner,
    *,
    driver: str | None = None,
    paths: VfioPaths | None = None,
) -> str:
    vfio_paths = paths or VfioPaths()
    device = vfio_paths.pci_devices / bdf
    if not device.is_dir():
        raise ColleiError(f"invalid PCI BDF: {bdf}")

    target = driver or _resolve_default_driver(bdf, device, vfio_paths, runner)
    if _current_driver(device) == target:
        return target

    runner.run(["sudo", "modprobe", target])
    driver_override = device / "driver_override"
    if not driver_override.exists():
        raise ColleiError(f"driver_override is unavailable: {bdf}")
    runner.run(["sudo", "tee", driver_override], input_text=f"{target}\n")

    current_driver = device / "driver"
    unbind = current_driver / "unbind"
    if unbind.is_file():
        runner.run(["sudo", "tee", unbind], input_text=f"{bdf}\n")
    runner.run(
        ["sudo", "tee", vfio_paths.pci_devices.parent / "drivers_probe"],
        input_text=f"{bdf}\n",
    )
    if _current_driver(device) != target:
        raise ColleiError(f"failed to bind {bdf} to {target}")

    runner.run(["sudo", "tee", driver_override], input_text="\n")
    _state_file(bdf, vfio_paths).unlink(missing_ok=True)
    return target


def device_status(bdf: str, *, paths: VfioPaths | None = None) -> str:
    vfio_paths = paths or VfioPaths()
    device = vfio_paths.pci_devices / bdf
    if not device.is_dir():
        raise ColleiError(f"invalid PCI BDF: {bdf}")
    return _current_driver(device) or "unbound"


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Bind a PCI device to vfio-pci, unbind it, or restore its default driver."
        )
    )
    subparsers = parser.add_subparsers(dest="action", required=True)
    for action in ("bind", "unbind", "status"):
        command = subparsers.add_parser(action)
        command.add_argument("bdf", help="PCI BDF, for example 0000:02:00.0")
    default = subparsers.add_parser(
        "default", aliases=["restore"], help="restore the device's normal driver"
    )
    default.add_argument("bdf", help="PCI BDF, for example 0000:02:00.0")
    default.add_argument("--driver", help="override the detected default driver")
    options = parser.parse_args(argv)

    try:
        if options.action == "bind":
            pci_bind_to_vfio(options.bdf, CommandRunner())
        elif options.action == "unbind":
            unbind_pci_device(options.bdf, CommandRunner())
        elif options.action in {"default", "restore"}:
            bind_to_default_driver(options.bdf, CommandRunner(), driver=options.driver)
        print(f"{options.bdf}: {device_status(options.bdf)}")
        return 0
    except (ColleiError, OSError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
