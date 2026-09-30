# virtio-win 驱动手动构建与安装记录（BUILD NOTES）

> [!NOTE]
> 参考神奇海螺的意见，有待验证
> glm 5.3

日期：2026-09-11 目标：在 Win11_24H2 虚拟机内，从 kvm-guest-drivers-windows
master 源码手动编译并安装全套最新 virtio-win 驱动（测试签名模式）。

## 结果

- 源码：master `95153a652421`（"Update status.txt with release mm323"）
- 构建配置：`build_AllNoSdv.bat Win11 amd64`（EWDK 24H2 环境，Win11 Release
  x64）
- 全部 16 个驱动包产出 `Install\Win11\amd64`（.sys/.cat/.inf 齐全），34
  个文件用仓库测试证书 VirtIOTestCert 签名，`Get-AuthenticodeSignature` 状态
  Valid
- guest 已开启 `bcdedit /set testsigning on`，证书已导入 Root + TrustedPublisher
- 16 个驱动包全部 pnputil 安装，重启后所有 virtio 设备运行在
  **100.6.101.58000**（master 构建号 58000，DriverDate 2026/9/11）
- 驱动产物归档：本目录 `virtio-win-master-95153a65.tar.gz`（2.3M，不含 pdb）
- 唯一 error 设备 `PCI\VEN_1234&DEV_11E8` 是 QEMU `edu` 教学设备，从来没有
  Windows 驱动，属预期

## 构建材料

| 材料           | 大小（字节）   | sha256                                                             | 位置                                                                                      |
| -------------- | -------------- | ------------------------------------------------------------------ | ----------------------------------------------------------------------------------------- |
| EWDK 24H2 ISO  | 19,100,119,040 | `9a23f3399bf6b80b119bcaf9def8679ed296f4b7d742e0251cf0b76a3fb97f77` | `~/data/hack/iso/EWDK_ge_release_26100_240331-1435.iso`                                   |
| WinFSP 2.0 MSI | 2,207,744      | `6324dc81194a6a08f97b6aeca303cf5c2325c53ede153bae9fc4378f0838c101` | `~/data/hack/iso/winfsp-2.0.23075.msi`                                                    |
| CPDK 安装器    | 1,281,504      | `76d6f7580fce9bb1045b5f0871718342443ec5d99c316489ef8c7bc83d4dd6ed` | `~/data/hack/iso/cpdksetup.exe`                                                           |
| 源码           | —              | master `95153a652421`                                              | `~/data/kvm-guest-drivers-windows`（host）/ `C:\build\kvm-guest-drivers-windows`（guest） |

- EWDK：Enterprise WDK，免安装，挂载即用；环境入口 `BuildEnv\SetupBuildEnv.cmd`
- WinFSP：viofs 构建前置；`msiexec /qn ADDLOCAL=ALL` 静默安装
- CPDK：viocrypt 和 viorng(cng) 构建前置；`/q /norestart` 静默安装，装到
  `C:\Program Files (x86)\Windows Kits\10\Cryptographic Provider Development Kit\`（viorngum.vcxproj
  硬编码此路径）
- 源码构建不依赖 .git，tar 打包传输即可

## 下载方法（以下 URL 均于 2026-09-11 验证可访问、大小吻合）

### 1. EWDK 24H2 ISO（19.1G，本环境直接 host 下载）

- 官方入口：learn.microsoft.com 的 "Download the WDK" 页 → Enterprise WDK
  (EWDK)（24H2 版条目在 2024 年中版本的许可页上，现已被 25H2/26H1 取代，但
  fwlink 仍有效）
- fwlink（24H2 EWDK，VS BuildTools
  17.8.6）：`https://go.microsoft.com/fwlink/?linkid=2271957`
- 解析后的直链：
  `https://software-static.download.prss.microsoft.com/dbazure/888969d5-f34g-4e03-ac9d-1f9786c66749/EWDK_ge_release_26100_240331-1435.iso`
- host 下载（该域名 TLS 正常；服务器支持 `Accept-Ranges`，可断点续传）：
  ```sh
  curl -L -C - -o ~/data/hack/iso/EWDK_ge_release_26100_240331-1435.iso \
    'https://go.microsoft.com/fwlink/?linkid=2271957'
  ```

