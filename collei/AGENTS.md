# Collei 目录

## 核心脚本架构

```
collei/
├── scripts/
│   ├── collei.py          # Python 主入口：编译并执行 QEMU 命令
│   ├── collei-install.py  # Python 安装入口：创建 VM 目录，不启动 QEMU
│   ├── collei-action.py   # Python VM 操作入口
│   ├── collei-global.py   # Python 全局操作入口
│   ├── actions.py         # action/global 共用的配置、运行时和 host 准备模块位于 scripts/ 根目录
│   ├── virtme.py          # virtme 模式：kernel cmdline、initramfs 生成、virtiofsd
│   ├── kernel.py          # 内核构建树辅助：kernel_image / kernel_release（从镜像提取版本）
│   ├── vmtest-init.sh     # guest initramfs 使用的 Bash init
│   └── bash-archive/      # 迁移前 Bash 实现和注释基线，仅作为参考
└── ...
```

Python 迁移架构、兼容边界和检查命令见 `PYTHON.md`。

## Python 代码检查

- 系统已经通过 `~/.nix-profile/bin/ruff` 提供 Ruff，直接运行即可，不需要 `nix-shell -p ruff`。
- 先运行 `ruff check --select I --fix`，再运行 `ruff check --fix`，最后运行 `ruff format`。
- 不使用 Black。
- 使用 Pyright 做类型检查。
- Neovim 的 Python LSP 使用 `ty`。Pyright 检查通过不代表 nvim 中没有类型诊断；
  修改 Python 后还需要从 `collei/scripts` 目录运行 `ty check <files>`，并确保
  nvim 中的 `ty` diagnostics 为 0。`ty.toml` 中的相对搜索路径以当前目录为
  基准；从仓库根目录运行可能把本地 `kernel.py`、`virtme.py` 错认成其他模块。

1. collei.py 定义如何启动已有虚拟机，包括 QEMU 参数、host 准备和迁移目标。
   Python 不会调用或回退到 `bash-archive/collei.sh`；归档 Shell 仅作为迁移参考。

collei-install.py 定义如何创建 VM 目录，包括 ISO、NixOS、vmtest 和 virtme 安装模式。
安装过程只构建 vm_dir 并更新默认 VM symlink，不生成 cmd.sh，也不启动 QEMU。

不同的虚拟机启动有不同的配置参数，他们的配置在 $vm_dir/opt 下

rg check_option collei/scripts/bash-archive/collei.sh 可以知道旧 Bash 实现中一共存在那些配置

永远都不可能直接修改 vm_dir 中的 cmd.sh ，例如
~/data/hack/vm/fake/cmd.sh
cmd.sh 是自动生成的，大多数情况下，都是用于调试的。

2. collei-action.py 中定义如何操作虚拟机

| 选项 | 参数        | 功能                                     | 示例      |
|------|-------------|------------------------------------------|-----------|
| `-a` | `<action>`  | 指定要执行的操作                         | `-a ssh`  |
| `-n` | `<vm_name>` | 直接指定虚拟机名称（用于 AI/自动化） | `-n yyds` |
| `-s` | -           | 交互式选择虚拟机                         | `-s`      |
| `-y` | -           | 自动确认（无需提示）                     | `-y`      |
| `-h` | -           | 显示帮助                                 | `-h`      |

常用命令:
```bash
# 直接指定虚拟机名称进行操作，下面以 yyds 为例
./collei/scripts/collei-action.py -a ssh_auto -n yyds          # 获取到 SSH 到 yyds
./collei/scripts/collei-action.py -a kill -n yyds         # 杀死 yyds
./collei/scripts/collei-action.py -a force_reboot -n yyds         # 快速重启虚拟机，
./collei/scripts/collei-action.py -a run -n test-vm       # 启动 test-vm
./collei/scripts/collei-action.py -a monitor -n yyds-fs      # 连接 yyds 的 monitor
```


## 注意虚拟机的内核是如何构建
- ../build/AGENTS.md 构建内核，而 collei 使用 -kernel -initrd 这种替换 kernel 的方法来构建
- agents/skills/kernel/build-tar/SKILL.md : 快速构建内核安装包，上传到虚拟机中安装测试
- `force_reboot` 只是 guest 内部重启，QEMU 不会从磁盘重新读取 `-kernel` 指向的 bzImage。
  换内核（例如重新编译后）必须 `kill` 再 `run`，否则会出现"旧内核 + 新模块"的错配，
  典型症状是模块加载报 `Unknown symbol`(例如开关 KCSAN 后的 `__tsan_*` 符号缺失)，
  virtio_net 等模块加载失败导致网络不通。

## 如何 SSH 到虚拟机中

**场景**：AI 助手或自动化脚本需要获取 SSH 连接信息，而不是直接执行 SSH 命令。

**解决方案**：使用 `ssh_auto` action，仅输出 SSH 命令而不执行：

例如
```bash
./collei/scripts/collei-action.py -a ssh_auto -n yyds-fs
# 输出:
# ╔═══════╗
# ║yyds-fs║
# ╚═══════╝
# ssh -p 51404 martins3@localhost
# 也就是登录 yyds-fs 虚拟机，可以使用命令 ssh -p 51404 martins3@localhost
```

### virtme 虚拟机走 vsock SSH

virtme 模式的虚拟机（opt/virtme + opt/vsock）默认不配置 guest 网络，
基于 TCP 端口转发的 `ssh_auto` 连不上；正确方式是 vsock SSH，
它不依赖 guest 网络配置：

```bash
./collei/scripts/collei-action.py -a ssh -n virtme            # 直接登录(自动走 vsock)
./collei/scripts/collei-action.py -a ssh_vsock_auto -n virtme # 仅输出命令
# 输出类似:
# ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
#     -o 'ProxyCommand=socat - VSOCK-CONNECT:1096:22' martins3@virtme
```

自动化场景直接在 ssh 后面接命令即可执行 guest 内命令（登录用户是 host 同名
用户，wheel 组，可用 sudo)。不要为了在 guest 里跑脚本而去加 opt/exec、
opt/root_user 之类的临时配置。

## 如何获取虚拟机日志
首先获取到 ssh 到虚拟机的方法，然后使用 ssh ，例如
```sh
ssh -p 51404 martins3@localhost "dmesg"
```

更加好的方法是，因为虚拟机由于宕机，看不到日志:
```sh
./collei/scripts/collei-action.py -a log -n yyds-fs
```

## 如何传递文件到虚拟机中去

一般来说，虚拟机的 ~/data 通过 nfs 共享了物理机的 ~/data
也就说，在物理机构建产生的内核 ko ，无需任何传递，可以直接到虚拟机中 insmod

## 如何克隆虚拟机

```bash
./collei/scripts/collei-action.py -a clone_vm_auto -n <源虚拟机名> <新虚拟机名>
# 示例：从 yyds-fs 克隆 iouring-swap
./collei/scripts/collei-action.py -a clone_vm_auto -n yyds-fs iouring-swap
```

## 开发代码测试
1. 使用 yyds-collei 来进行测试如下内容
	- kill
	- 启动
	- 本地热迁移
2. 正常启动 Win11_24H2_Chinese_Simplified_x64
3. qsr qds 等命令可用
4. 修改了安装相关的代码后，需要安装一个新的虚拟机，完成登录测试，保留虚拟机

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
