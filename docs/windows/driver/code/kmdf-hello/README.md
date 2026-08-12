# KMDF Hello

这是一个最小的 root-enumerated KMDF 驱动，用于验证 Visual Studio、SDK、WDK、INF、
catalog 和测试签名链路。

它只实现两个入口：

- `DriverEntry()`：初始化 `WDF_DRIVER_CONFIG` 并创建 framework driver object；
- `KmdfHelloEvtDeviceAdd()`：响应 PnP manager 的 `AddDevice`，创建 `WDFDEVICE`。

## 构建

把目录复制到 Windows 后执行：

```powershell
pwsh -NoLogo -NoProfile -ExecutionPolicy Bypass `
  -File "$HOME\.dotfiles\scripts\windows-driver-build.ps1" `
  -Solution "$HOME\data\kmdf-hello\kmdf-hello.vcxproj"
```

输出位于 `build/x64/Debug`。构建必须使用脚本选择的 64 位 MSBuild。

已验证的输出包括 `KmdfHello.sys`、`kmdf-hello.inf` 和 `kmdfhello.cat`。构建过程完成 INF
verification、API validation、catalog generation 和 WDK 测试签名。

## 安全边界

不要在日常开发机直接安装。应把输出复制到带快照的测试 VM，配置测试签名与 WinDbg 后再用
下面的 hardware ID 安装：

```text
ROOT\MARTINS3_KMDF_HELLO
```

## 后续安装清单

当前只完成构建和 WDK 测试签名，尚未在任何 Windows 中安装或加载。后续只在有外层快照和
控制台的测试 VM 中按以下顺序执行：

1. 导出并复制 catalog 使用的 `WDKTestCert` 和完整驱动包；
2. 把证书导入 target 的 Local Machine `Trusted Root` 与 `Trusted Publishers`；
3. 执行 `bcdedit /set testsigning on` 并重启；
4. 连接 WinDbg，设置 `DriverEntry()` 和 `KmdfHelloEvtDeviceAdd()` 的 deferred breakpoint；
5. 用官方 samples 构建的 x64 DevCon 执行：

   ```powershell
   devcon.exe install .\kmdf-hello.inf 'ROOT\MARTINS3_KMDF_HELLO'
   ```

6. 验证 PnP device、`Martins3KmdfHello` 服务、`KmdfHello.sys`、签名状态以及
   `KdPrintEx()` 输出；
7. 测试结束后恢复快照，或删除设备、driver package 和证书，再关闭 `TESTSIGNING` 并重启。

下载 WDK、WinDbg、samples、构建 DevCon、导入证书、安装验证和手工回滚的可复制命令见
`~/.dotfiles/docs/windows-driver.md` 的“将 KmdfHello 安装到测试 VM”章节。

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
