# Fedora 使用记录 2026
## 基本记录
- ccls 居然被删除了 https://src.fedoraproject.org/rpms/ccls

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

3. 切换内核到
sudo dnf copr enable kwizart/kernel-longterm-5.15 fedora-38-x86_64
https://copr.fedorainfracloud.org/coprs/kwizart/kernel-longterm-5.15/
这个方法不好用，还不如直接下载 rpm 来安装的，其实也可以直接用我的经典方法

### 6. flameshot

Fedora 43 + GNOME Wayland 下，推荐使用 Flatpak 版本:

```sh
flatpak install -y flathub org.flameshot.Flameshot
sudo dnf install -y wl-clipboard
```

注意这里必须确认 Fedora 系统的 `wl-clipboard` 已安装。如果 PATH 里 Nix 的 `rpm` 排在 `/usr/bin/rpm` 前面，`rpm -q wl-clipboard` 可能误报没有安装，因为它会去读 Nix rpm 默认的 `/var/lib/rpm`。用下面的命令确认:

```sh
dnf list --installed wl-clipboard
/usr/bin/rpm -q wl-clipboard
```

如果从终端启动遇到:

```txt
qt.qpa.xcb: could not connect to display
Could not load the Qt platform plugin "xcb"
```

通常是当前 shell/tmux/zellij 没有继承 GNOME Wayland 环境，只有 `DISPLAY=:0`，没有 `WAYLAND_DISPLAY=wayland-0`。

临时启动:

```sh
env WAYLAND_DISPLAY=wayland-0 XDG_SESSION_TYPE=wayland QT_QPA_PLATFORM=wayland \
  flatpak run org.flameshot.Flameshot gui
```

用户级 launcher 覆盖:

```txt
~/.local/share/applications/org.flameshot.Flameshot.desktop
```

内容:

```ini
[Desktop Entry]
Name=Flameshot
Name[zh_CN]=火焰截图
GenericName=Screenshot tool
GenericName[zh_CN]=屏幕截图工具
Comment=Powerful yet simple to use screenshot software.
Comment[zh_CN]=强大又易用的屏幕截图软件
Keywords=flameshot;screenshot;capture;shutter;
Keywords[zh_CN]=flameshot;screenshot;capture;shutter;截图;屏幕;
Exec=/usr/bin/env WAYLAND_DISPLAY=wayland-0 XDG_SESSION_TYPE=wayland XDG_CURRENT_DESKTOP=GNOME QT_QPA_PLATFORM=wayland /usr/bin/flatpak run --branch=stable --arch=x86_64 --command=flameshot org.flameshot.Flameshot
Icon=org.flameshot.Flameshot
Terminal=false
Type=Application
Categories=Graphics;
StartupNotify=false
StartupWMClass=flameshot
Actions=Configure;Capture;Launcher;
X-Flatpak=org.flameshot.Flameshot

[Desktop Action Configure]
Name=Configure
Name[zh_CN]=配置
Exec=/usr/bin/env WAYLAND_DISPLAY=wayland-0 XDG_SESSION_TYPE=wayland XDG_CURRENT_DESKTOP=GNOME QT_QPA_PLATFORM=wayland /usr/bin/flatpak run --branch=stable --arch=x86_64 --command=flameshot org.flameshot.Flameshot config

[Desktop Action Capture]
Name=Take screenshot
Name[zh_CN]=进行截图
Exec=/home/martins3/.local/bin/flameshot-copy

[Desktop Action Launcher]
Name=Open launcher
Name[zh_CN]=打开启动器
Exec=/usr/bin/env WAYLAND_DISPLAY=wayland-0 XDG_SESSION_TYPE=wayland XDG_CURRENT_DESKTOP=GNOME QT_QPA_PLATFORM=wayland /usr/bin/flatpak run --branch=stable --arch=x86_64 --command=flameshot org.flameshot.Flameshot launcher
```

刷新和校验:

```sh
desktop-file-validate ~/.local/share/applications/org.flameshot.Flameshot.desktop
update-desktop-database ~/.local/share/applications
```

