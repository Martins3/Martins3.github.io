# rdp

## IronRDP
- https://github.com/Devolutions/IronRDP
	- https://github.com/winapps-org/winapps 用的是 https://github.com/FreeRDP

2026-06-07

使用 dotfiles 中的 scripts/nix/env/ironrdp.nix ，cargo build ，但是运行有点问题
以后再看吧:
```txt
./target/debug/ironrdp-viewer 192.168.1.100 --username admin  --password secret

Error: os error at /home/martins3/.cargo/registry/src/index.crates.io-1949cf8c6b5b557f/winit-0.30.13/src/platform_impl/linux/mod.rs:765: neither WAYLAND_DISPLAY nor WAYLAND_SOCKET nor DISPLAY is set.
```

## winboat
https://github.com/TibixDev/winboat

他利用虚拟化 加上 FreeRDP 来实现功能
- 虚拟化用的是: https://github.com/dockur/windows

几个极大的优化:
- 支持剪切板拷贝

```txt
/dev/fuse on /tmp/com.freerdp.client.cliprdr.2387701 type fuse (rw,nosuid,nodev,relatime,user_id=1000,group_id=1000)
```

### 在物理机中观察到 smbd 占用了 80% 的 CPU
定位清楚了：80% CPU 来自 WinBoat 容器里的 smbd，不是普通 smbclient。

原因链：

- WinBoat 将整个 $HOME 共享给 Windows：/home/martins3/.winboat/docker-compose.yml:38
- Windows 自动访问 \\host.lan\Data
- Samba 开启了 wide links = yes
- Proton 的 dosdevices/z: 是指向 / 的符号链接
- Windows 递归扫描进入 /dev/fd，形成无限回环路径：
  .../z:/dev/fd/29/fd/29/fd/29/...

- smbd 因此长期占用约 72% CPU，当前持有约 1135 个 FD，累计运行 CPU 时间超过 4 小时。

Windows 侧触发者大概率是 Explorer、Defender 或索引服务；SMB 记录无法直接区分具体 Windows 进程。

推荐根治：不要共享整个 HOME，把 compose 改成专用目录，例如：

- /home/martins3/winboat-share:/shared

然后重建 WinBoat 容器。仅重启容器只能临时恢复，扫描之后还会复发。

另外当前共享是匿名、读写、强制 root，并暴露整个 HOME，安全风险也很高。此次只做了调查，没有修改配置。


## 资源
- https://github.com/miroslavpejic85/p2p
- https://rustdesk.com/zh/
- https://github.com/screego/server
- https://github.com/kunkundi/crossdesk


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
