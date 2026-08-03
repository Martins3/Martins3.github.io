from __future__ import annotations

import getpass
import json
import os
import platform
import re
import shlex
import shutil
import signal
import subprocess
import sys
import time
import uuid
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from typing import Callable, Sequence

from block_migration import (
    BlockMirrorSession,
    MigratableDisk,
    discover_migratable_disks,
    prepare_target_images,
)
from commands import CommandRunner
from errors import ColleiError, ColleiHelp
from host_setup import prepare_ovs_tap
from launch_options import LaunchOptions
from monitor import QmpClient, hmp_command, hmp_commands
from network_templates import network_configurations, temporary_ip_commands
from runtime import ColleiContext, VmRuntime
from tasks import add_background_task, exec_task_log
from ui import choose, confirm


class VmRequirement(Enum):
    ANY = ""
    ACTIVE = "active"
    INACTIVE = "inactive"
    LAUNCH = "launch"
    DEBUG_KERNEL = "debug_kernel"


ActionFunction = Callable[["ActionContext", Sequence[str]], None]


@dataclass(frozen=True)
class Action:
    function: ActionFunction | None = None
    requirement: VmRequirement = VmRequirement.ANY


@dataclass
class ActionContext:
    collei: ColleiContext
    vm: VmRuntime
    runner: CommandRunner
    auto_yes: bool = False

    def ssh_info(self) -> tuple[str, str, int | None]:
        user = self.vm.config.options.get("user") or "root"
        ip = self.vm.config.options.get("ip")
        if ip is not None:
            return user, ip, None
        return user, "localhost", self.vm.tcp_port("ssh")


def _ssh_argv(context: ActionContext, copy_id: bool = False) -> list[str]:
    user, ip, port = context.ssh_info()
    argv = ["ssh-copy-id" if copy_id else "ssh"]
    if port is not None:
        argv.extend(["-p", str(port)])
    argv.append(f"{user}@{ip}")
    return argv


def _ssh_vsock_argv(context: ActionContext) -> list[str]:
    if not context.vm.config.options.enabled("vsock"):
        raise ColleiError(
            f"vsock not enabled for {context.vm.config.name}, run: echo 1 > opt/vsock"
        )
    # 与 virtme kernel_args 的用户选择保持一致: guest 共享 host rootfs，
    # virtiofsd 以普通用户运行，只有该用户的 home 里的 authorized_keys 可读。
    user = (
        context.vm.config.options.get("user")
        or os.environ.get("SUDO_USER")
        or getpass.getuser()
    )
    cid = context.vm.vsock_cid
    socat = shutil.which("socat")
    if socat is None:
        raise ColleiError("socat not found, required for vsock ssh")
    return [
        "ssh",
        "-o",
        "StrictHostKeyChecking=no",
        "-o",
        "UserKnownHostsFile=/dev/null",
        "-o",
        f"ProxyCommand={socat} - VSOCK-CONNECT:{cid}:22",
        f"{user}@virtme",
    ]


def _confirm(context: ActionContext, message: str) -> bool:
    if context.auto_yes or context.runner.dry_run:
        return True
    return confirm(message)


def action_ssh_auto(context: ActionContext, args: Sequence[str]) -> None:
    del args
    print(" ".join(_ssh_argv(context)))


def action_ssh_vsock_auto(context: ActionContext, args: Sequence[str]) -> None:
    del args
    print(shlex.join(_ssh_vsock_argv(context)))


def _ssh_default_argv(context: ActionContext) -> list[str]:
    # virtme VM 启用 vsock 后默认走 vsock SSH: 它不依赖 guest 网络配置。
    options = context.vm.config.options
    if options.enabled("virtme") and options.enabled("vsock"):
        return _ssh_vsock_argv(context)
    return _ssh_argv(context)


def action_ssh(context: ActionContext, args: Sequence[str]) -> None:
    del args
    context.runner.exec(
        _ssh_default_argv(context),
        cwd=context.vm.directory,
        env={"TERM": "xterm-256color"},
    )


def action_ssh_vsock(context: ActionContext, args: Sequence[str]) -> None:
    del args
    context.runner.exec(
        _ssh_vsock_argv(context),
        cwd=context.vm.directory,
        env={"TERM": "xterm-256color"},
    )


def action_ssh_copy_id(context: ActionContext, args: Sequence[str]) -> None:
    del args
    context.runner.exec(_ssh_argv(context, copy_id=True), cwd=context.vm.directory)


def action_default(context: ActionContext, args: Sequence[str]) -> None:
    del args
    print(f"default is : {context.vm.directory}")


def action_run(context: ActionContext, args: Sequence[str]) -> None:
    context.runner.exec([context.collei.scripts / "collei.py", *args])


def action_kill(context: ActionContext, args: Sequence[str]) -> None:
    del args
    if not _confirm(context, f"Kill {context.vm.directory} ?"):
        return
    os.kill(context.vm.pid, signal.SIGTERM)
    deadline = time.monotonic() + 5
    while Path(f"/proc/{context.vm.pid}").exists() and time.monotonic() < deadline:
        time.sleep(0.05)
    print("Done")


def action_force_reboot(context: ActionContext, args: Sequence[str]) -> None:
    del args
    socket = context.vm.directory / context.vm.which_qemu / "hmp"
    hmp_command(socket, "system_reset")


def action_log(context: ActionContext, args: Sequence[str]) -> None:
    del args
    exec_task_log(context.collei, context.runner, context.vm.directory, follow=False)


def action_follow_log(context: ActionContext, args: Sequence[str]) -> None:
    del args
    exec_task_log(context.collei, context.runner, context.vm.directory, follow=True)


def action_vnc(context: ActionContext, args: Sequence[str]) -> None:
    del args
    print(
        f"http://{context.collei.master_ip()}:{context.vm.tcp_port('vnc') + 1}/vnc.html"
    )


def action_hmp(context: ActionContext, args: Sequence[str]) -> None:
    del args
    which_qemu = context.vm.which_qemu
    # 类似这种资源在两个 QEMU 都运行时需要再次选择，但这种情况并不多。
    if len(context.vm.live_pids) == 2:
        source = (context.vm.directory / "migrate_source").read_text().strip()
        if source not in {"s", "t"}:
            raise ColleiError(f"invalid migration source: {source}")
        target = "t" if source == "s" else "s"
        selected = choose(("source", "target"), prompt="HMP QEMU")
        which_qemu = source if selected == "source" else target

    socket = context.vm.directory / which_qemu / "hmp"
    context.runner.exec(["socat", "-,echo=0,icanon=0", f"unix-connect:{socket}"])


def action_edit(context: ActionContext, args: Sequence[str]) -> None:
    del args
    context.runner.exec(["nvim"], cwd=context.vm.directory)


def action_backup(context: ActionContext, args: Sequence[str]) -> None:
    del args
    if context.vm.active and not _confirm(context, "vm is alive, be carefull"):
        return
    image_dir = context.vm.image_directory
    if any(image_dir.glob("boot*.bak")) and not _confirm(
        context, "overwrite existing snapshot ?"
    ):
        return
    for source in sorted(image_dir.glob("boot*")):
        if source.name.endswith(".bak"):
            continue
        target = source.with_name(f"{source.name}.bak")
        if not context.runner.dry_run:
            print(f"cp {source} {target}")
        context.runner.run(["cp", source, target])


def action_restore(context: ActionContext, args: Sequence[str]) -> None:
    del args
    for source in sorted(context.vm.image_directory.glob("boot*.bak")):
        target = source.with_name(source.name.removesuffix(".bak"))
        if not context.runner.dry_run:
            print(f"cp {source} {target}")
        context.runner.run(["cp", source, target])


# 一种高级技巧，让虚拟机可以在任何环境中运行，只要把必要的文件拷贝过去
# 这个项目会自动构建出来 cmd.sh 也就是 qemu 启动脚本
# 这个函数就是让 cmd.sh 的路径被替换掉
def action_path(context: ActionContext, args: Sequence[str]) -> None:
    del args
    command = context.vm.directory / "cmd.sh"
    old_path = str(context.collei.global_config.vm_root)
    new_path = "/root/"
    print(f"[{old_path}]")
    print(f"[{new_path}]")
    command.write_text(command.read_text().replace(old_path, new_path))


