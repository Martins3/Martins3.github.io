# 企业微信消息通知桥

这个通知桥把 Collei Windows VM 中的企业微信消息活动转换为 Linux GNOME 通知。
通知内容固定为“企业微信：有新消息”，不会读取或传输发送人、会话名称和消息正文。

## RDP 是否必须保持打开

不需要一直打开 Linux 上的 RDP 客户端，但 Windows 用户必须处于已登录状态。

- 关闭 `xfreerdp` 窗口通常只会断开 RDP，会话内的企业微信和通知任务继续运行。
- 在 Windows 中选择“断开连接”也可以。
- 不要在 Windows 中选择“注销”。注销会结束企业微信和通知任务。
- Windows VM 重启后，如果没有配置自动登录，需要通过 RDP 登录一次。登录后可以立即断开 RDP。
- Linux 侧必须存在已登录的 GNOME 桌面会话，否则没有可接收通知的用户 D-Bus。

可以在 Windows PowerShell 中使用下面的命令区分断开和注销：

```powershell
quser
```

看到用户会话的状态为 `Active` 或 `Disc` 均可。没有用户会话时，需要重新通过 RDP 登录。

## Windows 重启后

当前 VM 的 `AutoAdminLogon` 为 `0`，没有配置 Windows 自动登录。企业微信和
`Codex-WeCom-Notify-Bridge` 都在用户 `maritns3\97936` 登录后启动。因此每次 Windows
重启后执行下面的步骤：

1. 等待 Windows VM 启动完成。
2. 通过 RDP 登录用户 `97936` 一次。
3. 确认企业微信已经自动启动并保持登录；如果企业微信要求验证，需要完成验证。
4. 登录后可以直接关闭 `xfreerdp` 窗口或断开 RDP，不要在 Windows 中选择“注销”。

不需要手工运行两个脚本。登录时，企业微信的启动项和计划任务会分别自动启动。
如果监听器被意外终止，计划任务的每分钟看门狗会在约 1 分钟内自动拉起；正常运行时
`MultipleInstances=IgnoreNew` 会忽略重复启动，不会产生多个监听进程。

登录后可以在 Windows PowerShell 中快速确认：

```powershell
Get-Process WXWork
Get-ScheduledTask -TaskName "Codex-WeCom-Notify-Bridge"
```

企业微信应存在进程，计划任务的 `State` 应为 `Running`。

## 文件和运行位置

Linux 宿主机：

- `wecom-notify-bridge.py`：Windows 监听器的源码和部署源文件。
- `notify-from-windows.sh`：受限 SSH key 在 Linux 上唯一允许执行的通知命令。
- `~/.ssh/wecom-bridge`：Windows 通知桥专用私钥的宿主备份。
- `~/.ssh/authorized_keys`：通过 `restrict,command=...` 限制该 key 只能发送固定通知。

Windows VM：

- `C:\Users\97936\wecom-notify-bridge.py`：实际运行的监听器。
- `%LOCALAPPDATA%\WeComNotifyBridge\host_key`：通知桥专用 SSH 私钥。
- `%LOCALAPPDATA%\WeComNotifyBridge\known_hosts`：Linux 宿主机 SSH 指纹。
- `%LOCALAPPDATA%\WeComNotifyBridge\state.txt`：监听器启动状态。
- `%LOCALAPPDATA%\WeComNotifyBridge\bridge.log`：检测和发送结果日志。
- 计划任务 `Codex-WeCom-Notify-Bridge`：用户登录时启动监听器，并每分钟检查一次是否需要拉起。

## 工作方式

`wecom-notify-bridge.py` 使用 Python 标准库 `ctypes` 监听企业微信创建的提示窗口，
不需要安装第三方 Python 包。检测到新的提示窗口后，
`Send-HostNotification` 使用专用 SSH key 连接 `martins3@10.0.2.2`。Linux SSH
服务强制执行 `notify-from-windows.sh`，忽略客户端提供的其他命令；脚本再通过
GNOME 用户 D-Bus 发送固定通知。

监听器不再观察 `message.db`。数据库会在任何消息到达时变化，包括免打扰会话，
因此把数据库变化当成通知会导致免打扰消息误报。现在只有企业微信自己创建提示窗口时
才会转发；免打扰消息虽然仍然写入数据库，但不会触发宿主通知。

以下情况用于减少误报：

- 企业微信位于 Windows 前台且最近 10 秒有人操作时，不发送通知。
- 两次通知至少间隔 15 秒。
- 监听器启动时先记录现有窗口，任务重启本身不会产生通知。

## 日常使用

正常情况下不需要手工启动任何脚本：

1. 保持 Collei Windows VM 运行。
2. Windows 用户 `97936` 保持登录，可以断开 RDP。
3. 企业微信保持登录并运行。
4. Linux 用户 `martins3` 保持 GNOME 桌面登录。

Windows VM 当前 SSH 入口为：

```bash
ssh -p 51104 97936@localhost
```

如果端口发生变化，重新查询：

```bash
cd /home/martins3/data/vn
./collei/scripts/collei-action.py \
    -a ssh_auto \
    -n Win11_24H2_Chinese_Simplified_x64
```

需要进入 Windows 桌面时：

```bash
cd /home/martins3/data/vn
./collei/scripts/collei-action.py \
    -a rdp \
    -n Win11_24H2_Chinese_Simplified_x64
```

登录完成后可以关闭 RDP 窗口，通知桥仍会运行。

## 管理 Windows 任务

先 SSH 到 Windows，然后进入 PowerShell：

```bash
ssh -p 51104 97936@localhost
```

