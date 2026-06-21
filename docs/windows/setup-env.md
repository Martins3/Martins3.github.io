# windows 环境搭建

安装字体:
https://www.nerdfonts.com/font-downloads
解压，右键，里面有安装

但是这个东西似乎安装了也没有效果啊:
```txt
https://github.com/tonsky/FiraCode/wiki/Installing
```

## windows terminal tmux 类似的效果
https://learn.microsoft.com/en-us/windows/terminal/panes

https://vladelaina.github.io/Catime/

## neovide
1. 字体
2. 如何放大所有的字体

neovide 也是不错的了

```txt
Error: Font can't be updated to: FontOptions {
    normal: [
        FontDescription {
            family: "monospace",
            style: None,
        },
    ],
    italic: None,
    bold: None,
    bold_italic: None,
    features: {},
    size: 22.666668,
    width: 0.0,
    hinting: Full,
    edging: AntiAlias,
}
Following fonts couldn't be loaded: FontKey { font_desc: Some(FontDescription { family: "monospace", style: Some("Bold Italic") }), hinting: Full, edging: AntiAlias },
FontKey { font_desc: Some(FontDescription { family: "monospace", style: Some("Bold") }), hinting: Full, edging: AntiAlias },
FontKey { font_desc: Some(FontDescription { family: "monospace", style: Some("Italic") }), hinting: Full, edging: AntiAlias },
FontKey { font_desc: Some(FontDescription { family: "monospace", style: None }), hinting: Full, edging: AntiAlias }

E354: Invalid register name: '^@'
```


### sln 和 vcproj 作用是什么
https://stackoverflow.com/questions/7133796/what-are-sln-and-vcproj-files-and-what-do-they-contain

### 下载主题
工具，选项，调整字体

点 extension 就可以了
Catppuccin

### 使用 msbuild

https://stackoverflow.com/questions/39798321/generate-clang-compilation-database-for-a-visual-studio-project
```txt
msbuild .\01-ErrorShow.vcxproj /t:ClangTidy -t:Rebuild -p:Configuration=Release -p:Platform=X64
```
可以自动生成 compile_commands.json

这个工具不靠谱的:
https://clangpowertools.com/

如何使用:
- https://learn.microsoft.com/en-us/visualstudio/msbuild/walkthrough-using-msbuild?view=vs-2022
- https://learn.microsoft.com/en-us/visualstudio/msbuild/msbuild-command-line-reference?view=vs-2022


解决 msbuild 的环境变量问题:
- https://stackoverflow.com/questions/6319274/how-do-i-run-msbuild-from-the-command-line-using-windows-sdk-7-1

目前在这个路径:
```txt
PS C:\Program Files> fzf
Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe
```

## 一些技巧
- vsVim : Visual Studio 的 vim 模式
- PowerToys 来切换键盘的键位映射

## 打开 ssh 功能
- https://learn.microsoft.com/en-us/windows-server/administration/openssh/openssh_install_firstuse?tabs=powershell&pivots=windows-11

使用 powershell ，安装 server 似乎过程很慢，预期的

在 powershell 中可以用这个登录，这个用户名是通过 `WHOAMI` 获取的
```txt
ssh "martins3\97936@10.0.0.8"
```
但是在 linux 中需要添加双引号

## 构建 qemu
并不容易，也不原生，不知道那些 virt-manager 之类的工具都是如何解决的:
https://stackoverflow.com/questions/53084815/compile-qemu-under-windows-10-64-bit-for-windows-10-64-bit

## 参考
https://github.com/jayharris/dotfiles-windows

## 已经解决的
### 为什么我的 git sync 很慢

观察到了一个非常奇怪的事情，就是在 windows 上使用 gsy 特别慢

无论是 windows 的 wsl / hyper-v manager 虚拟机中，还是物理机中

但是 git pull 很快?

哦，是代理有问题，导致走 gitee 也是走代理了

### 微信和 powertoys 的冲突问题
参考:
https://github.com/microsoft/PowerToys/issues/21877#issuecomment-1876571225

将 Fn22 映射为 Disable

### hyperv 虚拟机的网卡 ip
固定 mac 地址

如果不行，继续看看:
https://superuser.com/questions/1526309/hyper-v-default-switch-static-ip

最后结局办法是定义一个工具在 powershell 中的

## 还没解决的问题
### vim 启动太慢了
如果遇到，那么在环境调试

### windows 环境基本配置
https://github.com/glzr-io/glazewm

配置文件在:
C:\Users\97936\.glzr\glazewm

似乎还不错，需要将 animation 关闭掉

参考这个试试吧:
- https://superuser.com/questions/940342/how-to-change-shortcut-key-for-switching-between-virtual-desktops-in-windows-10
  - 参考这个答案 : https://superuser.com/a/1050690
    - 这个配置也是有 bug 的
- https://github.com/pmb6tz/windows-desktop-switcher

## uv 的使用

似乎需要执行这个来激活:
```pwsh
.venv\Scripts\activate.ps1
```

## 关闭动画
https://guanjia.qq.com/knowledge-base/content/1127?from=clinic

## 键盘速度
```powershell
$path = 'HKCU:\Control Panel\Keyboard'
Set-ItemProperty -Path $path -Name KeyboardDelay -Value '0'
Set-ItemProperty -Path $path -Name KeyboardSpeed -Value '31'

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public class KeyboardNative {
  [DllImport("user32.dll", SetLastError=true)]
  public static extern bool SystemParametersInfo(uint uiAction, uint uiParam, IntPtr pvParam, uint fWinIni);
}
'@

$SPIF_UPDATEINIFILE = 0x01
$SPIF_SENDCHANGE = 0x02

[KeyboardNative]::SystemParametersInfo(0x0017, 0, [IntPtr]::Zero, $SPIF_UPDATEINIFILE -bor $SPIF_SENDCHANGE) | Out-Null
[KeyboardNative]::SystemParametersInfo(0x000B, 31, [IntPtr]::Zero, $SPIF_UPDATEINIFILE -bor $SPIF_SENDCHANGE) | Out-Null

Get-ItemProperty -Path $path | Select-Object KeyboardDelay, KeyboardSpeed, InitialKeyboardIndicators

更简单的复现版本也可以只执行：

Set-ItemProperty -Path 'HKCU:\Control Panel\Keyboard' -Name KeyboardDelay -Value '0'
Set-ItemProperty -Path 'HKCU:\Control Panel\Keyboard' -Name KeyboardSpeed -Value '31'
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