def action_rsync(context: ActionContext, args: Sequence[str]) -> None:
    del args
    user, ip, port = context.ssh_info()
    location = "/root" if user == "root" else f"/home/{user}"
    deployment = Path.cwd() / ".nvim" / "deployment.lua"
    deployment.parent.mkdir(parents=True, exist_ok=True)
    fields = ["return {"]
    if port is not None:
        fields.extend([f'\tport = "{port}",', '\tip = "localhost",'])
    else:
        fields.append(f'\tip = "{ip}",')
    fields.extend(
        [
            f'\tuser = "{user}",',
            f'\tlocation = "{location}",',
            "\tignore_git = false,",
            "}",
        ]
    )
    deployment.write_text("\n".join(fields) + "\n")


def action_bg(context: ActionContext, args: Sequence[str]) -> None:
    del args
    original = context.vm.config.options.get("bg") or "1"
    now = "1" if original == "0" else "0"
    (context.vm.config.options.path / "bg").write_text(f"{now}\n")
    print(f"option is : {original} ==> {now}")


def action_top(context: ActionContext, args: Sequence[str]) -> None:
    del args
    proc = Path(f"/proc/{context.vm.pid}")
    print((proc / "sched").read_text(), end="")
    print()
    print((proc / "schedstat").read_text(), end="")
    print()
    print(f"/sys/fs/cgroup/{(proc / 'cgroup').read_text().strip()}")


def action_cgroup(context: ActionContext, args: Sequence[str]) -> None:
    del args
    cgroup_line = Path(f"/proc/{context.vm.pid}/cgroup").read_text().splitlines()[0]
    cgroup = Path("/sys/fs/cgroup") / cgroup_line.split(":", 2)[-1].lstrip("/")
    (cgroup / "memory.high").write_text("1G\n")
    print(cgroup)


def action_setup_vmware(context: ActionContext, args: Sequence[str]) -> None:
    del args
    options = context.vm.config.options.path
    values = {"bios": "ovmf_binary", "netdev": "vmxnet3", "boot": "ide", "sata": "1"}
    for name, value in values.items():
        (options / name).write_text(f"{value}\n")


def action_tty3(context: ActionContext, args: Sequence[str]) -> None:
    del args
    print(f"Sending Ctrl+Alt+F3 to {context.vm.config.name} ...")
    socket = context.vm.directory / context.vm.which_qemu / "hmp"
    hmp_command(socket, "sendkey ctrl-alt-f3")


def action_unplug_disk(context: ActionContext, args: Sequence[str]) -> None:
    del args
    socket = context.vm.directory / context.vm.which_qemu / "hmp"
    hmp_command(socket, "device_del hp_disk0")
    counter = context.vm.directory / context.vm.which_qemu / "hp_disk_counter"
    counter.write_text("0\n")


def action_clone_vm_auto(context: ActionContext, args: Sequence[str]) -> None:
    if len(args) != 1:
        raise ColleiError("clone_vm_auto requires a new VM name")
    target = context.collei.global_config.vm_root / args[0]
    if target.exists():
        raise ColleiError(f"VM already exists: {target}")
    source_active = context.vm.active
    if source_active and not _confirm(context, "vm is alive, be careful"):
        return
    print(f"cp -r {context.vm.directory} {target}")
    context.runner.run(["cp", "-r", context.vm.directory, target])
    (target / "opt" / "ip").unlink(missing_ok=True)
    ids = [vm.config.guest_id for vm in context.collei.list_vms()]
    new_id = max(ids, default=10) + 1
    (target / "opt" / "id").write_text(f"{new_id}\n")
    (target / "opt" / "uuid").write_text(f"{uuid.uuid4()}\n")
    if source_active:
        for pid_file in target.glob("*/pid"):
            pid_file.unlink()
    new_vm = context.collei.vm(target.name)
    context.collei.set_default(new_vm)
    print(f"default vm is {target.name}")


def _fzf(items: str) -> str:
    selected = subprocess.run(
        ["fzf"], input=items, text=True, capture_output=True, check=False
    )
    if selected.returncode or not selected.stdout.strip():
        raise ColleiError("no selection")
    return selected.stdout.strip().splitlines()[0]


def action_monitor(context: ActionContext, args: Sequence[str]) -> None:
    del args
    resource = _fzf("qmp\nshell\nqga\nmain\n")
    monitor_dir = context.vm.directory / context.vm.which_qemu
    if resource == "shell":
        qmp_shell = (
            context.collei.repo.parent.parent / "qemu" / "scripts" / "qmp" / "qmp-shell"
        )
        context.runner.exec([qmp_shell, monitor_dir / "qmp-shell"])
        return
    sockets = {"qmp": "qmp", "qga": "qga.sock", "main": "main.sock"}
    socket_name = sockets.get(resource)
    if socket_name is None:
        raise ColleiError(f"unsupported monitor: {resource}")
    context.runner.exec(
        ["socat", "-,echo=0,icanon=0", f"unix-connect:{monitor_dir / socket_name}"]
    )


def action_perf_qemu(context: ActionContext, args: Sequence[str]) -> None:
    del args
    context.runner.exec(
        [
            "sudo",
            "perf",
            "record",
            "-g",
            "-p",
            str(context.vm.pid),
            "--",
            "sleep",
            "10",
        ]
    )


def action_gdb(context: ActionContext, args: Sequence[str]) -> None:
    del args
    status = Path(f"/proc/{context.vm.pid}/status").read_text().splitlines()
    uid_line = next(line for line in status if line.startswith("Uid:"))
    process_uid = int(uid_line.split()[1])
    argv = ["gdb", "-ex", "handle SIGUSR1 nostop noprint", "-p", str(context.vm.pid)]
    if process_uid != os.getuid():
        argv.insert(0, "sudo")
    context.runner.exec(argv, cwd=context.collei.repo.parent.parent / "qemu")


def action_hotplug_usb(context: ActionContext, args: Sequence[str]) -> None:
    del context, args
    print("TODO")


def action_trace(context: ActionContext, args: Sequence[str]) -> None:
    del args
    events = context.runner.run(
        ["qemu-system-x86_64", "-trace", "help"], capture=True
    ).stdout
    event = _fzf(events)
    socket = context.vm.directory / context.vm.which_qemu / "hmp"
    print(hmp_command(socket, f"info trace-events {event}"), end="")
    hmp_command(socket, f"trace-event {event} on")
    print(hmp_command(socket, f"info trace-events {event}"), end="")


def _qmp(context: ActionContext) -> QmpClient:
    return QmpClient(context.vm.directory / context.vm.which_qemu / "qmp-no-pretty")


SNAPSHOT_NODE = "boot1"
SNAPSHOT_DEFAULT_TAG = "default"


def _snapshot_tag(action: str, args: Sequence[str]) -> str:
    if len(args) > 1:
        raise ColleiError(f"usage: {action} [TAG]")
    return args[0] if args else SNAPSHOT_DEFAULT_TAG


def _snapshot_qmp_path(context: ActionContext) -> Path:
    return context.vm.directory / context.vm.which_qemu / "qmp-no-pretty"


def _validate_snapshot_vm(context: ActionContext) -> None:
    if context.vm.config.options.get("nvme") is not None:
        raise ColleiError(
            "snapshot actions do not support opt/nvme: QEMU aborts while "
            "restoring NVMe aer_reqs"
        )


def _ensure_snapshot_node(context: ActionContext) -> None:
    with QmpClient(_snapshot_qmp_path(context), timeout=600) as qmp:
        nodes = qmp.execute("query-named-block-nodes", {"flat": True})
    if not isinstance(nodes, list):
        raise ColleiError("query-named-block-nodes returned invalid data")
    if not any(
        isinstance(node, dict) and node.get("node-name") == SNAPSHOT_NODE
        for node in nodes
    ):
        raise ColleiError(f"snapshot block node {SNAPSHOT_NODE!r} does not exist")


