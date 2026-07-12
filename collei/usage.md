## systemd 后台任务检查

启用 systemd backend：

```bash
echo systemd > ~/.config/collei/task_backend
```

普通 VM 主进程使用 user unit 名 `collei-vm-<vm-name>.service`。例如
`yyds-fs` 对应：

```bash
systemctl --user status collei-vm-yyds-fs.service
journalctl --user -u collei-vm-yyds-fs.service -o cat
journalctl --user -u collei-vm-yyds-fs.service -f -o cat
```

也可以继续使用 collei action，它会读取 VM 目录中记录的 backend 和 task id：

```bash
./collei/scripts/collei-action.py -a log -n yyds-fs
./collei/scripts/collei-action.py -a follow_log -n yyds-fs
```

辅助进程如 `novnc`、`virtiofsd`、`swtpm`、`qsd` 使用
`collei-<label>-<vm-name>-<suffix>.service` 形式的 transient unit，可用：

```bash
systemctl --user list-units 'collei-*'
journalctl --user -u 'collei-*' -o cat
```

本地热迁移会同时存在两个 QEMU。source `s` 继续使用
`collei-vm-<vm-name>.service`，target `t` 使用
`collei-vm-<vm-name>-t.service`，例如：

```bash
systemctl --user list-units 'collei-vm-yyds-collei*'
systemctl --user status collei-vm-yyds-collei.service
systemctl --user status collei-vm-yyds-collei-t.service
```

清理失败状态：

```bash
./collei/scripts/collei-global.py -a clean
```

## VFIO 设备驱动切换

`vfio.py` 可以直接查看设备驱动、绑定 `vfio-pci`，或恢复绑定前的默认驱动：

```bash
./collei/scripts/vfio.py status 0000:02:00.0
./collei/scripts/vfio.py bind 0000:02:00.0
./collei/scripts/vfio.py unbind 0000:02:00.0
./collei/scripts/vfio.py default 0000:02:00.0
```

`unbind` 只从当前驱动解绑，不会自动探测或绑定其他驱动，可用于修改 IOMMU domain type。`default` 也可写成 `restore`。绑定 VFIO 时会记录原驱动；如果没有记录，则通过 PCI modalias 解析默认驱动。必要时可以用 `default BDF --driver DRIVER` 明确指定。


<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