### 2. WinFSP 2.0

- `https://github.com/winfsp/winfsp/releases/download/v2.0/winfsp-2.0.23075.msi`
- host 下载：
  ```sh
  curl -L -o ~/data/hack/iso/winfsp-2.0.23075.msi \
    'https://github.com/winfsp/winfsp/releases/download/v2.0/winfsp-2.0.23075.msi'
  ```

### 3. CPDK（Windows Cryptographic Provider Development Kit，web 安装器）

- 实际版本："Windows Cryptographic Provider Development Kit - Windows
  10"，bundle 10.1.19041.953（安装日志实测）
- 直链（bootstrapper 本体）：
  `https://download.microsoft.com/download/1/7/6/176909B0-50F2-4DF3-B29B-830A17EA7E38/CPDK_RELEASE_UPDATE/cpdksetup.exe`
- 对应的 fwlink
  下载根：`http://go.microsoft.com/fwlink/?LinkId=258675`（目录根，安装器在其下拼
  `Installers/*.msi`、`*.cab` 等子路径；浏览器直接访问它 404
  属正常，要用上面的完整直链）
- 坑：host 的 curl/wget 访问 `download.microsoft.com` 会 TLS 验证失败（curl exit
  60，缺 intermediate 证书）。解决办法：在 Windows guest 内下载（Windows
  证书链处理会自动走 AIA 补全中间证书），再传回 host：
  ```powershell
  # guest 内
  Invoke-WebRequest -Uri 'https://download.microsoft.com/download/1/7/6/176909B0-50F2-4DF3-B29B-830A17EA7E38/CPDK_RELEASE_UPDATE/cpdksetup.exe' -OutFile C:\Users\97936\cpdksetup.exe
  ```
  ```sh
  # host 取回
  scp -P 51104 97936@localhost:cpdksetup.exe ~/data/hack/iso/
  ```

### 4. 源码

```sh
git clone https://github.com/virtio-win/kvm-guest-drivers-windows ~/data/kvm-guest-drivers-windows
```

## guest 内布局

- `C:\build\kvm-guest-drivers-windows`：源码树（构建产物在各驱动
  `Install\Win11\amd64`）
- `C:\build\build-main.log`：构建日志
- `C:\build\netkvm-install.log`：netkvm 安装日志
- `C:\Users\97936\*.ps1`：曾用过的辅助脚本

## 完整步骤（可复现）

1. 传输源码与依赖进 guest（SSH 端口 51104）：
   - host:
     `tar -C ~/data --exclude=.git -czf /tmp/virtio-src.tar.gz kvm-guest-drivers-windows`
   - `scp -P 51104 /tmp/virtio-src.tar.gz ~/data/hack/iso/winfsp-2.0.23075.msi 97936@localhost:`
   - guest: `tar -xzf C:\Users\97936\virtio-src.tar.gz -C C:\build`
2. QMP 热挂载 EWDK ISO（socket：`<vm>/s/qmp-no-pretty`，先
   `qmp_capabilities`）：
   - `blockdev-add` file 节点（read-only）→ `blockdev-add` raw 节点 →
     `device_add scsi-cd,bus=scsi1.0,scsi-id=20`
   - guest 里出现盘符（本次为 D:）
3. 安装 WinFSP（`msiexec /i ... /qn ADDLOCAL=ALL`）和
   CPDK（`cpdksetup.exe /q /norestart`），确认 `bcrypt_provider.h` 存在
4. 构建（写成 CRLF 行尾的 .cmd，用 SYSTEM 计划任务跑，避免 SSH
   断连/会话结束杀进程）：
   ```bat
   set VIRTIO_WIN_NO_ARM=Yes
   set VIRTIO_WIN_NO_CAB_CREATION=Yes
   set EWDK11_24H2_DIR=D:\
   cd /d C:\build\kvm-guest-drivers-windows
   call build_AllNoSdv.bat Win11 amd64
   ```
   - 不传 `amd64` 会连 x86 一起编（sln 的 x86 前置库除外，那是必需的）
   - 老式 INF 驱动（pciserial/fwcfg/Q35）会因找不到 `inf2cat` 报错但不影响主产物
   - 本次总耗时约 2 分钟（增量），全量约 30-60 分钟