def _run_snapshot_job(context: ActionContext, command: str, tag: str) -> None:
    job_id = f"{command}-{uuid.uuid4().hex}"
    arguments = {
        "job-id": job_id,
        "tag": tag,
        "vmstate": SNAPSHOT_NODE,
        "devices": [SNAPSHOT_NODE],
    }
    if context.runner.dry_run:
        print(json.dumps({"execute": command, "arguments": arguments}))
        return

    deadline = time.monotonic() + 600
    qmp_path = _snapshot_qmp_path(context)
    with QmpClient(qmp_path, timeout=600) as qmp:
        qmp.execute(command, arguments)

    last_connection_error: OSError | ColleiError | None = None
    while time.monotonic() < deadline:
        try:
            with QmpClient(qmp_path, timeout=600) as qmp:
                jobs = qmp.execute("query-jobs")
        except (OSError, ColleiError) as error:
            # Loading VMState can temporarily make the monitor unavailable.
            # Reconnect while QEMU is still alive, but report a crash promptly.
            if not context.collei.vm(context.vm.config.name).active:
                raise ColleiError(
                    f"{command} failed because QEMU exited; check the VM log"
                ) from error
            last_connection_error = error
            time.sleep(0.1)
            continue
        last_connection_error = None
        if not isinstance(jobs, list):
            raise ColleiError("query-jobs returned invalid data")
        job = next(
            (
                item
                for item in jobs
                if isinstance(item, dict) and item.get("id") == job_id
            ),
            None,
        )
        if job is None:
            raise ColleiError(f"snapshot job disappeared: {job_id}")
        if job.get("status") == "concluded":
            error = job.get("error")
            with QmpClient(qmp_path) as qmp:
                qmp.execute("job-dismiss", {"id": job_id})
            if error is not None:
                raise ColleiError(f"{command} failed: {error}")
            result = "saved" if command == "snapshot-save" else "loaded"
            print(f"snapshot {tag!r} {result}")
            return
        time.sleep(0.1)
    detail = f": {last_connection_error}" if last_connection_error else ""
    raise ColleiError(f"{command} timed out after 600 seconds{detail}")


def action_snapshot_save(context: ActionContext, args: Sequence[str]) -> None:
    tag = _snapshot_tag("snapshot-save", args)
    _validate_snapshot_vm(context)
    _ensure_snapshot_node(context)
    _run_snapshot_job(context, "snapshot-save", tag)


def action_snapshot_load(context: ActionContext, args: Sequence[str]) -> None:
    tag = _snapshot_tag("snapshot-load", args)
    _validate_snapshot_vm(context)
    _ensure_snapshot_node(context)
    _run_snapshot_job(context, "snapshot-load", tag)


def action_hotplug_cpu(context: ActionContext, args: Sequence[str]) -> None:
    del args
    with _qmp(context) as qmp:
        cpus = qmp.execute("query-hotpluggable-cpus")
    if not isinstance(cpus, list):
        raise ColleiError("query-hotpluggable-cpus returned invalid data")
    add_all = _confirm(context, "hotplug all cpu?")
    for cpu in cpus:
        if not isinstance(cpu, dict) or "qom-path" in cpu:
            continue
        props = cpu.get("props")
        if not isinstance(props, dict):
            continue
        socket_id = int(props.get("socket-id", 0))
        core_id = int(props.get("core-id", 0))
        thread_id = int(props.get("thread-id", 0))
        arguments = {
            "driver": f"host-{platform.machine()}-cpu",
            "id": f"CPU{socket_id}_{core_id}_{thread_id}",
            "socket-id": socket_id,
            "core-id": core_id,
            "thread-id": thread_id,
        }
        with _qmp(context) as qmp:
            qmp.execute("device_add", arguments)
        if not add_all:
            break


def action_hotplug_mem(context: ActionContext, args: Sequence[str]) -> None:
    del args
    counter_file = context.vm.directory / context.vm.which_qemu / "hp_mm_counter"
    counter = int(counter_file.read_text())
    counter_file.write_text(f"{counter + 1}\n")
    socket = context.vm.directory / context.vm.which_qemu / "hmp"
    hmp_command(socket, f"object_add memory-backend-memfd,id=hp_mem{counter},size=10G")
    time.sleep(1)
    hmp_command(
        socket, f"device_add pc-dimm,id=hp_dimm{counter},memdev=hp_mem{counter}"
    )


def action_throttle(context: ActionContext, args: Sequence[str]) -> None:
    del args
    arguments = {
        "device": "virtio_blk_1",
        "iops": 100,
        "iops_rd": 0,
        "iops_wr": 0,
        "bps": 0,
        "bps_rd": 0,
        "bps_wr": 0,
        "iops_max": 2000,
        "iops_max_length": 60,
    }
    with _qmp(context) as qmp:
        qmp.execute("block_set_io_throttle", arguments)


def _balloon_actual(context: ActionContext) -> int:
    output = hmp_command(
        context.vm.directory / context.vm.which_qemu / "hmp", "info balloon"
    )
    match = re.search(r"actual=([0-9]+)", output)
    if not match:
        raise ColleiError("cannot parse balloon actual size")
    return int(match.group(1))


def _balloon_set(context: ActionContext, size: int) -> None:
    hmp_command(context.vm.directory / context.vm.which_qemu / "hmp", f"balloon {size}")


def action_balloon(context: ActionContext, args: Sequence[str]) -> None:
    del args
    with _qmp(context) as qmp:
        stats = qmp.execute(
            "qom-get",
            {"path": "/machine/peripheral/balloon0", "property": "guest-stats"},
        )
    if not isinstance(stats, dict):
        raise ColleiError("balloon guest-stats returned invalid data")
    available = int(stats["stat-available-memory"]) // 1024 // 1024
    total = int(stats["stat-total-memory"]) // 1024 // 1024
    print(f"available memory {available}")
    actual = _balloon_actual(context)
    target = actual - available + 500
    print(f"{actual}M -> {target}M")
    if not _confirm(context, "do it ?"):
        return
    _balloon_set(context, target)
    time.sleep(10)
    _balloon_set(context, total)


def action_add_iso(context: ActionContext, args: Sequence[str]) -> None:
    del args
    iso_root = Path(context.collei.global_config.directory.require("iso"))
    choices = "\n".join(str(path) for path in sorted(iso_root.glob("*.iso"))) + "\n"
    selected = Path(_fzf(choices))
    print(selected)
    with (context.vm.config.options.path / "iso").open("a") as option:
        option.write(f"{selected.name}\n")


def action_addr(context: ActionContext, args: Sequence[str]) -> None:
    del args
    kernel_dir = Path(context.vm.config.options.require("kernel"))
    address = "virtqueue_get_buf_ctx_split+0x63"
    script = kernel_dir / "scripts" / "faddr2line"
    content = script.read_text()
    if not content.startswith("#!"):
        script.write_text(f"#!/usr/bin/env bash\n{content}")
    print(f"{script} {kernel_dir / 'vmlinux'} {address}")
    print(address)
    context.runner.exec([script, kernel_dir / "vmlinux", address])


def _scp_to_guest(context: ActionContext, source: Path) -> None:
    user, ip, port = context.ssh_info()
    argv = ["scp"]
    if port is not None:
        argv.extend(["-P", str(port)])
    argv.extend([str(source), f"{user}@{ip}:"])
    print(" ".join(argv))
    context.runner.run(argv)


def action_bash_prompt(context: ActionContext, args: Sequence[str]) -> None:
    del args
    prompt = (
        f'PS1="\\[\\033[01;32m\\]\\u@{context.vm.config.name}'
        '\\[\\033[00m\\]:\\[\\033[01;34m\\]\\w\\[\\033[00m\\]\\$ "'
    )
    command = f"echo '{prompt}' >> ~/.bashrc"
    print(command)
    user, ip, port = context.ssh_info()
    argv = ["ssh"]
    if port is not None:
        argv.extend(["-p", str(port)])
    argv.extend([f"{user}@{ip}", command])
    context.runner.run(argv)


