# 普通 virtserialport：字节通信与 Bash shell demo

`vport` 不属于 TTY，也不是内核 console，但可以通过用户态程序承载交互式 Bash。
本 demo 使用 Python 3.11+ 标准库，无须安装第三方 Python 包。

```text
物理机                                           虚拟机
host.py ↔ Unix socket ↔ QEMU virtserialport ↔ virtqueue ↔ /dev/vportXpY
                                                            ↕
                                                         guest.py
                                                            ↕
                                                         PTY master
                                                            ↕
                                                         PTY slave
                                                            ↕
                                                   bash --noprofile --norc -i
```

SSH 仅用于复制/启动 guest 程序；Bash 输入输出和 Ctrl+C 均走 vport。
运行时 guest 只有 loopback 网卡，本路径不依赖 guest TCP/IP 网络。

## 文件

| 文件 | 作用 |
|---|---|
| `guest.py` | guest 收发数据；shell 模式可用 PTY、普通管道或 HVC0 启动 Bash |
| `host.py` | host 交互终端；支持 vport JSON relay 或 HVC host-PTY relay |
| `wire.py` | JSON 行分帧；终端字节用 base64 编码 |

`vport` 和 Unix socket 都是字节流，不保留消息边界，所以程序自己分帧。
协议包含 shell、ready、data、exit、close；shell 的 `data` 帧装原始终端字节。
例如 Ctrl+C 是 `0x03`，guest 将它写入 PTY master，PTY 的行规程向前台进程组发送 SIGINT。
退出状态通过 `exit` 帧返回，因为 guest 关闭 vport 不等同于 QEMU 关闭 host socket。

## 复现

本次实验的 VM 和端口保留，guest 程序已复制到 `/tmp/virtio-port-demo/`。
每次运行 guest.py 处理一个会话，结束后重新启动它即可再连。

物理机工作目录：

```bash
cd /home/martins3/data/vn/docs/kernel/tty/code/virtio-port-demo
```

只有在重新启动 QEMU 或新的 VM 上才需要热插入；同一实例不要重复 attach：

```bash
python3 attach.py \
  --qmp /home/martins3/data/hack/vm/sysrq-lab-20260925/s/qmp-no-pretty \
  --socket /home/martins3/data/hack/vm/sysrq-lab-20260925/s/vport-demo.sock
```

默认总线是本机实际查询到的 `virtio-serial-bus.0`；其他 VM 可传 `--bus`。
使用当前活动 QEMU 的 `s` 或 `t` 路径。guest 须启用 `CONFIG_VIRTIO_CONSOLE`，
PCI 传输还需要 `CONFIG_VIRTIO_PCI`；数据端口不需要任何 `console=` 参数。
端口通过 QMP 热插入，仅在本次 QEMU 进程中存在，不修改 collei 配置或 cmd.sh。

guest 重启清空 `/tmp` 后，可在 host 重新复制：

```bash
ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
  -o 'ProxyCommand=/home/martins3/.nix-profile/bin/socat - VSOCK-CONNECT:1162:22' \
  root@virtme 'mkdir -p /tmp/virtio-port-demo'
scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
  -o 'ProxyCommand=/home/martins3/.nix-profile/bin/socat - VSOCK-CONNECT:1162:22' \
  guest.py wire.py root@virtme:/tmp/virtio-port-demo/
```

终端 A：在 guest 中启动服务。以 root 打开设备，但 Bash 降权到 martins3：

```bash
ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
  -o 'ProxyCommand=/home/martins3/.nix-profile/bin/socat - VSOCK-CONNECT:1162:22' \
  root@virtme 'python3 /tmp/virtio-port-demo/guest.py --user martins3'
```

终端 B：启动交互 Bash：

```bash
python3 host.py \
  --socket /home/martins3/data/hack/vm/sysrq-lab-20260925/s/vport-demo.sock
```

在 `vport-demo$` 提示符下输入 `hostname`、`tty`、`sleep 30` 等。
Ctrl+C 转发给 guest；`exit` 正常退出；Ctrl+] 断开会话。
demo 启动时传一次窗口大小，没有实现运行中的窗口缩放或登录认证；
socket 权限为 0600，shell 以 `--user` 指定的用户运行。

`guest.py` 默认使用 PTY，也可以启动不带 PTY 的 Bash：

```bash
# PTY：有 controlling terminal、行规程、终端回显和作业控制
sudo /home/martins3/data/vn/docs/kernel/tty/code/virtio-port-demo/guest.py \
  --user martins3 --mode pty

# pipe：Bash 的标准流直接连接普通管道，不创建 controlling terminal
sudo /home/martins3/data/vn/docs/kernel/tty/code/virtio-port-demo/guest.py \
  --user martins3 --mode pipe
```

两种模式都使用同一个 host 命令。PTY 模式下 `tty` 返回 `/dev/pts/*`，
`test -t 0` 成功，Ctrl+C 由终端行规程转换为 SIGINT；pipe 模式下 `tty` 报
`not a tty`，`test -t 0` 失败，输入只是普通字节，不能依靠终端生成 SIGINT。

HVC0 是另一条独立路径，不经过 `vport.sock` 或 JSON framing。当前 QEMU 的
`query-chardev` 会把 `char_pty` 报告为类似 `pty:/dev/pts/19` 的宿主机路径；
这个路径是 QEMU 的 host-side PTY，不是 guest 内的 `/dev/pts/N`。启动方式如下：

```bash
# guest：Bash 的 stdin/stdout/stderr 直接接到 /dev/hvc0
sudo /home/martins3/data/vn/docs/kernel/tty/code/virtio-port-demo/guest.py \
  --user martins3 --mode hvc

# host：把本地终端原始字节转发到 QEMU 报告的 char_pty 路径
/home/martins3/data/vn/docs/kernel/tty/code/virtio-port-demo/host.py \
  --hvc /dev/pts/19
```

HVC 模式下 `tty` 返回 `/dev/hvc0`，`test -t 0` 成功；guest 不会创建新的
`/dev/pts/N`。当前 VM 没有在 hvc0 上运行 getty；如果实际 VM 有 getty，需要先停掉它。
HVC 路径没有 vport 的 close 帧，先在 shell 中执行 `exit` 再退出 host relay；
VM 重启后 host-side PTY 编号可能改变，需要重新查询 `char_pty`。

## 需要说明的，其实 wayland 的实现就是 client 和 server 就是 unix domain socket 来沟通的

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
