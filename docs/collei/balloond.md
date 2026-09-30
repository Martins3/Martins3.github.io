# 利用 virtio balloon 来节省内存

我现在我的环境一般有多个虚拟机运行，即便我的机器是 128G 内存，
同时开启 5 ~ 8 个虚拟机后， 总内存占用量很容易超过 64G ，已经开始和 Edge / 微信这些内存消耗大户来抢占内存了
解决起来也很容易，就是让所有的虚拟机都配置上 balloon

配置方法为:
```txt
-device virtio-balloon,id=balloon0,iothread=io_balloon,deflate-on-oom=true,free-page-reporting=true,free-page-hint=true
```

注意，由于虚拟机中会尽量的占用 page cache ，所以自动释放不用的内存的 free-page-reporting 其实意义不大。

## 基本规则
`collei-balloond` 会扫描正在运行的 Collei QEMU 虚拟机，并通过
virtio-balloon QMP 命令，让每台受支持虚拟机的可用内存维持在可配置的
`MemAvailable` 水位附近。它不会通过 SSH 登录虚拟机；当内存统计缺失或过期时，
会跳过对应虚拟机。

如果正在运行的 QEMU 命令行中包含 `vfio-pci` 设备，该虚拟机会自动进入
`observe-only`（仅观察）模式：守护进程仍会采集并显示它的内存统计，但绝不会修改
它的 balloon 目标。

默认策略会保留 `max(2 GiB, 20%)` 的可用内存，不会将虚拟机当前内存降低到
`max(2 GiB, 25%)` 以下。可用内存连续三个采样周期高于水位后，每 10 秒最多回收
512 MiB。

## 构建与安装

首次安装时，在本目录中执行：

```bash
cargo build --release
install -Dm755 target/release/collei-balloond ~/.local/bin/collei-balloond.out
install -Dm644 balloond.example.toml ~/.config/collei/balloond.toml
install -Dm644 systemd/collei-balloond.service ~/.config/systemd/user/collei-balloond.service
systemd-analyze --user verify ~/.config/systemd/user/collei-balloond.service
systemctl --user daemon-reload
systemctl --user enable --now collei-balloond.service
```

升级时保留现有配置，只替换可执行文件和 systemd 单元文件：

```bash
cargo build --release
systemctl --user stop collei-balloond.service
install -Dm755 target/release/collei-balloond ~/.local/bin/collei-balloond.out
install -Dm644 systemd/collei-balloond.service ~/.config/systemd/user/collei-balloond.service
systemctl --user daemon-reload
systemctl --user start collei-balloond.service
```

## 命令

```text
collei-balloond.out check-config
collei-balloond.out status [--json] [--only VM]
collei-balloond.out run [--dry-run] [--only VM]
collei-balloond.out restore [--vm VM]
```

`status` 的 `CURRENT MiB` 是 balloon 调整后 guest 可见的内存大小；
`RSS MiB` 是从宿主机 `/proc/<qemu-pid>/statm` 第二列（resident）读取的
QEMU 进程驻留页数，乘以宿主机实际页大小后向下取整为 MiB，已 swap out 的
页面不计入。`statm` 使用内核维护的计数，可能存在统计偏差。
RSS 包含 QEMU 自身开销和
映射的共享页面，不等同于 guest 内存使用量；共享页面未按比例分摊，显式
HugeTLB 页面不计入此指标。RSS 也不涵盖已驻留但未建立进程页表映射的
memfd 页面，例如 iommufd 直接固定的 guest RAM，因此不能作为 VM 全部物理
内存占用的完整统计。
JSON 输出对应字段为 `rss_mib`。进程退出、权限不足或无法确定唯一 QEMU
进程时显示 `-`（JSON 为 `null`）；guest balloon 统计不可用时仍可显示 RSS。

守护进程停止时会有意保留当前 balloon 状态。由于守护进程和 `restore` 使用
`$XDG_RUNTIME_DIR` 下的同一个排他锁，执行 `restore` 前必须先停止用户服务：

```bash
systemctl --user stop collei-balloond.service
collei-balloond.out restore
systemctl --user start collei-balloond.service
```

配置文件路径为 `~/.config/collei/balloond.toml`，可以从
`balloond.example.toml` 开始修改。每台虚拟机的独立设置使用带引号的 TOML 表，
例如 `[vms."fedora44-server"]`。

安装后的服务会把日志写入用户 journal：

```text
systemctl --user status collei-balloond.service
journalctl --user -u collei-balloond.service
```

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