def action_kexec(context: ActionContext, args: Sequence[str]) -> None:
    del args
    initrd = context.vm.directory / "initrd"
    kernel = context.vm.directory / "kernel"
    if not initrd.is_file() or not kernel.is_file():
        raise ColleiError("kexec need initrd and kernel")
    _scp_to_guest(context, kernel)
    _scp_to_guest(context, initrd)
    print(" sudo kexec -l kernel --initrd=initrd --reuse-cmdline")
    print(" sudo kexec e")


def action_vmlinux(context: ActionContext, args: Sequence[str]) -> None:
    del args
    kernel = context.vm.config.options.get("kernel")
    source = Path(kernel) / "vmlinux" if kernel else context.vm.directory / "vmlinux"
    if not source.is_file():
        raise ColleiError(f"{source} not found")
    _scp_to_guest(context, source)
    print("sudo crash vmlinux")
    print("drgn -s HOME/vmlinux --debug-directory /lib/modules/6.14.2/")


def action_pty(context: ActionContext, args: Sequence[str]) -> None:
    del args
    output = hmp_command(
        context.vm.directory / context.vm.which_qemu / "hmp", "info chardev"
    )
    matches = re.findall(r"/dev/pts/[0-9]+", output)
    if not matches:
        raise ColleiError("no QEMU PTY found")
    context.runner.exec(["screen", matches[-1], "115200"])


def action_vlan(context: ActionContext, args: Sequence[str]) -> None:
    del args
    guest_id = context.vm.config.guest_id
    commands = (
        ["sudo", "ovs-vsctl", "set", "port", f"vif_s_{guest_id}_1", "tag=[]"],
        ["sudo", "ovs-vsctl", "set", "port", f"vif_s_{guest_id}_2", "tag=[]"],
        ["sudo", "ovs-vsctl", "set", "port", f"vif_s_{guest_id}_2", "trunks=[]"],
    )
    for command in commands:
        context.runner.run(command)


def _hmp_sequence(context: ActionContext, commands: Sequence[str]) -> None:
    socket = context.vm.directory / context.vm.which_qemu / "hmp"
    for command in commands:
        print(hmp_command(socket, command), end="")


def action_cpr_exec(context: ActionContext, args: Sequence[str]) -> None:
    del args
    from qemu import QemuCommand

    vm_dir = context.vm.directory
    command = QemuCommand.from_script(vm_dir / "cmd.sh")
    cpr_command = QemuCommand(
        (*command.argv, "-incoming", f"file:{vm_dir / 'cpr_vmstate.img'}")
    )
    cpr_command.write_script(vm_dir / "cmd-cpr.sh")
    _hmp_sequence(
        context,
        (
            "info status",
            "migrate_set_parameter mode cpr-exec",
            f"migrate_set_parameter cpr-exec-command {vm_dir / 'cmd-cpr.sh'}",
            f"migrate -d file:{vm_dir / 'cpr_vmstate.img'}",
            "info status",
        ),
    )


def action_save_vm_cpr(context: ActionContext, args: Sequence[str]) -> None:
    del args
    if len(context.vm.live_pids) != 1:
        raise ColleiError("need exactly one qemu")
    state = context.vm.directory / "cpr_vmstate.img"
    _hmp_sequence(
        context,
        (
            "info status",
            "migrate_set_parameter mode cpr-reboot",
            "migrate_set_capability x-ignore-shared on",
            f"migrate -d file:{state}",
            "info status",
        ),
    )
    while True:
        with _qmp(context) as qmp:
            status = qmp.execute("query-status")
        if isinstance(status, dict) and status.get("status") == "postmigrate":
            return
        time.sleep(1)


def action_load_vm_cpr(context: ActionContext, args: Sequence[str]) -> None:
    del args
    state = context.vm.directory / "cpr_vmstate.img"
    _hmp_sequence(
        context,
        (
            "info status",
            "migrate_set_parameter mode cpr-reboot",
            "migrate_set_capability x-ignore-shared on",
            f"migrate_incoming file:{state}",
            "info status",
        ),
    )


def action_unplug_nic(context: ActionContext, args: Sequence[str]) -> None:
    del context, args


def action_add_boot_disk(context: ActionContext, args: Sequence[str]) -> None:
    del args
    image_dir = context.vm.image_directory
    numbers = [
        int(path.name.removeprefix("boot"))
        for path in image_dir.glob("boot*[0-9]")
        if path.name.removeprefix("boot").isdigit()
    ]
    number = max(numbers, default=0) + 1
    image = image_dir / f"boot{number}"
    context.runner.run(["qemu-img", "create", "-f", "qcow2", image, "350G"])
    with (context.vm.config.options.path / "disk").open("a") as option:
        option.write(f"boot{number} virtio-blk\n")


def action_hotplug_disk(context: ActionContext, args: Sequence[str]) -> None:
    del args
    counter_file = context.vm.directory / context.vm.which_qemu / "hp_disk_counter"
    counter = int(counter_file.read_text())
    counter_file.write_text(f"{counter + 1}\n")
    image = context.vm.image_directory / f"hotplug{counter}"
    if not image.is_file():
        context.runner.run(["qemu-img", "create", "-f", "qcow2", image, "400G"])
    drive_id = f"hp_drive{counter}"
    disk_id = f"hp_disk{counter}"
    socket = context.vm.directory / context.vm.which_qemu / "hmp"
    hmp_command(socket, f"drive_add 0 if=none,file={image},format=qcow2,id={drive_id}")
    time.sleep(1)
    hmp_command(
        socket,
        f"device_add scsi-hd,bus=scsi4.0,channel=0,scsi-id={counter},lun=0,drive={drive_id},id={disk_id}",
    )


def _unbind_from_vfio(context: ActionContext, bdf: str) -> None:
    device = Path("/sys/bus/pci/devices") / bdf
    if not device.is_dir():
        raise ColleiError(f"PCI device not found: {bdf}")
    vendor = (device / "vendor").read_text().strip().removeprefix("0x")
    product = (device / "device").read_text().strip().removeprefix("0x")
    vendor_device = f"{vendor} {product}\n"
    context.runner.run(
        ["sudo", "tee", "/sys/bus/pci/drivers/vfio-pci/remove_id"],
        check=False,
        input_text=vendor_device,
    )
    driver = device / "driver"
    if driver.is_symlink():
        context.runner.run(["sudo", "tee", driver / "unbind"], input_text=f"{bdf}\n")
    context.runner.run(
        ["sudo", "tee", "/sys/bus/pci/drivers_probe"], input_text=f"{bdf}\n"
    )


def action_unplug_vfio(context: ActionContext, args: Sequence[str]) -> None:
    del args
    devices = context.vm.config.options.get("vfio")
    if not devices:
        raise ColleiError("vfio is not configured")
    for device in devices.splitlines():
        context.runner.run(["lspci", "-s", device])
        _unbind_from_vfio(context, device)


def action_unplug_sriov(context: ActionContext, args: Sequence[str]) -> None:
    del args
    interfaces = context.vm.config.options.get("sriov")
    if not interfaces:
        raise ColleiError("sriov is not configured")
    for interface in interfaces.splitlines():
        context.runner.run(["ip", "link", "show", interface])
        for vf in sorted(
            (Path("/sys/class/net") / interface / "device").glob("virtfn*")
        ):
            if not vf.is_symlink():
                continue
            target = vf.resolve()
            if not (target / "iommu_group").is_symlink():
                continue
            print(target.name)
            _unbind_from_vfio(context, target.name)
        print("after unbind")
        for link in sorted(Path("/sys/class/net").glob(f"*{interface}v*")):
            print(link)


def _choose_new_vm_name(context: ActionContext) -> str:
    draft = Path("/tmp/martins3/vm_name")
    draft.parent.mkdir(parents=True, exist_ok=True)
    draft.write_text(f"{context.vm.config.name}\n")
    while True:
        completed = subprocess.run(["nvim", draft], check=False)
        if completed.returncode:
            raise ColleiError("nvim failed")
        name = draft.read_text().splitlines()[0].split()[0]
        target = context.collei.global_config.vm_root / name
        if name and not target.exists():
            return name
        if not _confirm(context, "continue ?"):
            raise ColleiError("cancelled")


