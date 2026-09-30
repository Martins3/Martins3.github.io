# bash 核心

## bash 的启动的过程
https://blog.flowblok.id.au/2013-02/shell-startup-scripts.html

https://tiswww.case.edu/php/chet/bash/bashref.html

## login shell 是什么?

> [!NOTE]
> 参考神奇海螺的意见，有待验证

因为 Unix 需要区分两件事：**“用户刚进入系统，需要建立工作环境”**，以及 **“用户已经有了工作环境，只是又启动了一个 shell”**。

`login shell` 就是让 shell 知道：**这次启动要按“登录入口”的方式初始化。它不是负责验证密码的 shell，也不是权限更高的 shell。** 在 Bash 中，它主要影响启动文件的选择。:chatgpt-content-reference{index="0"}

### 1. 从传统的终端登录过程看

传统文本终端的登录流程可以简化为：

```text
agetty
   ↓ 准备终端、读取用户名
login
   ↓ 完成认证、设置用户身份和基本环境
bash（以 login shell 模式启动）
   ↓ 执行登录初始化文件
用户开始运行命令
```

这里分工很明确：`agetty` 处理终端并调用 `login`；`login` 完成登录相关工作，设置 UID/GID 以及 `HOME`、`USER`、`SHELL`、`PATH` 等基本环境，然后启动用户的 shell。**密码验证不是 Bash 的工作。**:chatgpt-content-reference{index="1"}

但系统提供的基本环境不一定满足用户需求。例如，用户希望使用自己的工具目录、默认编辑器等，就需要进一步执行个人初始化配置。登录 shell 为这一步提供了入口。:chatgpt-content-reference{index="2"}

### 2. 为什么不让每一个 shell 都执行完整的登录初始化？

关键在于：**环境变量可以沿进程树继承，不需要每启动一个程序就从头建立一次。** `fork()` 复制父进程的环境，`execve()` 把指定的环境传给新程序；shell 通常会把已 `export` 的变量传给它启动的程序。:chatgpt-content-reference{index="3"}

所以，理想的组织方式是：

```text
登录 shell：建立基础工作环境
    │
    ├── 编译器：继承环境
    ├── 编辑器：继承环境
    └── 新启动的 bash：继承环境，而不是重新建立一遍
```

举个例子，你正在使用一套临时工具链：

```bash
export PATH="/opt/my-toolchain/bin:$PATH"
bash
```

你期待新 Bash 继续使用这个 `PATH`。假设每次启动 Bash 都执行一份“把 `PATH` 重置成默认值”的登录配置，那么刚才的工具链设置就会丢失。另一种情况是配置不断执行：

```bash
export PATH="$HOME/bin:$PATH"
```

嵌套启动几次 shell，就可能重复添加同一个目录。这些都是“重复初始化”和“继承现有环境”混在一起后可能产生的问题。

不过，**另一类配置又确实需要每个新 shell 各自建立**，例如别名、命令补全和交互设置。别名不是普通环境变量，不会因为父 shell 定义过，就自动成为新启动 Bash 的别名。:chatgpt-content-reference{index="4"}

因此，真正需要分开的两层是：

> **登录初始化：建立可供后续进程继承的基础环境。**
> **交互初始化：为当前这个 shell 配置交互行为。**

这也是理解 `profile` 和 `rc` 类配置文件分工的关键。

### 3. 在 Bash 中，这体现为不同的启动文件

以 Bash 的默认行为为例：

```text
交互式 login shell：
    /etc/profile
    然后按顺序查找：
        ~/.bash_profile
        ~/.bash_login
        ~/.profile
    三个用户文件只读取第一个存在且可读的

交互式 non-login shell：
    ~/.bashrc
```

显式使用 `bash --login` 启动的非交互式 shell，也会读取登录初始化文件。:chatgpt-content-reference{index="5"}

一个交互式登录 shell 显然也需要别名、补全等设置，所以常见做法是在 `~/.bash_profile` 中显式加载 `~/.bashrc`：

```bash
# ~/.bash_profile

# 此处放登录阶段的初始化……

# 同时加载交互配置
if [[ $- == *i* && -r ~/.bashrc ]]; then
    . ~/.bashrc
fi
```

**不是 Bash 在登录时必然自动读取 `.bashrc`，而是登录配置文件可以主动读取它。**:chatgpt-content-reference{index="6"}

另外，“登录初始化”不表示系统保证它只执行一次。你反复运行 `bash -l`，它就可能反复执行。因此，初始化代码仍然应该尽量避免重复执行造成副作用。

### 4. shell 怎么知道自己是不是 login shell？

它不需要检查“用户刚才有没有输入密码”，而是由启动者告诉它。

Bash 支持的方式是：启动时 `argv[0]` 以 `-` 开头，或者指定 `-l` / `--login` 选项。:chatgpt-content-reference{index="7"}

例如，完成认证和身份设置后，启动程序可以采用这样的调用形式：

```c
/* 启动方式示意 */
char *argv[] = { "-bash", NULL };
execve("/bin/bash", argv, envp);
```

执行的仍然是 `/bin/bash`，只是 Bash 看到 `argv[0]` 为 `-bash`，就按登录 shell 处理。用户也可以直接运行：

```bash
bash --login
```

因此，**这是用户态程序之间的启动约定，不是内核给进程授予了某种“已登录”权限。**