5. 签名 + 导入证书（PowerShell 直接做，不需要 EWDK 环境；signtool
   按绝对路径调）：
   - `signtool` =
     `D:\Program Files\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe`（ISO
     挂载期间）
   - `certutil -addstore -f Root build\VirtIOTestCert.cer`
   - `certutil -addstore -f TrustedPublisher build\VirtIOTestCert.cer`
   - 对每个
     `Install\Win11\amd64\*.sys,*.cat`：`signtool sign /fd SHA256 /f build\VirtIOTestCert.pfx <file>`（pfx
     无密码）
6. `bcdedit /set testsigning on`（本机 Secure Boot=False、HVCI 关，可行），重启
7. 安装驱动（顺序重要）：
   - 先装除 NetKVM
     外全部：`pnputil /add-driver <dir>\Install\Win11\amd64\*.inf /install`（在用设备当场更新，网络不受影响）
   - NetKVM 最后装：写成 .cmd 由 SYSTEM 计划任务执行（网卡重启会断
     SSH，任务不受影响），装完 SSH 自动恢复
   - 再重启一次收尾
8. 卸载 ISO：QMP `device_del` → `blockdev-del`

## 关键坑（本次踩过）

1. **master 版本号低于发布版**：master 是 100.6.101.58000，0.1.302 发布版是
   100.103.104.30200。四段版本比较下 master 反而"更老"，所以必须用
   `pnputil /install` 强制指定安装；若只 stage 不 install，PnP
   排名（签名分数优先）会保留 WHQL 旧驱动
2. **SSH 裸命令引号会坏**：`ssh host 'cmd args'` 直接跑 cmd/bcdedit
   参数会被打碎（本次 bcdedit 报"项无效"、shutdown 根本没执行）。所有 guest
   命令一律写 .ps1/.cmd 文件 scp 过去再执行
3. **ps1 不能含中文且无 BOM**：Windows PowerShell 5.1 把无 BOM UTF-8 当 ANSI
   解析，中文会导致语法错误
4. **批处理要 CRLF**：LF 行尾的 .cmd 在 cmd.exe 里行为不可靠（最后一个 echo
   丢失）
5. **PowerShell 布尔要加括号**：`(Test-Path ...) -and (...)`，否则 `-and` 被
   Test-Path 当参数
6. **EWDK 环境链会吞掉外层 .cmd 的后续行**：`call SetVsEnv.bat`
   之后外层脚本的剩余语句不执行（原因未深究，task
   退出码是可信的成功信号）；签名时绕开环境、直接用 signtool 绝对路径即可
7. **CPDK 必须装**：不只 viocrypt，viorng 的 CNG 用户态组件（viorngum）也要
   `bcrypt_provider.h`，缺了整个构建报错退出
8. **tar 不认 `~`**：guest 里用绝对路径 `C:\Users\97936\...`

## 维护与回滚

- 重新构建：重复上面步骤 2-4（源码已在 guest，可 `git pull`——guest 有 git 2.55）
- 重挂 ISO 的 QMP 命令见步骤 2；scsi-id=20 当前空闲
- 卸载某个测试驱动：`pnputil /delete-driver oemNN.inf /uninstall /force`（Published
  Name 用 `pnputil /enum-drivers` 查）
- 恢复正式签名模式：`bcdedit /set testsigning off`
  后重启；注意届时测试签名驱动（版本号又低于正式版）可能需要重新装回旧驱动
- 本机原 0.1.302 驱动包仍在驱动库（oem*.inf 旧条目未删，可作回退）

## 相关文件

- `usb-kbd-power-hang.md`：usb-kbd/usb-tablet 电源挂死问题报告（本 VM
  已按其建议改为 virtio-keyboard/virtio-mouse/virtio-tablet-pci，QEMU 参数中 USB
  设备已全部移除）
- `virtio-win-master-95153a65.tar.gz`：本次构建的驱动产物归档
- `minidumps/`：此前崩溃转储与分析输出

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