def action_clone_vm(context: ActionContext, args: Sequence[str]) -> None:
    del args
    action_clone_vm_auto(context, [_choose_new_vm_name(context)])


def action_rename(context: ActionContext, args: Sequence[str]) -> None:
    del args
    target = context.collei.global_config.vm_root / _choose_new_vm_name(context)
    shutil.move(context.vm.directory, target)
    renamed_vm = context.collei.vm(target.name)
    context.collei.set_default(renamed_vm)


def _guest_vmlinux(context: ActionContext) -> Path | None:
    kernel = context.vm.config.options.get("kernel")
    candidate = Path(kernel) / "vmlinux" if kernel else context.vm.directory / "vmlinux"
    return candidate if candidate.is_file() else None


def action_dump_and_crash(context: ActionContext, args: Sequence[str]) -> None:
    del args
    dump = context.vm.directory / context.vm.which_qemu / "dump"
    vmlinux = _guest_vmlinux(context)
    if vmlinux is None:
        raise ColleiError("guest vmlinux not found")
    if dump.is_dir():
        shutil.rmtree(dump)
    else:
        dump.unlink(missing_ok=True)
    with _qmp(context) as qmp:
        qmp.execute(
            "dump-guest-memory",
            {
                "detach": True,
                "paging": False,
                "protocol": f"file:{dump}",
                "format": "kdump-raw-zlib",
            },
        )
    while True:
        with _qmp(context) as qmp:
            status = qmp.execute("query-dump")
        if isinstance(status, dict) and status.get("status") == "completed":
            break
        time.sleep(1)
    if not dump.is_file():
        raise ColleiError("dump failed ?")
    context.runner.exec(["crash", dump, vmlinux])


def action_perf_guest(context: ActionContext, args: Sequence[str]) -> None:
    del args
    vmlinux = _guest_vmlinux(context)
    if vmlinux is not None:
        common = ["sudo", "perf", "kvm", "--guest", f"--guestvmlinux={vmlinux}"]
    else:
        kallsyms = context.vm.directory / "kallsyms"
        modules = context.vm.directory / "modules"
        if not kallsyms.is_file():
            raise ColleiError("guest kallsyms not found")
        common = [
            "sudo",
            "perf",
            "kvm",
            "--guest",
            f"--guestkallsyms={kallsyms}",
            f"--guestmodules={modules}",
        ]
    context.runner.run(
        [*common, "record", "--pid", str(context.vm.pid), "--", "sleep", "3"]
    )
    context.runner.exec([*common, "report"])


def _wait_postmigrate(context: ActionContext) -> None:
    while True:
        with _qmp(context) as qmp:
            status = qmp.execute("query-status")
        if isinstance(status, dict) and status.get("status") == "postmigrate":
            return
        time.sleep(1)


def action_migrate_to_file(context: ActionContext, args: Sequence[str]) -> None:
    del args
    if len(context.vm.live_pids) != 1:
        raise ColleiError("need exactly one qemu")
    image = context.vm.directory / "vmstate.img"
    _hmp_sequence(
        # 基于 userfault 的虚拟机镜像功能
        # <!-- 1a68c5b4-cab0-4c40-86a0-85ad53299d58 -->
        #
        # 立刻保存 vmstate ，然后对于内存进行 write protect
        # 然后开启一个 background 来把内存全部都写入盘中。
        # 如果一个 page 还没写回，遇到了 write page fault ，
        # 那么就 stall 住，让这个页面立刻写回。写会之后，页面保护接触。
        #
        # 这个功能需要开启 userfault 来解决权限问题
        # echo "vm.unprivileged_userfaultfd=1" | sudo tee /etc/sysctl.d/99-unprivileged-userfaultfd.conf
        # sudo sysctl -p /etc/sysctl.d/99-unprivileged-userfaultfd.conf
        #
        # 1. mapped-ram 是仅仅能用于文件，否则存在如下报错
        #   error: migrate_incoming unix:/home/martins3/data/hack/vm/virtme/migrate.sock: Migration requires seekable transport (e.g. file)
        #   具体参考 transport_supports_seeking
        context,
        (
            "migrate_set_capability mapped-ram on",
            # "migrate_set_capability background-snapshot on",
            # "migrate_set_capability multifd on",
            "info migrate_capabilities",
            "info migrate_parameters",
            f"migrate -d file:{image}",
        ),
    )
    _wait_postmigrate(context)


def action_hotplug_nic(context: ActionContext, args: Sequence[str]) -> None:
    del args
    tap, mac = prepare_ovs_tap(context.collei, context.vm, context.runner)
    socket = context.vm.directory / context.vm.which_qemu / "hmp"
    hmp_command(
        socket, f"netdev_add tap,ifname={tap},id={tap},script=no,downscript=no,vhost=on"
    )
    hmp_command(socket, f"device_add virtio-net,netdev={tap},mac={mac},bus=root_port_1")