这里也能看出 `su user` 与 `su - user` 的区别：在 util-linux 的实现中，前者默认保留当前目录和较多原有环境；后者会重建主要环境变量、切换到目标用户主目录，并把 shell 标记为 login shell。这些环境清理和目录切换工作是 **`su` 做的**，不能全部归功于 shell 的 login 标志。:chatgpt-content-reference{index="8"}

### 5. login 与 interactive 是两个独立维度

**login 回答的是“按哪种入口初始化”，interactive 回答的是“是否作为交互式命令解释器运行”。** 两者并不等价。:chatgpt-content-reference{index="9"}

| 启动方式 | Login？ | Interactive？ |
|---|---:|---:|
| `bash -il` | 是 | 是 |
| `bash -i` | 否 | 是 |
| `bash -lc 'echo hello'` | 是 | 否 |
| `bash -c 'echo hello'` | 否 | 否 |

所以，不能通过“有没有终端窗口”“有没有提示符”来判断是不是 login shell。终端模拟器也可以选择以 login 或 non-login 模式启动 shell，例如 GNOME Terminal 就提供了这一配置项。:chatgpt-content-reference{index="10"}

在 Bash 中可以分别检查：

```bash
# 是否为 login shell：显示 on 或 off
shopt login_shell

# 是否为交互式 shell：查看是否包含 i
echo "$-"
```

这两个状态有独立的判断方法。:chatgpt-content-reference{index="11"}

**归根结底，login shell 解决的是“初始化边界”的问题：什么时候应该建立登录环境，什么时候应该继承现有环境、只初始化当前 shell 自己的行为。**

## 另一种表述

历史上，TTY 登录、普通 SSH 登录通常会启动 login shell；在已有终端里再输入 bash 或 zsh，通常会启动 non-login shell。具体是否为 login
shell，由启动方式决定。以 Bash 为例，传入 -l / --login，或者让 argv[0] 以 - 开头，都会启用登录模式。因此，执行 bash -l 就能启动 login
shell，无须重新输入用户名和密码。Bash 官方手册：Invocation (https://www.gnu.org/software/bash/manual/html_node/Invoking-Bash.html)

login 和 interactive 是两个独立的属性。 interactive 决定是否启用提示符、行编辑等交互行为，两者有四种组合：

 类型                 Bash 示例
━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 交互、login          bash -il
───────────────────  ──────────────────────────────────────
 交互、non-login      bash -i
───────────────────  ──────────────────────────────────────
 非交互、login        bash -lc 'echo hello'
───────────────────  ──────────────────────────────────────
 非交互、non-login    bash -c 'echo hello'、bash script.sh

这里 -l 表示登录模式，-i 强制交互模式，-c 指定要执行的命令。Bash 官方手册 (https://www.gnu.org/software/bash/manual/html_node/Invoking-Bash.html)

为什么要区分？传统上有两类初始化需求：

• 会话环境：例如 export PATH=...、export LANG=...，放在 profile 文件中。导出的环境变量会被子进程继承，不必每启动一个子 shell 都重新设置。
• 每个交互 shell 的配置：例如 alias、补全、提示符，放在 rc 文件中。新启动的 shell 需要重新加载这些配置。

例如，父 shell 已执行 export FOO=bar，随后启动的 non-login shell 即使没有读取 profile，也能继承 FOO=bar。所以，没有读取登录配置，不等于没有登录环境。

Bash 的常规启动规则是：

 启动方式          读取的配置
━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 login             /etc/profile，然后在 ~/.bash_profile、~/.bash_login、~/.profile 中按顺序读取第一个存在且可读的文件
────────────────  ────────────────────────────────────────────────────────────────────────────────────────────────────
 交互 non-login    ~/.bashrc
────────────────  ────────────────────────────────────────────────────────────────────────────────────────────────────
 普通非交互脚本    如果设置了 BASH_ENV，读取它指定的文件

Bash login shell 不会自动读取 ~/.bashrc，所以常见做法是在 ~/.bash_profile 中主动加载它。远程调用、sh 兼容模式等还有额外规则。Bash 官方手册：Startup
Files (https://www.gnu.org/software/bash/manual/html_node/Bash-Startup-Files.html)

你使用的 Zsh 则稍有不同：用户配置通常按 .zshenv → .zprofile → .zshrc → .zlogin 的顺序处理；其中 .zshenv 用于所有启动，.zprofile 和 .zlogin 用于 login
shell，.zshrc 用于交互 shell。因此，交互 login Zsh 也会自动读取 .zshrc。Zsh 官方手册：Startup/Shutdown Files
(https://zsh.sourceforge.io/Doc/Release/Files.html#Startup_002fShutdown-Files)

可以直接检查当前 shell：

```txt
# 在 Bash 中
shopt login_shell   # on 表示 login，off 表示 non-login
echo "$-"           # 包含 i 表示 interactive

# 在 Zsh 中
echo "login=${options[login]}, interactive=${options[interactive]}"
```

你链接里用 echo "$0" 判断的方法有局限：bash -l 的 $0 仍然可以是 bash，没有前导 -。用 shell 自己的状态判断更可靠。

## login shell 和非 login shell 是什么区别?
https://unix.stackexchange.com/questions/38175/difference-between-login-shell-and-non-login-shell

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