Fedora 43 + GNOME Wayland 上，Flatpak Flameshot 13.3.0 可能出现“截图可以保存，但是复制到剪切板无效”的问题。日志特征是:

```txt
flameshot: info: Capture saved to clipboard.
kf.guiaddons: Could not init WaylandClipboard, falling back to QtClipboard.
```

此时 screenshot portal 已经可用，坏的是 Flameshot 内置的 Wayland 剪切板路径。绕过方法是不要用内置 copy，而是让 Flameshot 输出 PNG 到 stdout，再交给 `wl-copy`，同时保存一份到 `~/Pictures/Screenshots`:

```sh
/usr/bin/flatpak run --branch=stable --arch=x86_64 --command=flameshot \
  org.flameshot.Flameshot gui --delay 500 --raw | wl-copy --type image/png
```

为了让 GNOME launcher 也能找到 Nix 或系统中的 `wl-copy`，放一个 wrapper:

```sh
~/.local/bin/flameshot-copy
```

内容:

```bash
#!/usr/bin/env bash
set -E -e -u -o pipefail

SCREENSHOT_PATH=""

function cleanup() {
  if [[ -n ${SCREENSHOT_PATH:-} ]]; then
    rm -f -- "$SCREENSHOT_PATH"
  fi
}

function screenshot_dir() {
  local dir="${HOME}/Pictures/Screenshots"
  mkdir -p -- "$dir"
  printf '%s\n' "$dir"
}

function main() {
  export PATH="/home/martins3/.nix-profile/bin:/nix/var/nix/profiles/default/bin:/usr/local/bin:/usr/bin:/bin:${PATH:-}"
  export WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-wayland-0}"
  export XDG_SESSION_TYPE="${XDG_SESSION_TYPE:-wayland}"
  export XDG_CURRENT_DESKTOP="${XDG_CURRENT_DESKTOP:-GNOME}"
  export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-wayland}"

  if (($# > 0)); then
    /usr/bin/flatpak run --branch=stable --arch=x86_64 --command=flameshot \
      org.flameshot.Flameshot "$@"
    return
  fi

  local wl_copy
  wl_copy="$(command -v wl-copy)"

  SCREENSHOT_PATH="$(mktemp "${TMPDIR:-/tmp}/flameshot-copy.XXXXXX")"
  trap cleanup EXIT

  /usr/bin/flatpak run --branch=stable --arch=x86_64 --command=flameshot \
    org.flameshot.Flameshot gui --delay 500 --raw >"$SCREENSHOT_PATH"

  if [[ -s "$SCREENSHOT_PATH" ]]; then
    "$wl_copy" --type image/png <"$SCREENSHOT_PATH"
    cp -- "$SCREENSHOT_PATH" "$(screenshot_dir)/flameshot-$(date +%Y%m%d-%H%M%S).png"
  fi
}

main "$@"
```

```sh
chmod +x ~/.local/bin/flameshot-copy
```

注意: 上面的 wrapper 只影响 `.desktop` 的 `Take screenshot` action 或手动执行 `~/.local/bin/flameshot-copy`。右上角托盘 icon 里面的截图按钮属于 Flameshot 进程内部逻辑，不会经过 `.desktop Exec`，所以仍然会走坏掉的 Qt/KSystemClipboard。

托盘 icon 截图也要进入 Wayland 剪切板时，使用这个绕过方案:

1. 让 Flameshot 内部 copy 后自动保存图片。
2. 用 user systemd path 监控保存目录。
3. 发现新 PNG 后，用系统的 `/usr/bin/wl-copy --foreground` 持有真正的 Wayland 剪切板。

Flameshot 配置:

```ini
# ~/.var/app/org.flameshot.Flameshot/config/flameshot/flameshot.ini
[General]
contrastOpacity=188
saveAfterCopy=true
saveAsFileExtension=png
savePath=/home/martins3/Pictures/Screenshots
useGrimAdapter=false
```

`~/.local/bin/flameshot-wl-copy-file`:

```bash
#!/usr/bin/env bash
set -E -e -u -o pipefail

function main() {
  if (($# != 1)); then
    printf 'usage: %s IMAGE.png\n' "${0##*/}" >&2
    return 2
  fi

  exec /usr/bin/wl-copy --foreground --type image/png <"$1"
}

main "$@"
```

`~/.local/bin/flameshot-copy-latest`:

```bash
#!/usr/bin/env bash
set -E -e -u -o pipefail

function latest_png() {
  find "${HOME}/Pictures/Screenshots" -maxdepth 1 -type f -name '*.png' -printf '%T@ %p\n' |
    sort -rn |
    sed -n '1{s/^[^ ]* //;p;}'
}

function wait_until_stable() {
  local file="$1"
  local before
  local after

  for _ in {1..20}; do
    before="$(stat -c '%s' "$file")"
    sleep 0.1
    after="$(stat -c '%s' "$file")"
    if [[ $before == "$after" && $after != 0 ]]; then
      return 0
    fi
  done

  return 1
}

function main() {
  local file
  file="$(latest_png)"
  if [[ -z $file ]]; then
    return 0
  fi

  wait_until_stable "$file"

  local stamp
  stamp="$(stat -c '%n:%Y:%s' "$file")"

  local state="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}/flameshot-copy-latest.state"
  if [[ -f $state && $(<"$state") == "$stamp" ]]; then
    return 0
  fi
  printf '%s\n' "$stamp" >"$state"

  systemctl --user stop flameshot-wl-copy.service 2>/dev/null || true
  systemd-run --user --unit=flameshot-wl-copy --collect \
    /home/martins3/.local/bin/flameshot-wl-copy-file "$file" >/dev/null
}

main "$@"
```

```sh
chmod +x ~/.local/bin/flameshot-copy-latest ~/.local/bin/flameshot-wl-copy-file
```

`~/.config/systemd/user/flameshot-copy-latest.service`:

```ini
[Unit]
Description=Copy latest Flameshot screenshot to Wayland clipboard

[Service]
Type=oneshot
ExecStart=%h/.local/bin/flameshot-copy-latest
```

`~/.config/systemd/user/flameshot-copy-latest.path`:

```ini
[Unit]
Description=Watch Flameshot screenshot directory

[Path]
PathChanged=%h/Pictures/Screenshots
PathModified=%h/Pictures/Screenshots
Unit=flameshot-copy-latest.service

[Install]
WantedBy=default.target
```

启动:

```sh
mkdir -p ~/Pictures/Screenshots
systemctl --user daemon-reload
systemctl --user enable --now flameshot-copy-latest.path
```

验证:

```sh
systemctl --user status flameshot-copy-latest.path flameshot-wl-copy.service
wl-paste --list-types
wl-paste --type image/png | wc -c
```

如果遇到:

```txt
The universal wayland screen capture adapter requires Grim as the screen capture component of wayland.
```

不要在 GNOME Wayland 下继续强制使用 `grim`。`grim` 主要适合 wlroots compositor，例如 sway/hyprland。GNOME 应该走 `xdg-desktop-portal-gnome`。

给 Flatpak Flameshot 授权 screenshot portal:

```sh
flatpak permission-set screenshot screenshot org.flameshot.Flameshot yes
flatpak permissions screenshot
```

关闭 Flatpak Flameshot 的 grim adapter:

```ini
# ~/.var/app/org.flameshot.Flameshot/config/flameshot/flameshot.ini
[General]
contrastOpacity=188
useGrimAdapter=false
```

如果有旧的 Flameshot 进程，先杀掉再重启:

```sh
pgrep -af 'flameshot|flatpak run.*Flameshot|/app/bin/flameshot'
kill <pid>
```

注意: 如果 `/usr/bin/flameshot`、`/usr/bin/grim`、`/usr/bin/slurp` 存在，但是 `rpm -qf` 显示不属于任何 rpm 包，说明它们是手工残留文件。此时优先使用 Flatpak 入口，不要使用裸 `flameshot gui`。