def _sendkeys(context: ActionContext, command: str) -> None:
    socket = context.vm.directory / context.vm.which_qemu / "hmp"
    try:
        translated = subprocess.Popen(
            ["awk", "-f", context.collei.repo / "sendkeys.awk"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
    except OSError as error:
        raise ColleiError(f"cannot start sendkeys.awk: {error}") from error

    assert translated.stdin is not None
    assert translated.stdout is not None
    assert translated.stderr is not None
    try:
        translated.stdin.write(f"{command}\n")
        translated.stdin.close()
        hmp_commands(socket, (line.rstrip("\n") for line in translated.stdout))
        stderr = translated.stderr.read()
        if translated.wait() != 0:
            raise ColleiError(f"sendkeys.awk failed: {stderr.strip()}")
    except BaseException:
        if translated.poll() is None:
            translated.kill()
        translated.wait()
        raise
    finally:
        translated.stdout.close()
        translated.stderr.close()


def action_setup_nmcli(context: ActionContext, args: Sequence[str]) -> None:
    del args
    level = context.collei.global_config.directory.integer("level", 0)
    guest = context.vm.config.guest_id
    mac = f"52:54:00:{level:02x}:{guest:02x}:00"
    print(
        f'sudo nmcli connection add type ethernet con-name vhost ifname "*" mac {mac} ip4 10.0.{guest}.0/16'
    )
    _sendkeys(
        context,
        f'sudo nmcli connection add type ethernet con-name vhost ifname "*" mac {mac} ip4 10.0.{guest}.0/16',
    )
    hmp_command(context.vm.directory / context.vm.which_qemu / "hmp", "sendkey ret")


def action_auto_install(context: ActionContext, args: Sequence[str]) -> None:
    del args
    if choose(("net", "coreos", "6", "5"), prompt="Install mode") != "coreos":
        return
    _sendkeys(
        context,
        "sudo coreos-installer install /dev/sda --ignition-file /sys/firmware/qemu_fw_cfg/by_key/46/raw",
    )
    time.sleep(30)
    socket = context.vm.directory / context.vm.which_qemu / "hmp"
    hmp_command(socket, "sendkey ret")
    _sendkeys(context, "sudo shutdown now")
    time.sleep(1)
    hmp_command(socket, "sendkey ret")
    (context.vm.config.options.path / "install").write_text("\n")
    (context.vm.config.options.path / "user").write_text("martins3\n")
    print("finished")


def _choose_migrate_host(context: ActionContext) -> str:
    if not context.vm.config.options.enabled("nbd"):
        return "127.0.0.1"
    host = _fzf("10.0.0.2\n10.0.0.5\n10.0.0.6\n127.0.0.1\n")
    addresses = context.runner.run(
        ["ip", "-4", "-o", "addr", "show"], capture=True
    ).stdout
    local = {
        field.split("/")[0]
        for line in addresses.splitlines()
        for field in line.split()
        if "/" in field and field[0].isdigit()
    }
    if host in local:
        raise ColleiError("migrate to local ?")
    return host


# 这个到底什么用来着?
# "migrate_set_capability x-ignore-shared on",
def action_migrate_cpr(context: ActionContext, args: Sequence[str]) -> None:
    del args
    if not _confirm(context, "rk -T"):
        return
    source = (context.vm.directory / "migrate_source").read_text().strip()
    path = context.vm.directory / source / "qmp-no-pretty"
    with QmpClient(path) as qmp:
        qmp.execute("migrate-set-parameters", {"mode": "cpr-transfer"})
        qmp.execute(
            "migrate",
            {
                "channels": [
                    {
                        "channel-type": "main",
                        "addr": {
                            "transport": "socket",
                            "type": "inet",
                            "host": "0",
                            "port": "44444",
                        },
                    },
                    {
                        "channel-type": "cpr",
                        "addr": {
                            "transport": "socket",
                            "type": "unix",
                            "path": "/tmp/cpr.sock",
                        },
                    },
                ]
            },
        )


def action_kvm_dmesg(context: ActionContext, args: Sequence[str]) -> None:
    del args
    source = Path.home() / "data" / "kvm-dmesg"
    if not source.is_dir():
        context.runner.run(
            ["git", "clone", "https://github.com/rayylee/kvm-dmesg", source]
        )
        context.runner.run(["make", f"-j{os.cpu_count() or 1}"], cwd=source)
    kernel = context.vm.config.options.get("kernel")
    system_map = (
        Path(kernel) / "System.map" if kernel else context.vm.directory / "System.map"
    )
    if not system_map.is_file():
        raise ColleiError(f"{system_map} not found")
    context.runner.exec(
        [
            source / "kvm-dmesg",
            context.vm.directory / context.vm.which_qemu / "qmp-no-pretty",
            system_map,
        ]
    )


def action_debug_kernel(context: ActionContext, args: Sequence[str]) -> None:
    del args
    if context.vm.active:
        if not _confirm(context, "Kill the machine?"):
            print("Give up")
            return
        os.kill(context.vm.pid, signal.SIGTERM)
        deadline = time.monotonic() + 5
        while Path(f"/proc/{context.vm.pid}").exists() and time.monotonic() < deadline:
            time.sleep(0.05)
        if Path(f"/proc/{context.vm.pid}").exists():
            raise ColleiError(f"QEMU process {context.vm.pid} did not exit")

    gdb_socket = context.vm.directory / "gdb.socket"
    gdb_socket.unlink(missing_ok=True)
    handle = add_background_task(
        context.collei,
        context.runner,
        [context.collei.scripts / "collei.py", "-s", "--foreground"],
        vm=context.vm,
        record=True,
    )

    kernel = Path(context.vm.config.options.require("kernel"))
    vmlinux = context.vm.directory / "vmlinux"
    if not vmlinux.is_file():
        vmlinux = kernel / "vmlinux"

    deadline = time.monotonic() + 10
    ready_since: float | None = None
    refreshed = context.collei.vm(context.vm.config.name)
    while True:
        now = time.monotonic()
        if refreshed.active and gdb_socket.is_socket():
            if ready_since is None:
                ready_since = now
            elif now - ready_since >= 0.5:
                break
        else:
            ready_since = None
        if time.monotonic() >= deadline:
            raise ColleiError(
                f"QEMU did not create {gdb_socket}; check {handle.backend} task {handle.identifier}"
            )
        time.sleep(0.05)
        refreshed = context.collei.vm(context.vm.config.name)

    context.runner.exec(
        [
            "gdb",
            vmlinux,
            "-ex",
            f"target remote {gdb_socket}",
            "-ex",
            "hbreak start_kernel",
            "-ex",
            "hbreak __crash_kexec",
            "-ex",
            "continue",
        ],
        cwd=kernel,
    )


def _wait_local_migration(
    source_qmp: Path, target_qmp: Path, timeout: float = 300
) -> None:
    """等待本地热迁移完成，并同时校验 source 和 target 的状态。"""
    deadline = time.monotonic() + timeout
    snapshots: dict[str, tuple[dict[str, object], dict[str, object]]] = {}
    while time.monotonic() < deadline:
        snapshots.clear()
        for side, qmp_path in (("source", source_qmp), ("target", target_qmp)):
            with QmpClient(qmp_path) as qmp:
                run_state = qmp.execute("query-status")
                migration_state = qmp.execute("query-migrate")
            if not isinstance(run_state, dict) or not isinstance(migration_state, dict):
                raise ColleiError(
                    f"{side} returned invalid migration state: "
                    f"run={run_state!r}, migration={migration_state!r}"
                )
            snapshots[side] = (run_state, migration_state)

        print(
            "migration status:\n"
            + json.dumps(
                {
                    side: {"run": run, "migration": migration}
                    for side, (run, migration) in snapshots.items()
                },
                indent=2,
            ),
            flush=True,
        )

        # 只等待 source 进入 postmigrate 会漏掉 target 加载设备状态失败；
        # 因此任何一端报告 failed 都应立即返回真实错误，而不是一直等待。
        for side, (_, migration) in snapshots.items():
            if migration.get("status") == "failed":
                error = migration.get("error-desc", "unknown migration error")
                raise ColleiError(f"{side} migration failed: {error}")

        source_status, _ = snapshots["source"]
        target_status, _ = snapshots["target"]
        source_done = source_status.get("status") == "postmigrate"
        target_running = target_status.get("status") == "running"
        if source_done and target_running:
            break
        time.sleep(1)
    else:
        raise ColleiError(
            f"migration timed out after {timeout:g} seconds: "
            f"source={snapshots.get('source')!r}, "
            f"target={snapshots.get('target')!r}"
        )


def _wait_migration_status(
    source_qmp: Path, expected: str, timeout: float = 300.0
) -> None:
    deadline = time.monotonic() + timeout
    last: dict[str, object] | None = None
    while time.monotonic() < deadline:
        with QmpClient(source_qmp) as qmp:
            migration = qmp.execute("query-migrate")
        if not isinstance(migration, dict):
            raise ColleiError(f"invalid migration state: {migration!r}")
        last = migration
        status = migration.get("status")
        if status == expected:
            return
        if status in {"cancelled", "failed"}:
            error = migration.get("error-desc", "unknown migration error")
            raise ColleiError(f"migration failed before {expected}: {error}")
        time.sleep(0.1)
    raise ColleiError(
        f"migration did not reach {expected!r} after {timeout:g} seconds: {last!r}"
    )


def _run_local_migration(
    context: ActionContext,
    before_migrate: Sequence[str],
    after_migrate: Sequence[str] = (),
    *,
    interactive: bool = True,
    pre_switchover: Callable[[], None] | None = None,
) -> None:
    vm_dir = context.vm.directory
    source = (vm_dir / "migrate_source").read_text().strip()
    target = (vm_dir / "migrate_target").read_text().strip()
    target_socket = vm_dir / target / "hmp"
    source_socket = vm_dir / source / "hmp"

    # 给 target 注册命令
    for command in (
        *before_migrate,
        f"migrate_incoming unix:{vm_dir / 'migrate.sock'}",
    ):
        _migration_hmp_command(target_socket, command)

    # 给 source 注册命令
    for command in (
        *before_migrate,
        "info migrate_capabilities",
        "info migrate_parameters",
        f"migrate -d unix:{vm_dir / 'migrate.sock'}",
        *after_migrate,
    ):
        _migration_hmp_command(source_socket, command)

    if pre_switchover is not None:
        source_qmp = vm_dir / source / "qmp-no-pretty"
        _wait_migration_status(source_qmp, "pre-switchover")
        pre_switchover()
        with QmpClient(source_qmp) as qmp:
            qmp.execute("migrate-continue", {"state": "pre-switchover"})

    _wait_local_migration(
        vm_dir / source / "qmp-no-pretty",
        vm_dir / target / "qmp-no-pretty",
    )

    # 进入到 target 端的 qhm 环境中
    print(f"socat -,echo=0,icanon=0 unix-connect:{source_socket}")
    # 当 ai 执行的时候，非交互环境执行会失败
    if not interactive or not sys.stdin.isatty():
        return
    context.runner.exec(["socat", "-,echo=0,icanon=0", f"unix-connect:{source_socket}"])


def _migration_hmp_command(socket: Path, command: str) -> None:
    output = hmp_command(socket, command)
    print(output, end="", flush=True)
    match = re.search(r"\bError:\s*([^\r\n]+)", output)
    if match is not None:
        raise ColleiError(f"{command}: {match.group(1)}")


def action_migrate(context: ActionContext, args: Sequence[str]) -> None:
    del args
    if not _confirm(context, "rk -a"):
        return
    _run_local_migration(
        context,
        (
            "migrate_set_capability multifd on",
            "migrate_set_parameter multifd-channels 4",
            "migrate_set_parameter multifd-compression zstd",
            # 可以利用 max-bandwidth 来限速，其单位是 MB/s
            # 默认 128M ，所以需要打开设置
            "migrate_set_parameter max-bandwidth 0",
            # 普通的热迁移也可以使用 background-snapshot
            # 1. 存在兼容性问题 Error: Background-snapshot is not compatible with multifd
            # 2. 本地热迁移会有 block lock 问题: Is another process using the image [/home/martins3/data/hack/vm/virtme/img/virtio-scsi_1]?
            #
            # 不过，热迁移加上 background-snapshot 就很奇怪，因为从时间 T0 开始
            # 到 T1 完成热迁移到 target 端后，但是 target 端的虚拟机还是从 T0 的状态开始执行的
            # 这个也干扰了正常的热迁移:
            # "migrate_set_capability multifd off",
            # "migrate_set_capability background-snapshot on",
        ),
    )


def _migration_image_directory(vm_dir: Path, slot: str) -> Path:
    if slot == "s":
        return vm_dir / "img"
    if slot == "t":
        return vm_dir / "img-t"
    raise ColleiError(f"invalid migration slot: {slot!r}")


def _validate_nbd_migration_vm(context: ActionContext) -> None:
    unsupported = tuple(
        name
        for name in ("buildroot", "fire", "multipath", "qsd", "sriov", "vfio")
        if context.vm.config.options.enabled(name)
    )
    if unsupported:
        configured = ", ".join(f"opt/{name}" for name in unsupported)
        raise ColleiError(f"migrate_nbd does not support {configured}")
    if len(context.vm.live_pids) != 1:
        raise ColleiError("migrate_nbd requires exactly one running source QEMU")


def _wait_qmp_ready(qmp_path: Path, pid_file: Path, timeout: float = 30.0) -> None:
    deadline = time.monotonic() + timeout
    last_error: ColleiError | OSError | None = None
    while time.monotonic() < deadline:
        try:
            pid = int(pid_file.read_text().strip())
        except (FileNotFoundError, ValueError):
            pid = 0
        if pid and not Path(f"/proc/{pid}/status").is_file():
            raise ColleiError(f"target QEMU exited before creating {qmp_path}")
        try:
            with QmpClient(qmp_path) as qmp:
                status = qmp.execute("query-status")
            if isinstance(status, dict):
                return
        except (ColleiError, OSError) as error:
            last_error = error
        time.sleep(0.1)
    detail = f": {last_error}" if last_error is not None else ""
    raise ColleiError(f"target QMP was not ready after {timeout:g} seconds{detail}")


def _validate_target_disks(
    source: Sequence[MigratableDisk], target: Sequence[MigratableDisk]
) -> None:
    source_by_name = {disk.node_name: disk for disk in source}
    target_by_name = {disk.node_name: disk for disk in target}
    if source_by_name.keys() != target_by_name.keys():
        missing = sorted(source_by_name.keys() - target_by_name.keys())
        extra = sorted(target_by_name.keys() - source_by_name.keys())
        raise ColleiError(
            f"source/target block topology differs: missing={missing}, extra={extra}"
        )
    for name, source_disk in source_by_name.items():
        target_disk = target_by_name[name]
        if (
            source_disk.image_format != target_disk.image_format
            or source_disk.virtual_size != target_disk.virtual_size
        ):
            raise ColleiError(
                f"source/target block node {name} differs: "
                f"source={source_disk.image_format}/{source_disk.virtual_size}, "
                f"target={target_disk.image_format}/{target_disk.virtual_size}"
            )


def _stop_qemu_slot(vm_dir: Path, slot: str, timeout: float = 10.0) -> None:
    pid_file = vm_dir / slot / "pid"
    try:
        pid = int(pid_file.read_text().strip())
    except (FileNotFoundError, ValueError):
        return
    process = Path(f"/proc/{pid}/status")
    if not process.is_file():
        return
    os.kill(pid, signal.SIGTERM)
    deadline = time.monotonic() + timeout
    while process.is_file() and time.monotonic() < deadline:
        time.sleep(0.05)
    if process.is_file():
        raise ColleiError(f"QEMU slot {slot} process {pid} did not exit")


def _resume_source_after_failed_migration(source_qmp: Path) -> None:
    try:
        with QmpClient(source_qmp) as qmp:
            migration = qmp.execute("query-migrate")
            if isinstance(migration, dict) and migration.get("status") in {
                "active",
                "device",
                "pre-switchover",
                "setup",
            }:
                qmp.execute("migrate_cancel")
        deadline = time.monotonic() + 10.0
        while time.monotonic() < deadline:
            with QmpClient(source_qmp) as qmp:
                migration = qmp.execute("query-migrate")
            if not isinstance(migration, dict) or migration.get("status") not in {
                "active",
                "cancelling",
                "device",
                "pre-switchover",
                "setup",
            }:
                break
            time.sleep(0.1)
        with QmpClient(source_qmp) as qmp:
            status = qmp.execute("query-status")
            if isinstance(status, dict) and status.get("running") is not True:
                qmp.execute("cont")
    except (ColleiError, OSError) as error:
        print(f"warning: could not resume source QEMU: {error}")


def action_migrate_nbd(context: ActionContext, args: Sequence[str]) -> None:
    if args:
        raise ColleiError("usage: migrate_nbd")
    _validate_nbd_migration_vm(context)
    if not _confirm(context, "migrate VM with blockdev-mirror + NBD?"):
        return

    vm_dir = context.vm.directory
    source = context.vm.which_qemu
    target = "t" if source == "s" else "s"
    source_qmp = vm_dir / source / "qmp-no-pretty"
    target_qmp = vm_dir / target / "qmp-no-pretty"
    source_images = context.vm.image_directory
    target_images = _migration_image_directory(vm_dir, target)
    disks = discover_migratable_disks(source_qmp, source_images, target_images)
    existing = [disk.target for disk in disks if disk.target.exists()]
    if existing and not _confirm(
        context,
        f"overwrite {len(existing)} image(s) in inactive storage slot {target}?",
    ):
        return
    prepare_target_images(context.collei, context.runner, disks)

    target_command = [
        context.collei.scripts / "collei.py",
        "-a",
        "--nbd-target",
        "--foreground",
    ]
    target_runtime = VmRuntime(
        context.vm.config,
        target,
        context.vm.live_pids,
        target,
    )
    if context.runner.dry_run:
        context.runner.run(target_command)
        BlockMirrorSession(
            source_qmp,
            target_qmp,
            vm_dir / target / "block-migration.nbd",
            disks,
        ).print_plan()
        return

    session = BlockMirrorSession(
        source_qmp,
        target_qmp,
        vm_dir / target / "block-migration.nbd",
        disks,
    )
    switched = False
    try:
        # 启动服务来利用 nbd 机制来同步热迁移
        add_background_task(
            context.collei,
            context.runner,
            target_command,
            vm=target_runtime,
            label="qemu-nbd-target",
            record=True,
        )
        _wait_qmp_ready(target_qmp, vm_dir / target / "pid")
        target_disks = discover_migratable_disks(
            target_qmp, target_images, source_images
        )
        _validate_target_disks(disks, target_disks)
        session.start()
        session.wait_ready()

        def finish_storage() -> None:
            session.finish()
            session.cleanup()

        # 发起 qemu 的热迁移
        _run_local_migration(
            context,
            (
                "migrate_set_capability multifd on",
                "migrate_set_capability pause-before-switchover on",
                "migrate_set_parameter multifd-channels 4",
                "migrate_set_parameter multifd-compression zstd",
            ),
            interactive=False,
            pre_switchover=finish_storage,
        )
        switched = True
        context.vm.persist_storage_slot(target)
        _stop_qemu_slot(vm_dir, source)
        print(f"migrate_nbd completed: active storage slot is {target}")
    except BaseException:
        session.cleanup(best_effort=True)
        if switched:
            context.vm.persist_storage_slot(target)
            try:
                _stop_qemu_slot(vm_dir, source)
            except ColleiError as error:
                print(f"warning: could not stop source QEMU: {error}")
        else:
            _resume_source_after_failed_migration(source_qmp)
            try:
                _stop_qemu_slot(vm_dir, target)
            except ColleiError as error:
                print(f"warning: could not stop target QEMU: {error}")
        raise


def action_migrate_postcopy(context: ActionContext, args: Sequence[str]) -> None:
    del args
    if not _confirm(context, "rk -a"):
        return
    _run_local_migration(
        context,
        (
            # Postcopy is not yet compatible with multifd
            # 默认 multifd 打开的，所以需要关闭一下
            "migrate_set_capability multifd off",
            "migrate_set_capability postcopy-ram on",
        ),
        ("migrate_start_postcopy",),
    )


def _choose_guest_level() -> int:
    print("VM level, level 0 means physical machine")
    return int(choose(("1", "2", "3"), prompt="VM level")) - 1


def action_tmp_ip(context: ActionContext, args: Sequence[str]) -> None:
    del args
    print(temporary_ip_commands(context.vm.config.guest_id, _choose_guest_level()))


def action_network(context: ActionContext, args: Sequence[str]) -> None:
    del args
    print(network_configurations(context.vm.config.guest_id, _choose_guest_level()))


def action_auto(context: ActionContext, args: Sequence[str]) -> None:
    del args
    hmp_command(
        context.vm.directory / context.vm.which_qemu / "hmp",
        "device_del boot1",
    )


ACTIONS: dict[str, Action] = {
    "add_boot_disk": Action(action_add_boot_disk),
    "add_iso": Action(action_add_iso),
    "addr": Action(action_addr),
    "auto_install": Action(action_auto_install, VmRequirement.ACTIVE),
    "auto": Action(action_auto, VmRequirement.ACTIVE),
    "backup": Action(action_backup),
    "balloon": Action(action_balloon, VmRequirement.ACTIVE),
    "bash_prompt": Action(action_bash_prompt),
    "bg": Action(action_bg),
    "cgroup": Action(action_cgroup, VmRequirement.ACTIVE),
    "clone_vm_auto": Action(action_clone_vm_auto),
    "clone_vm": Action(action_clone_vm),
    "cold_migrate": Action(None, VmRequirement.INACTIVE),
    "default": Action(action_default),
    "debug_kernel": Action(action_debug_kernel, VmRequirement.DEBUG_KERNEL),
    "dump_and_crash": Action(action_dump_and_crash, VmRequirement.ACTIVE),
    "edit": Action(action_edit),
    "follow_log": Action(action_follow_log),
    "force_reboot": Action(action_force_reboot, VmRequirement.ACTIVE),
    "gdb": Action(action_gdb, VmRequirement.ACTIVE),
    "hmp": Action(action_hmp, VmRequirement.ACTIVE),
    "hotplug_cpu": Action(action_hotplug_cpu, VmRequirement.ACTIVE),
    "hotplug_disk": Action(action_hotplug_disk, VmRequirement.ACTIVE),
    "hotplug_mem": Action(action_hotplug_mem, VmRequirement.ACTIVE),
    "hotplug_nic": Action(action_hotplug_nic, VmRequirement.ACTIVE),
    "hotplug_usb": Action(action_hotplug_usb, VmRequirement.ACTIVE),
    "unplug_disk": Action(action_unplug_disk, VmRequirement.ACTIVE),
    "unplug_nic": Action(action_unplug_nic, VmRequirement.ACTIVE),
    "unplug_sriov": Action(action_unplug_sriov, VmRequirement.ACTIVE),
    "unplug_vfio": Action(action_unplug_vfio),
    "kill": Action(action_kill, VmRequirement.ACTIVE),
    "kexec": Action(action_kexec, VmRequirement.ACTIVE),
    "kvm_dmesg": Action(action_kvm_dmesg, VmRequirement.ACTIVE),
    "log": Action(action_log),
    "monitor": Action(action_monitor, VmRequirement.ACTIVE),
    # 热迁移相关
    "migrate": Action(action_migrate, VmRequirement.ACTIVE),
    "migrate_nbd": Action(action_migrate_nbd, VmRequirement.ACTIVE),
    "migrate_postcopy": Action(action_migrate_postcopy, VmRequirement.ACTIVE),
    "migrate_cpr": Action(action_migrate_cpr, VmRequirement.ACTIVE),
    "save_vm_cpr": Action(action_save_vm_cpr, VmRequirement.ACTIVE),
    "load_vm_cpr": Action(action_load_vm_cpr, VmRequirement.ACTIVE),
    "cpr_exec": Action(action_cpr_exec, VmRequirement.ACTIVE),
    "migrate_to_file": Action(action_migrate_to_file, VmRequirement.ACTIVE),
    "snapshot-load": Action(action_snapshot_load, VmRequirement.ACTIVE),
    "snapshot-save": Action(action_snapshot_save, VmRequirement.ACTIVE),
    "path": Action(action_path),
    "perf_qemu": Action(action_perf_qemu, VmRequirement.ACTIVE),
    "perf_guest": Action(action_perf_guest, VmRequirement.ACTIVE),
    "pty": Action(action_pty, VmRequirement.ACTIVE),
    "rename": Action(action_rename, VmRequirement.INACTIVE),
    "restore": Action(action_restore),
    "rsync": Action(action_rsync),
    "run": Action(action_run, VmRequirement.LAUNCH),
    "ssh": Action(action_ssh, VmRequirement.ACTIVE),
    "ssh_auto": Action(action_ssh_auto, VmRequirement.ACTIVE),
    "ssh_vsock": Action(action_ssh_vsock, VmRequirement.ACTIVE),
    "ssh_vsock_auto": Action(action_ssh_vsock_auto, VmRequirement.ACTIVE),
    "ssh_copy_id": Action(action_ssh_copy_id, VmRequirement.ACTIVE),
    "setup_vmware": Action(action_setup_vmware),
    # 网络配置
    "setup_net_nmcli_vnc": Action(action_setup_nmcli, VmRequirement.ACTIVE),
    "setup_net_config": Action(action_network),
    "setup_net_nmcli_commands": Action(action_tmp_ip),
    "top": Action(action_top, VmRequirement.ACTIVE),
    "throttle": Action(action_throttle, VmRequirement.ACTIVE),
    "trace": Action(action_trace, VmRequirement.ACTIVE),
    "tty3": Action(action_tty3, VmRequirement.ACTIVE),
    "vlan": Action(action_vlan),
    "vmlinux": Action(action_vmlinux, VmRequirement.ACTIVE),
    "vnc": Action(action_vnc, VmRequirement.ACTIVE),
}


def effective_requirement(
    action: Action, arguments: Sequence[str] = ()
) -> VmRequirement:
    """根据 run 的启动模式确定它对当前 VM 状态的要求。"""
    if action.requirement is not VmRequirement.LAUNCH:
        return action.requirement

    try:
        options = LaunchOptions.parse(arguments)
    except ColleiHelp:
        return VmRequirement.ANY
    if options.migration in {"defer", "cpr-transfer"}:
        return VmRequirement.ACTIVE
    if options.dry_run and options.migration is None:
        return VmRequirement.ANY
    return VmRequirement.INACTIVE


def validate_requirement(
    action_name: str,
    action: Action,
    vm: VmRuntime,
    arguments: Sequence[str] = (),
) -> None:
    requirement = effective_requirement(action, arguments)
    if requirement is VmRequirement.ACTIVE and not vm.active:
        raise ColleiError(f"{action_name} need vm is active")
    if requirement is VmRequirement.INACTIVE and vm.active:
        raise ColleiError(f"{action_name} need vm is inactive")
