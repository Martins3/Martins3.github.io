# SystemTap

## 资料

- https://sourceware.org/git/systemtap.git
- https://sourceware.org/systemtap/documentation.html
- https://sourceware.org/systemtap/SystemTap_Beginners_Guide/index.html
- https://sourceware.org/systemtap/tutorial.pdf
- https://github.com/lichuang/awesome-systemtap-cn
- https://access.redhat.com/solutions/358933
- https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/7/html/systemtap_beginners_guide/introduction
- https://sourceware.org/systemtap/documentation.html
  - https://sourceware.org/systemtap/SystemTap_Beginners_Guide/index.html
- https://sourceware.org/systemtap/tutorial.pdf
- https://github.com/lichuang/awesome-systemtap-cn

Ubuntu/Debian 中的 `systemtap-sdt-dev` 只提供给程序加入 SDT 静态探针所需的开发文件，
不等于完整的 SystemTap 工具。

SystemTap 使用 `.stp` 脚本描述探针，默认把脚本翻译并编译成内核模块，然后加载模块收集数据。
它也提供不加载内核模块的 dyninst 用户态后端。

## 安装

Fedora 上安装 SystemTap，并让 `stap-prep` 补齐当前运行内核对应的开发包和调试信息：

```bash
sudo dnf install systemtap elfutils-devel
sudo stap-prep # 安装 kernel-devel 和 kernel-debuginfo 的
stap --version
```

## 基本使用
### 最小测试

默认后端需要加载内核模块，最直接的测试方法是使用 root：

```bash
sudo stap -v -e 'probe begin { println("hello from SystemTap"); exit() }'
```

成功时会经过五个阶段：解析脚本、分析探针、生成 C、编译内核模块、加载并运行。最后应看到：

```text
Pass 5: starting run.
hello from SystemTap
Pass 5: run completed
```

普通用户直接运行默认后端会收到 `You are trying to run systemtap as a normal user`。
除了配置 `stapusr`、`stapsys`、`stapdev` 组，也可以对纯用户态探针选择 dyninst 后端：

```bash
stap --runtime=dyninst \
  -e 'probe begin { println("hello from SystemTap dyninst"); exit() }'
```

本机的 Nix profile 把 Nix GCC 放在 `/usr/bin/gcc` 前面，
而 Nix GCC 默认看不到 `/usr/include/elfutils/libdwfl.h`。
出现该头文件缺失错误时，应让 SystemTap 使用系统编译器：

```bash
command -v gcc
PATH=/usr/bin:/bin stap --runtime=dyninst \
  -e 'probe begin { println("hello from SystemTap dyninst"); exit() }'
```

### 查找探针和上下文变量

`stap -l` 列出匹配的探针，`stap -L` 还会显示该探针可读取的参数：

```bash
sudo stap -l 'syscall.open*'
sudo stap -L 'syscall.openat'
sudo stap -L 'kernel.function("vfs_open")'
```

本机上 `syscall.openat` 可读取 `filename`、`flags`、`mode` 等上下文变量；`vfs_open()` 可读取 `$path` 和 `$file`。

### 跟踪一个命令的 openat

`-c` 启动并跟踪指定命令，`target()` 返回该命令的 PID。下面只记录目标进程的第一次 `openat`：

```bash
sudo stap -e '
probe syscall.openat {
    if (pid() == target()) {
        printf("pid=%d openat(%s)\n", pid(), filename)
        exit()
    }
}' -c '/usr/bin/head -c 1 /etc/hostname'
```

本机实测输出：

```text
pid=3193167 openat("/etc/ld.so.cache")
```

动态链接器通常先打开 `ld.so.cache`，因此第一次调用不一定是命令行指定的文件。去掉 `exit()` 可以继续观察后续调用。

### 读取 dentry 和 inode 引用计数

```bash
sudo stap -e '
probe kernel.function("vfs_open").call {
    if (pid() == target()) {
        printf("comm=%s name=%s dentry_ref=%d inode_ref=%d\n",
               execname(),
               kernel_string($path->dentry->d_name->name),
               $path->dentry->d_lockref->count,
               $path->dentry->d_inode->i_count->counter)
    }
}' -c '/usr/bin/head -c 1 /etc/hostname'
```

本机实测包括：

```text
comm=head name=ld.so.cache dentry_ref=5 inode_ref=1
comm=head name=libc.so.6 dentry_ref=218 inode_ref=1
comm=head name=hostname dentry_ref=1 inode_ref=1
```

注意：