GNOME 右上角没有图标，是因为 GNOME 默认不显示传统托盘。安装 AppIndicator/KStatusNotifier 扩展:

```sh
sudo dnf install -y gnome-shell-extension-appindicator
gsettings set org.gnome.shell enabled-extensions "['kimpanel@kde.org', 'BingWallpaper@ineffable-gmail.com', 'appindicatorsupport@rgcjonas.gmail.com']"
```

如果当前会话中 `gnome-extensions enable appindicatorsupport@rgcjonas.gmail.com` 报 `does not exist`，通常是 GNOME Shell 还没有加载新安装的扩展。注销后重新登录即可。

让 Flameshot 登录后自动常驻:

```txt
~/.config/autostart/org.flameshot.Flameshot.desktop
```

内容:

```ini
[Desktop Entry]
Type=Application
Name=Flameshot
Name[zh_CN]=火焰截图
Comment=Start Flameshot in the background
Exec=/usr/bin/env WAYLAND_DISPLAY=wayland-0 XDG_SESSION_TYPE=wayland XDG_CURRENT_DESKTOP=GNOME QT_QPA_PLATFORM=wayland /usr/bin/flatpak run --branch=stable --arch=x86_64 --command=flameshot org.flameshot.Flameshot
Icon=org.flameshot.Flameshot
Terminal=false
X-GNOME-Autostart-enabled=true
```

校验:

```sh
desktop-file-validate ~/.config/autostart/org.flameshot.Flameshot.desktop
```

最后需要注销并重新登录 GNOME:

- AppIndicator 扩展生效
- Flameshot 自动后台启动
- 右上角显示 Flameshot icon

#### 注意
我发现了一个非常有意思的问题，似乎 flameshot 解决之后，然后我发现我机器的 icon 机制变正常了，
现在微信的 icon 也存在了。

我相信这是一个极为 workaround 的解决办法，使用 systemd 服务来加上 fsnotify
我的天啊，只有 codex 这种天才才可以想到这么 nb 的方法。

## 最后几个有趣的问题
1. steam
2. 企业微信
3. 腾讯会议

## 清理不需要的 kernel 的方法

似乎没有特别好的办法:
```sh
current=$(uname -r | sed 's/\.x86_64$//')
sudo rpm -qa | grep -E '^kernel.*-[0-9]+\.[0-9]+\.[0-9]+' | grep -v "$current"

# 用 dnf 来删除，会自动的解决依赖
sudo dnf remove "kernel-*-6.19.13-200.fc43.x86_64"
```
## 安装 debuginfo 的基本方法

似乎这两种方法都是可以的，但是我遇到问题，就是不是所有的 kernel 都是满足这个需求的:
因为 fedora 仓库中只有最新的 kernel 包，所以下次安装的时候，一定需要先安装 debuginfo ，然后继续调试:
```txt
sudo dnf debuginfo-install kernel-$(uname -m)
sudo dnf --enablerepo=fedora-debuginfo,updates-debuginfo install kernel-debuginfo-$(uname -r)
```

## fedora 安装图形界面
就是这样，完全可以走通的:

sudo -S dnf install -y @gnome-desktop gdm
sudo -S systemctl enable gdm
sudo -S systemctl set-default graphical.target
systemctl is-enabled gdm
systemctl get-default

## 打印机
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

## 2026-04-26 尝试 fedora 的感触

两个原因:
1. fedora 自己改善了很多
	- 170% 的放大比例，终于 4k 屏幕可以无痛使用了
	- steam 上我购买的所有的游戏都可以玩
2. ai 让一些配置容易起来了
	- rime-ice 的配置，我之前一直以为我配置对了，但是其实根本没有
	- nvidia 驱动安装
3. 社区的进步
	- wine 的网易云
	- 微信都是可以直接使用的
4. 微信
5. nvidia 驱动
	- 多显示器，多 GPU 都支持的很好
	- 4k 显示器支持的很好
6. wine 也变好了很多
	- https://github.com/Zwhy2025/linux-wxwork

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
