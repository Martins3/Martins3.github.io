# Fedora 使用记录 2026

记录于 2026-04-26 ，总体体验极好，全都是我熟悉的图形界面操作，
终于不用处理各种奇怪的网络问题，例如配置 ovs / linux bridge，
随时可以调试 Linux 图形界面。

1. fedora 自己改善了很多
	- 170% 的放大比例，终于 4k 屏幕可以无痛使用了
	- steam 上我购买的所有的游戏都可以玩
2. ai 让一些配置容易起来了
	- rime-ice 的配置，我之前一直以为我配置对了，但是其实根本没有
	- nvidia 驱动安装
4. 微信
	- 居然可以原生支持，虽然有点点小问题，但是影响很小
5. nvidia 驱动
	- 多显示器，多 GPU 都支持的很好
	- 4k 显示器支持的很好
6. wine 也变好了很多
	- https://github.com/Zwhy2025/linux-wxwork
8. steam
	- 大多数游戏都是可以玩了

这个机器最开始是安装的 Server 版本，安装图形界面很容易:
```sh
sudo -S dnf install -y @gnome-desktop gdm
sudo -S systemctl enable gdm
sudo -S systemctl set-default graphical.target
systemctl is-enabled gdm
systemctl get-default
```
## 企业微信
基于 https://github.com/TibixDev/winboat

使用起来没有那么丝滑，但是目前这就是最佳解决方案。

## QQ 音乐
原生支持，我立刻直接充钱，从网易云切换到 QQ 音乐

存在问题:
- 没有托盘: https://github.com/flathub/com.qq.QQmusic/issues/25
- 过一段时间就会没有声音
- 每次关闭都需要重新登录

## 腾讯会议
已经有官方支持，但是只有 .deb 包
https://meeting.tencent.com/download/

不过可以通过 flatpak 直接安装:
```sh
flatpak install com.tencent.wemeet
```

2026-06-22 不过不知道为什么，登录的时候老是提示有网络问题
后面没尝试了。

## 安装杂记
1. ccls 居然被删除了 https://src.fedoraproject.org/rpms/ccls

1. 字体
https://www.nerdfonts.com/font-downloads

```sh
mkdir ~/.local/share/fonts
wget https://github.com/ryanoasis/nerd-fonts/releases/download/v3.4.0/FiraCode.zip
unzip *.zip  -d ~/.local/share/fonts/
fc-cache ~/.local/share/fonts
```

此外需要安装如下字体，不然浏览器中的
```sh
sudo dnf install -y google-noto-sans-fonts google-noto-sans-cjk-fonts google-noto-color-emoji-fonts dejavu-sans-fonts
```

2. edge 浏览器

```sh
sudo rpm --import https://packages.microsoft.com/keys/microsoft.asc
sudo dnf config-manager --add-repo https://packages.microsoft.com/yumrepos/edge
sudo mv /etc/yum.repos.d/packages.microsoft.com_yumrepos_edge.repo /etc/yum.repos.d/microsoft-edge-beta.repo
sudo dnf install microsoft-edge-beta
```

3. ghostty 安装

https://ghostty.org/docs/install/binary
```sh
sudo dnf copr enable scottames/ghostty
sudo dnf install ghostty
```

4. slack

直接下载 rpm ，然后到官方系统中安装。

5. thunderbird

商店中安装



7. pdf 阅读器

直接到 wps 官网中去下载就可以了。

8. wezterm
```sh
sudo dnf copr enable wezfurlong/wezterm-nightly
sudo dnf install wezterm
```

## 切换内核
3. 切换内核到
sudo dnf copr enable kwizart/kernel-longterm-5.15 fedora-38-x86_64
https://copr.fedorainfracloud.org/coprs/kwizart/kernel-longterm-5.15/
这个方法不好用，还不如直接下载 rpm 来安装的，其实也可以直接用我的经典方法

## flameshot

Fedora 44 + GNOME Wayland (Shell 50) 下，Flatpak Flameshot 14.0.0 已经开箱即用，不再需要任何 workaround:

```sh
flatpak install -y flathub org.flameshot.Flameshot
flatpak permission-set screenshot screenshot org.flameshot.Flameshot yes
```

第二条命令授予 screenshot portal 权限，避免每次截图弹 GNOME 授权对话框。

使用方式:

- 应用列表里右键 Flameshot 图标，选 "Take screenshot"（flatpak 自带的 desktop action，无需自定义 .desktop）。
- 或者绑定自定义快捷键到 `/usr/bin/flatpak run org.flameshot.Flameshot gui`。
- 框选区域后 Ctrl+C 复制。剪贴板立即可用，且 flameshot 进程退出后内容依然存在（14.0.0 会让捕获窗口存活到 compositor 取走数据为止，日志特征是 `GNOME Wayland detected; keeping capture window alive until clipboard data is fetched.`）。

注意事项:

- 不要同时安装 rpm 版 flameshot。两者都注册 `org.flameshot.Flameshot` 的 D-Bus service，D-Bus 激活会把截图请求静默路由给另一个版本。Fedora 44 仓库的 rpm 是 13.3.0，其 Wayland 剪贴板恰好是坏的（日志 `kf.guiaddons: Could not init WaylandClipboard, falling back to QtClipboard`，表现为复制无效）。14.0.0 通过 [PR #4363](https://github.com/flameshot-org/flameshot/pull/4363) 修复，但该修复只对 GUI 捕获窗口内的 copy 有效。
- `flameshot full -c` 这类无窗口的 CLI copy 在 GNOME Wayland 下依然无效（Wayland 要求设置剪贴板的客户端持有焦点窗口），日常使用 `gui` 即可。
- GNOME 下不要开 grim adapter（`useGrimAdapter=false`，默认即关闭）。grim 是给 sway/hyprland 这类 wlroots compositor 用的，GNOME 走 xdg-desktop-portal-gnome。遇到 `The universal wayland screen capture adapter requires Grim...` 就是这个原因。
- 如果误点了 portal 授权对话框的拒绝，重置权限:

  ```sh
  dbus-send --session --print-reply=literal --dest=org.freedesktop.impl.portal.PermissionStore /org/freedesktop/impl/portal/PermissionStore org.freedesktop.impl.portal.PermissionStore.DeletePermission string:'screenshot' string:'screenshot' string:''
  ```

- 确认系统包是否安装时，注意 PATH 里 Nix 的 `rpm` 可能排在 `/usr/bin/rpm` 前面，会误报"未安装"（它去读 Nix rpm 默认的 `/var/lib/rpm`）。用下面的命令确认:

  ```sh
  dnf list --installed wl-clipboard
  /usr/bin/rpm -q wl-clipboard
  ```

- AppIndicator 扩展仍然建议保留，它修复的是所有应用的托盘图标（微信的托盘 icon 也是装了它之后才正常显示的），只是 flameshot 自己已经不需要常驻托盘 daemon 了:

  ```sh
  sudo dnf install -y gnome-shell-extension-appindicator
  gnome-extensions enable appindicatorsupport@rgcjonas.gmail.com
  ```

  刚安装后 `gnome-extensions enable` 可能报 `does not exist`，注销重新登录即可。

### 历史

Flameshot 13.3.0 时代，本节的方案是 wl-copy wrapper + systemd path unit 监控保存目录，把每次新截图重新塞进剪贴板，极为逆天。
14.0.0 修复剪贴板之后，整套机制（`~/.local/bin/flameshot-copy*`、`flameshot-copy-latest.path/.service`、自定义 .desktop override、autostart daemon）
都已验证不再需要并删除。

## 内核管理
1. 清理不需要的 kernel 的方法，似乎没有特别好的办法:
```sh
current=$(uname -r | sed 's/\.x86_64$//')
sudo rpm -qa | grep -E '^kernel.*-[0-9]+\.[0-9]+\.[0-9]+' | grep -v "$current"

# 用 dnf 来删除，会自动的解决依赖
sudo dnf remove "kernel-*-6.19.13-200.fc43.x86_64"
```

2. 安装 debuginfo 的基本方法

似乎这两种方法都是可以的，但是我遇到问题，就是不是所有的 kernel 都是满足这个需求的:
因为 fedora 仓库中只有最新的 kernel 包，所以下次安装的时候，一定需要先安装 debuginfo ，然后继续调试:
```sh
sudo dnf debuginfo-install kernel-$(uname -m)
sudo dnf --enablerepo=fedora-debuginfo,updates-debuginfo install kernel-debuginfo-$(uname -r)
```

一般来说，自动管理就可以了:
```sh
sudo dnf upgrade kernel
sudo dnf upgrade kernel-debuginfo
```

## 打印机使用
```txt
sudo dnf install -y cups cups-client cups-filters avahi-tools gutenprint-cups system-config-printer

sudo systemctl enable --now cups cups-browsed
```

使用 lps 有问题:
```txt
sudo lpadmin -p FX_ApeosPort_C2560 -E -v ipps://FX4fb4ac.local/ipp/print -m everywhere
```

普通的没问题:
```txt
sudo lpadmin -p FX_ApeosPort_C2560 -E -v ipp://FX4fb4ac.local/ipp/print -m everywhere
```

设置默认打印机
sudo lpadmin -d FX_ApeosPort_C2560

最后结果:
```txt
🧀  lpstat -t
scheduler is running
system default destination: FX_ApeosPort_C2560
device for FX_ApeosPort_C2560: ipp://FX4fb4ac.local/ipp/print
FX_ApeosPort_C2560 accepting requests since Wed 17 Jun 2026 08:33:53 AM CST
printer FX_ApeosPort_C2560 is idle.  enabled since Wed 17 Jun 2026 08:33:53 AM CST
```

如果想看扫描结果，直接看就可以了，都是图形界面操作
就没有好说的了:
```txt
http://192.168.1.30/home/index.html
```

## 参考
https://world.hey.com/dhh/linux-as-the-new-developer-default-at-37signals-ef0823b7

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