- SystemTap 访问内嵌结构成员时也使用 `->`，例如 `$path->dentry->d_lockref->count`，不能混用 C 的 `.`；
- 这里读取的是进入 `vfs_open()` 那一刻的瞬时值，不是某个进程独占的引用数量；
- dentry 和 inode 的引用计数语义不同，数值不能直接相减或相互推导；
- `kernel.function()` 依赖 DWARF 调试信息，内核函数参数或结构字段无法解析时，先检查当前内核的 debuginfo 是否匹配。


## /usr/share/systemtap/tapset/ 内容解析

这个目录不只由 SystemTap 包维护。其他软件也会把自己的静态探针定义安装进来：

- `qemu-*.stp`：来自 QEMU RPM；
- `libvirt_probes-64.stp`：来自 `libvirt-libs`；
- `libperl5.42.2-64.stp`：来自 `perl-devel`；
- `libglib*.stp`：来自 `glib2-devel`；
- `sssd.stp`：来自 SSSD。


### 为什么每次都报错
```txt
› sudo stap -l 'syscall.open*'

  [sudo] password for martins3:
  parse error: expected literal string or number
          saw: operator '*' at /usr/share/systemtap/tapset/libvirt_probes-64.stp:204:3
       source:   *cert = user_string($arg4);
                 ^

  1 parse error.
  syscall.open
  syscall.open_by_handle_at
  syscall.openat
```

```txt
SystemTap 在第一阶段会解析 /usr/share/systemtap/tapset/ 下所
有 .stp 文件，其中 libvirt 提供的 libvirt.rpc.tls_context_new 探针包含：

*cert = user_string($arg4);
*keys = user_string($arg5);

SystemTap 不接受以 * 开头的变量名，因此报告语法错误。该文件来自：

libvirt-libs-12.0.0-3.fc44.x86_64

RPM 校验表明它没有被本地修改，是当前 Fedora libvirt 包自带的问题。

因为此次查询不需要 libvirt tapset，SystemTap 跳过错误后仍成功列出了 syscall 探针，而且退出码是
0。所以目前可以理解为：

libvirt tapset 有非致命语法错误
+
syscall.open* 查询成功

它不影响我们已经测试的 syscall.openat 和 vfs_open() 探针，只是会污染每次 SystemTap 输出。长期应通
过升级 libvirt-libs 获取修复；临时也可以将其中的 *cert、*keys 改为 cert、keys，但这是修改 RPM 管
理的系统文件。
```


### QEMU tapset

根目录下大量文件来自 QEMU 包，每种 QEMU binary 通常有三份：

```text
qemu-system-x86_64.stp
qemu-system-x86_64-log.stp
qemu-system-x86_64-simpletrace.stp
```

它们都由 QEMU 的 `scripts/tracetool.py` 根据 `trace-events` 自动生成：

- `qemu-system-x86_64.stp` 是基础 tapset，把 ELF 中的 `qemu` SDT marker
  映射成 `qemu.system.x86_64.*` probe alias，同时定义有意义的参数名；
- `qemu-system-x86_64-log.stp` 在基础 alias 上增加 `printf()`，输出人类可读文本；
- `qemu-system-x86_64-simpletrace.stp` 本身是可直接阅读的 SystemTap 文本脚本；
  执行该脚本时会输出 QEMU simpletrace 二进制事件记录，供
  `scripts/simpletrace.py` 离线解码。

例如：

```bash
sudo stap -l 'qemu.system.x86_64.cpu_exec*'
```

可以得到：

```text
qemu.system.x86_64.cpu_exec_end
qemu.system.x86_64.cpu_exec_start
```

不要跟踪没有边界的 `cpu_exec*` 之类高频事件，容易迅速淹没输出或 trace buffer。

QEMU 源码中的对应实现位置是：

- `scripts/tracetool/format/stap.py` 中的 `generate()`：生成基础 tapset；
- `scripts/tracetool/format/log_stap.py` 中的 `generate()`：生成文本 tapset；
- `scripts/tracetool/format/simpletrace_stap.py` 中的 `generate()`：生成输出
  simpletrace 二进制记录的文本 tapset；
- `meson.build` 中 `if stap.found()` 的 SystemTap targets：为每个 QEMU binary
  生成上述文件。

collei `virtme` 使用构建树 QEMU 时的 tapset 生成、实时文本 trace 和 simpletrace
采集方法见 [`collei/systemtap/README.md`](../../collei/systemtap/README.md)。QEMU
本身统一由 `build/sync/sync-qemu.sh` 构建。


### `qemu-trace-stap` 包装器

qemu 辅助工具: 源码在 scripts/qemu-trace-stap

安装 /usr/bin/qemu-trace-stap 中，可以调试

```bash
qemu-trace-stap list qemu-system-x86_64 'qmp_*'
sudo qemu-trace-stap run --pid=PID \
  qemu-system-x86_64 qmp_enter_query_status qmp_exit_query_status
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