```powershell
powershell
```

查看任务状态：

```powershell
Get-ScheduledTask -TaskName "Codex-WeCom-Notify-Bridge"
Get-ScheduledTaskInfo -TaskName "Codex-WeCom-Notify-Bridge"
```

`State` 应为 `Running`。长期运行时 `LastTaskResult` 为 `267009`，即十六进制
`0x41301`，含义是“任务正在运行”，不是错误。

任务包含用户登录触发器和每分钟看门狗触发器。任务允许在电池状态下启动，并且不会
因为电源或空闲状态变化而停止。

查看监听进程是否位于交互会话：

```powershell
Get-CimInstance Win32_Process |
    Where-Object {
        $_.Name -eq "pythonw.exe" -and
        $_.CommandLine -match "wecom-notify-bridge"
    } |
    Select-Object ProcessId, SessionId, CommandLine
```

启动、停止或重启任务：

```powershell
Start-ScheduledTask -TaskName "Codex-WeCom-Notify-Bridge"
Stop-ScheduledTask -TaskName "Codex-WeCom-Notify-Bridge"

Stop-ScheduledTask -TaskName "Codex-WeCom-Notify-Bridge"
Start-Sleep -Seconds 2
Start-ScheduledTask -TaskName "Codex-WeCom-Notify-Bridge"
```

## 查看日志

在 Windows PowerShell 中查看最近事件：

```powershell
Get-Content "$env:LOCALAPPDATA\WeComNotifyBridge\bridge.log" -Tail 30
Get-Content "$env:LOCALAPPDATA\WeComNotifyBridge\state.txt"
```

正常发送包含以下记录：

```text
source=window ssh_exit=0
```

常见记录的含义：

- `source=window ssh_exit=0`：企业微信提示窗口触发了通知。
- `suppressed=active`：Windows 最近有人在前台操作企业微信，因此主动抑制。
- `error=missing_key`：Windows 中缺少专用 SSH 私钥。
- `ssh_exit` 非零：检查 VM 到 `10.0.2.2:22` 的连接、key 和 `known_hosts`。

日志只包含时间、触发来源和 SSH 退出状态，可以安全清空：

```powershell
Clear-Content "$env:LOCALAPPDATA\WeComNotifyBridge\bridge.log"
```

## 测试

仅测试 Linux GNOME 通知：

```bash
cd /home/martins3/data/vn
./docs/linux/fedora/wecom-notify-bridge/notify-from-windows.sh
```

成功时会看到固定通知，终端输出类似：

```text
(uint32 717,)
```

数字是 GNOME 返回的通知 ID，每次可能不同。

测试完整链路时，让另一个账号向未设置免打扰的会话发送消息，并确认日志出现
`source=window ssh_exit=0`。再向设置了免打扰的会话发送消息，预期不会增加日志，
也不会出现 GNOME 通知。测试时不要在 Windows 中操作企业微信，否则可能出现
`suppressed=active`；等待 10 秒后重试。

## 更新 Windows 脚本

修改 `wecom-notify-bridge.py` 后，先在 Linux 运行 Ruff 和 ty：

```bash
ruff check docs/linux/fedora/wecom-notify-bridge/wecom-notify-bridge.py
ty check docs/linux/fedora/wecom-notify-bridge/wecom-notify-bridge.py
```

检查通过后，从 Linux 重新复制并重启计划任务：

```bash
cd /home/martins3/data/vn
scp -P 51104 \
    docs/linux/fedora/wecom-notify-bridge/wecom-notify-bridge.py \
    97936@localhost:'C:/Users/97936/wecom-notify-bridge.py'
```

然后在 Windows PowerShell 中执行：

```powershell
Stop-ScheduledTask -TaskName "Codex-WeCom-Notify-Bridge"
Start-Sleep -Seconds 2
Start-ScheduledTask -TaskName "Codex-WeCom-Notify-Bridge"
```

修改 `notify-from-windows.sh` 后不需要复制到 Windows，下一次通知会直接使用新版。

## 故障排查

收到普通消息，但没有 `source=window`：

1. 使用 `quser` 确认 Windows 用户仍为 `Active` 或 `Disc`。
2. 使用 `Get-Process WXWork` 确认企业微信正在运行。
3. 检查计划任务和监听进程是否为 `Running`。
4. 确认对应会话没有设置免打扰，并且企业微信自身会显示消息提示窗口。

有 `source=window`，但 `ssh_exit` 非零：

1. 在 Windows 执行 `Test-NetConnection 10.0.2.2 -Port 22`。
2. 检查 `%LOCALAPPDATA%\WeComNotifyBridge\host_key` 和 `known_hosts`。
3. 检查 Linux `sshd` 是否运行。

有 `ssh_exit=0`，但桌面没有横幅：

1. 直接运行 Linux 通知脚本进行对照测试。
2. 确认 Linux 用户已登录 GNOME 桌面。
3. 检查 GNOME 横幅开关：

```bash
gsettings get org.gnome.desktop.notifications show-banners
```

预期输出 `true`。全屏程序或勿扰模式仍可能暂时隐藏横幅，但通知会进入 GNOME 通知列表。

## 安全边界

- Windows 端不读取或观察企业微信数据库。
- Linux 通知文本固定，不接受 Windows 传入的标题或正文。
- 专用 key 在 `authorized_keys` 中绑定强制命令，不能用于执行其他宿主命令。
- 不要把 `host_key` 复制到其他主机或提交到 Git。
- `--dump-once` 仅用于诊断企业微信窗口；它可能输出窗口标题，日常运行不会使用。

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
