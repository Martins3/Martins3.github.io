# Windows 支持

我尝试了一下 WinBoat ，通过 rdp 其基本上解决了我需要的任何问题:
1. 虚拟机有声音
2. 物理机和虚拟机中剪切板互通

但是 winboat 启动的虚拟机 CPU 消耗比较高，我用起来不习惯，所以集成到 collei 中了:

## 调研

- https://github.com/Devolutions/IronRDP
	- https://github.com/winapps-org/winapps 用的是 https://github.com/FreeRDP

使用 dotfiles 中的 scripts/nix/env/ironrdp.nix ，cargo build ，但是运行有点问题
以后再看吧:
```txt
./target/debug/ironrdp-viewer 192.168.1.100 --username admin  --password secret

Error: os error at /home/martins3/.cargo/registry/src/index.crates.io-1949cf8c6b5b557f/winit-0.30.13/src/platform_impl/linux/mod.rs:765: neither WAYLAND_DISPLAY nor WAYLAND_SOCKET nor DISPLAY is set.
```

- https://github.com/TibixDev/winboat
- https://github.com/miroslavpejic85/p2p
- https://rustdesk.com/zh/
- https://github.com/screego/server
- https://github.com/kunkundi/crossdesk


## 问题解决

### Windows 如何 启用 RDP

Windows 激活状态不是本次问题。远程检查确认系统版本是 Professional，支持
作为 RDP server。RDP 未工作时的实际状态是：

```text
Edition=Professional
Elevated=True
RdpDenied=1
Sshd=Running/Automatic
```

通过 SSH 上传并运行 `scripts/windows-enable-rdp.ps1` 后：

```text
RdpDenied=0
NLA=0
TermService=Running/Automatic
FirewallTCP=True
FirewallUDP=True
```

但是 3389 当时仍未监听。原因是 `TermService` 在修改注册表之前已经运行，
`Start-Service` 不会让运行中的服务重新读取配置，而该 Windows 会话又拒绝停止
`TermService`。客户机正常重启后，3389 开始同时监听 IPv4 和 IPv6：

```text
0.0.0.0:3389 Listen
[::]:3389 Listen
```

因此首次执行 RDP 脚本后需要重启 Windows。脚本只在 `TermService` 未运行时
启动服务，并明确提示首次连接前重启。

首次从 VNC 手工执行脚本时使用过：

```powershell
Set-ExecutionPolicy -Scope Process Bypass
& "\\10.0.2.4\qemu\data\vn\collei\scripts\windows-enable-rdp.ps1"
```

第一条只临时放宽当前 PowerShell 进程的脚本策略，关闭窗口后失效，不修改系统
永久策略。第二条通过 QEMU user networking 的 Samba 共享访问宿主
`/home/martins3/data/vn/collei/scripts/windows-enable-rdp.ps1`。脚本不读取、保存
或修改 Windows 密码。

RDP 不能使用 Windows Hello PIN，需要账户的实际密码。如果本地账户没有可用
密码，可以通过已经配置好的 SSH 公钥登录后交互式修改：

```bash
ssh -tt -p 51104 -l 97936 127.0.0.1 "net user 97936 *"
```

`gr` 自动连接所使用的密码保存在 VM 的 `opt/rdp_password`，并且该文件必须
设置为 `0600`；密码不会写入命令参数或环境变量。
### FreeRDP 本地代理问题

第一次使用 FreeRDP 探测时，日志显示它读取了宿主代理：

```text
Parsed proxy configuration: http://127.0.0.1:7890
```

结果是本应访问 `127.0.0.1:51118` 的流量被错误送往 HTTP proxy。仅设置
`NO_PROXY` 不一定能覆盖所有 FreeRDP 构建，所以 `action_rdp()` 同时清空大小写
形式的 `HTTP_PROXY`、`HTTPS_PROXY` 和 `ALL_PROXY`，并设置大小写形式的
`NO_PROXY=127.0.0.1,localhost`。

### FreeRDP 警告

成功连接时出现过以下警告，它们不表示 RDP 失败：

- `no RDP scancode found`：XKB 中少数特殊键没有 RDP scancode，普通键盘输入
  通常不受影响。
- `WITH_GFX_AV1=ON`：本机 FreeRDP 构建启用了实验性 AV1，只是构建警告。
- `Certificate not checked`：当前使用 `/cert:ignore` 忽略 Windows 自签名 RDP
  证书。连接只监听在宿主 loopback，并通过 QEMU 转发，没有暴露到局域网；但
  它仍不验证客户机身份。

可以在后续将 `/cert:ignore` 改成 `/cert:tofu`，首次记录证书，后续检测证书
变化。

### 分辨率和缩放

全屏 `/f` 使用显示器原生分辨率。高 DPI 显示器上降低分辨率会导致画面模糊，
因此最终保留原生分辨率，在 `action_rdp()` 中增加：

```text
/scale-desktop:200
```

它将桌面应用放大到 200%。需要改变实际远程分辨率时，应取消 `/f` 并使用
`/size:<width>x<height>`；一般不推荐用这种方法解决字体太小。


他利用虚拟化 加上 FreeRDP 来实现功能
- 虚拟化用的是: https://github.com/dockur/windows

## 自动安装
无人值守安装真正来自 dockur/windows，而非 WinBoat。核心流程是 install.sh 中的 detectImage()、updateXML()、addDrivers()、updateImage() 和 buildImage()：

1. 解开 Windows ISO，检查 install.wim/boot.wim。
2. 自动识别 Windows 版本、架构和语言。
3. 将存储、网卡、balloon、GPU、串口等 VirtIO 驱动注入 boot.wim。
4. 生成并注入 autounattend.xml。
5. 写入分区、用户、自动登录、RDP、防火墙及 Windows 11 检查绕过配置。
6. 注入 $OEM$，在安装末尾执行 install.bat。
7. 重建可启动 ISO。dockur/windows 5.16 安装实现 (https://raw.githubusercontent.com/dockur/windows/c768ba894d100996bf58d050c41065dac3370e02/src/install.sh)。

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
