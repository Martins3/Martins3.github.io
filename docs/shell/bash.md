# 如何彻底征服 bash script

![](https://preview.redd.it/8a7tpszpdgj41.png?width=640&height=360&crop=smart&auto=webp&s=04e05726a9bb67ff47a8599101931409953859a0)

## 我很忙，不想听这么多废话


## 背景
大一升大二的暑假中，一个大佬帮我安装了 Linux，之后的几周时间逐渐熟悉并喜欢上了 shell 的使用，
但是我最近发现自己的对于 shell 下水平还是停留在那个暑假，干什么都需要 stackoverflow 一下，
一个问题总是在重复的查询，写一个 100 行的 bash script 需要一个上午。这种状况真的让人暴跳如雷。

我的之前的对于 bash 错误认识:
1. bash 不重要。人的手动操作是不可靠的，不可复现的，难以快速复制给其他人的，对于大多数人来说，批处理脚本非常重要，尤其是你打算写几十年的代码的时候。
2. bash 自然而然就会掌握，无需额外的花时间掌握。至少对于我来说，不是，必须刻意学习才可以。
3. 不要使用 Python C 语言和 bash 类比。

## bash script 的设计思想
对于 shell 编程，大致可以分为两个部分:
- 具体的工具，例如 git ipcs 之类的
- bash script

一个工具做好一件事情，而 bash 将工具粘合起来，高效的完成各种事情。

bash 的设计思想:
-  bash 处理的是各种命令的组合，而且需要足够简洁
  - 命令的输入输出都是 string
    - bash 中几乎没有数据类型，任何内容都是 string
    - 对于数值需要特殊的语法和命令
      - `[[ -eq ]]`，`$(())` 以及 `bc`
  - bash 将字符串处理的工作都交给 awk, sed，grep 和 cut 了
- bash 需要足够简洁
  - 利用 pipe
  - 没有异常处理
  - 没有面向对象
  - 没有结构体
  - 数据结构支持优先，只有 array 和 associated array
  - 使用了大量的缩写，例如 !! !$ $!

bash 没有设计出来过多的错误防护机制，几乎没什么人用 bash 写打项目。

## bash 奇怪的地方
局部变量 : 只有函数中的局部变量，但是没有 for 循环中的局部变量

## bash 设计失误
也许是我理解不到位

1. bash 使用 = 来作为相等判断，这导致 bash 的赋值 `=` 两侧不能有空格。

## 1. 打好基础
[Bash 脚本教程](https://wangdoc.com/bash/index.html)

## 问题
1. pstree -p

注意到你可以控制每行参数个数（-L）和最大并行数（-P）

最有用的大概就是 !$， 它用于指代上次键入的参数，而 !! 可以指代上次键入的命令了

## [ ] http://mywiki.wooledge.org/BashPitfalls

1. Filenames with leading dashes
  - cp -- "$file" "$target" : 使用 -- 来处理

2. 不可以同时打开和重定向同一个文件
  - cat file | sed s/foo/bar/ > file ：这个会导致 file 中的内容为空，最简单的是使用一个中间符号来代替。

3. & 本身就是一个结束符号
  - `for i in {1..10}; do ./something &; done` : 将 & 后的 ; 去掉

@todo 从这里继续吧
```txt
34. if [[ $foo = $bar ]] (depending on intent)
```

## [ ] https://mywiki.wooledge.org/BashFAQ

## echo 的额外用法
1. -n 参数可以取消末尾的回车符
2. -e 参数会解释引号（双引号和单引号）里面的特殊字符（比如换行符\n

## 如何输出一个字符串
在 bash 中 \ 会让下一行和上一行放到一起来解释，体会一下下面的两个命令的差别:
```sh
echo "one two
            three"

echo "one two \
             three"

echo one two \
             three

echo one two
             three
```

- https://stackoverflow.com/questions/13335516/how-to-determine-whether-a-string-contains-newlines-by-using-the-grep-command

## 变量
- [indirect expansion](https://unix.stackexchange.com/questions/41292/variable-substitution-with-an-exclamation-mark-in-bash)
```sh
hello_world="value"
# Create the variable name.
var="world"
ref="hello_$var"
# Print the value of the variable name stored in 'hello_$var'.
printf '%s\n' "${!ref}"
```

```sh
var="world"
declare "hello_$var=value"
printf '%s\n' "$hello_world"
```

## 有用的变量

## 一个括号是不是足够逆天
https://unix.stackexchange.com/questions/306111/what-is-the-difference-between-the-bash-operators-vs-vs-vs

## eval 和 exec 的区别
https://unix.stackexchange.com/questions/296838/whats-the-difference-between-eval-and-exec/296852

功能都非常变态:
- eval 相当于执行这个字符串
- exec 将当前的 bash 替换为执行程序

## 常用工具

### awk
### pushd 和 popd
- https://unix.stackexchange.com/questions/77077/how-do-i-use-pushd-and-popd-commands

- 从左边进入
- 最左边的就是当前的目录
- pushd x 会进入到 x 中

在 zsh 中，是自动打开 `setopt autopushd`
https://serverfault.com/questions/35312/unable-to-understand-the-benefit-of-zshs-autopushd 的，
这导致 cd 的行为和 pushd 相同。


### [ ] https://effective-shell.com/part-2-core-skills/job-control/

## 一些资源
- [forgit](https://github.com/wfxr/forgit) A utility tool powered by fzf for using git interactively
- [Bash web server](https://github.com/dzove855/Bash-web-server/) : 只有几百行的 web server 使用 bash 写的 :star:
- [Write a shell in C](https://brennan.io/2015/01/16/write-a-shell-in-c/) : 自己动手写一个 shell
- [Pure bash bible](https://github.com/dylanaraps/pure-bash-bible)

## 一些博客
- [window powershell 和 bash 的对比](https://vedipen.com/2020/linux-bash-vs-windows-powershell/)

## shell 资源推荐
1. https://devhints.io/bash  : 语法清单
2. https://explainshell.com/ : 给出一个 shell 命令，对于其进行解释
3. https://wangchujiang.com/linux-command/
## 一些小技巧
- [alias](https://thorsten-hans.com/5-types-of-zsh-aliases)
- [自动回答交互式的 shell script](https://askubuntu.com/questions/338857/automatically-enter-input-in-command-line)

## zsh 的技巧
- take 创建并且进入目录
- ctrl-x e 进入编辑模式

## 一些库
- [gum](https://github.com/charmbracelet/gum)
- https://github.com/bats-core/bats-core : bash 测试框架

## 冷知识
- [locate vs find](https://unix.stackexchange.com/questions/60205/locate-vs-find-usage-pros-and-cons-of-each-other)
  - locate 只是比 find 更快而已
- 使用 mv /tmp/gafsdfa/fadafsdf{aaa,bb}.png 来实现 rename
- [根据 shell 启动的不同，加载的配置的文件不同](https://cjting.me/2020/08/16/shell-init-type/)
  - 存在 login 和 non-login ，interactive 和 non-interactive 之分
- [Shell 启动类型探究](https://cjting.me/2020/08/16/shell-init-type/) : 不错不错，讲解 bash 的启动

## 获取帮助
1. whatis
2. tldr
3. cheat.sh
4. apropos 模糊查询 man

## 有趣
- https://github.com/mydzor/bash2048/blob/master/bash2048.sh : 300 行的 2048


- 输入 top 10 的命令，但是没看懂
```sh
history | awk '{CMD[$2]++;count++;}END { for (a in CMD)print CMD[a] " " CMD[a]/count*100 "% " a;}' | grep -v "./" | column -c3 -s " " -t | sort -nr | nl |  head -n10
```

## shellcheck 也是有问题的
1. common.sh 可以解决一下吗？

## [ ] 整理一下 coprocess 的内容

## glob 中和字符串当时是使用的正则表达式吧

1. glob 中是否支持 [a,b]*
2. glob 只能处理文件是吗？
  3. 字符串比较的时候也是可以的

## 任何时候都不要使用 [
https://stackoverflow.com/questions/3427872/whats-the-difference-between-and-in-bash

## 整理下如下内容
- bash 的替代品: https://github.com/oilshell/oil/wiki/Alternative-Shells

真的见过无数个 bash 最佳实践的文章，说实话，为什么一个语言如此放纵非最佳实践。

## shellcheck 让我必须将所有的变量全部使用双引号包含进来
- http://www.oilshell.org/release/latest/doc/idioms.html#new-long-flags-on-the-read-builtin

## 迷茫啊，这个会因为双引号警告的
```txt
	if [[ ! $i =~ ".*debug.*" ]]; then
		rpm_extract $i
	fi
```

## 高级话题
### 如何理解这个
```sh
bash <(curl -L zellij.dev/launch) 这个命令如何理解？
```

## grep 和 egrep 的差别
- https://stackoverflow.com/questions/18058875/difference-between-egrep-and-grep
  - [ ]  对于 regex 的理解又成为了问题。
- https://unix.stackexchange.com/questions/17949/what-is-the-difference-between-grep-egrep-and-fgrep
  - 还有 fgrep

## 等于号叫我再次做人
```c
if ! [[ $number =~ $re ]] ; then
  echo "not a number"
fi
```
可以写成这个吗？
```c
if [[ ! $number =~ $re ]] ; then
  echo "not a number"
fi
```

多个链接到一起，如何
```c
if [[ ! $number =~ $re ]] ; then
  echo "not a number"
fi
```

## 整理一下
  - 为什么这里必须存在一个双引号！

## 实际上，我们发现 bash 的一个 philosophy
- 很多常用命令不是熟练使用，还是痛苦面具
  - https://dashdash.io/


## 一个 AWK 痛苦面具问题
```sh
set -E -e -u -o pipefail
grep vendor_id /proc/cpuinfo | awk 'NR==1{print $0; exit}'
grep flags /proc/cpuinfo | awk 'NR==1{print $0; exit}'
```
我发现第一个 awk 不会让 exit non-zero，而第二个会。


## 重点分析下 find 命令
### 如何 flatten 一个目录
- https://unix.stackexchange.com/questions/52814/flattening-a-nested-directory
- find . -type f -exec ls '{}' +
  - 所有的参数一次执行
- find . -type f -exec ls '{}' \;
  - 对于每一个文件，分别 fork 出来执行
- find . -type f -execdir ls '{}' +
  - 切换到对应的目录执行

```txt
       -exec command ;
              Execute command; true if 0 status is returned.  All following arguments to find are taken to be arguments  to
              the  command  until an argument consisting of `;' is encountered.  The string `{}' is replaced by the current
              file name being processed everywhere it occurs in the arguments to the command, not just in  arguments  where
              it  is alone, as in some versions of find.  Both of these constructions might need to be escaped (with a `\')
              or quoted to protect them from expansion by the shell.  See the EXAMPLES section for examples of the  use  of
              the  -exec  option.  The specified command is run once for each matched file.  The command is executed in the
              starting directory.  There are unavoidable security problems surrounding use of the -exec action; you  should
              use the -execdir option instead.

       -exec command {} +
              This  variant  of  the -exec action runs the specified command on the selected files, but the command line is
              built by appending each selected file name at the end; the total number of invocations of the command will be
              much less than the number of matched files.  The command line is built in much the same way that xargs builds
              its command lines.  Only one instance of `{}' is allowed within the command, and it must appear at  the  end,
              immediately  before  the `+'; it needs to be escaped (with a `\') or quoted to protect it from interpretation
              by the shell.  The command is executed in the starting directory.  If any invocation with the  `+'  form  re‐
              turns  a  non-zero value as exit status, then find returns a non-zero exit status.  If find encounters an er‐
              ror, this can sometimes cause an immediate exit, so some pending commands may not be run at  all.   For  this
              reason  -exec my-command ... {} + -quit  may  not  result  in my-command actually being run.  This variant of
              -exec always returns true.
```

## 神奇的双引号
如果是在 ${} 和 $() 中，是可以继续使用双引号的

## https://stackoverflow.com/questions/7442417/how-to-sort-an-array-in-bash

## dirname 和 basename

## 文件处理

了解如何使用 sort 和 uniq，包括 uniq 的 -u 参数和 -d 参数，具体内容在后文单行脚本节中。另外可以了解一下 comm。

了解如何使用 cut，paste 和 join 来更改文件。很多人都会使用 cut，但遗忘了 join。

了解如何运用 wc 去计算新行数（-l），字符数（-m），单词数（-w）以及字节数（-c）。

了解 sort 的参数。显示数字时，使用 -n 或者 -h 来显示更易读的数（例如 du -h 的输出）。明白排序时关键字的工作原理（-t 和 -k）。例如，注意到你需要 -k1，1 来仅按第一个域来排序，而 -k1 意味着按整行排序。稳定排序（sort -s）在某些情况下很有用。例如，以第二个域为主关键字，第一个域为次关键字进行排序，你可以使用 sort -k1，1 | sort -s -k2，2。

## awk

```bash
awk '{ x += $3 } END { print x }' myfile

egrep -o 'acct_id=[0-9]+' access.log | cut -d= -f2 | sort | uniq -c | sort -rn
```


## 总结 job control
- hohup command &
- fg
- bg

## ~ 和 $HOME 的区别
https://askubuntu.com/questions/1177464/difference-between-home-and

- $HOME is an environment variable
- ~ is a shell expansion symbol

让我们回忆一下:

> 这种特殊字符的扩展，称为模式扩展（globbing）。其中有些用到通配符，又称为通配符扩展（wildcard expansion）。Bash 一共提供八种扩展。

启动第一个扩展就是波浪线扩展。

所以，区别就是:
arg_virtio="-drive aio=native,,file=~/hack/iso/virtio-win-0.1.208.iso,media=cdrom,index=2"
中的 ~ 是无法被展开的。

所以，你不能在 C 语言中使用
```c
int fd = open("~/abc.txt", O_RDWR | O_CREAT, 0644);
```

## bash 一个复杂的原因

有些功能是 bash 内置的，
但是外部工具也可以做的。

例如: https://stackoverflow.com/questions/13210880/replace-one-substring-for-another-string-in-shell-script

但是，实际上，也可以用 sed

## https://github.com/johnkerl/miller

## subshell

```txt
COMMAND EXECUTION ENVIRONMENT
       The shell has an execution environment, which consists of the following:

       •      open files inherited by the shell at invocation, as modified by redirections supplied to the exec builtin

       •      the current working directory as set by cd, pushd, or popd, or inherited by the shell at invocation

       •      the file creation mode mask as set by umask or inherited from the shell's parent

       •      current traps set by trap

       •      shell parameters that are set by variable assignment or with set or inherited from the shell's parent in the environment

       •      shell functions defined during execution or inherited from the shell's parent in the environment

       •      options enabled at invocation (either by default or with command-line arguments) or by set

       •      options enabled by shopt

       •      shell aliases defined with alias

       •      various process IDs, including those of background jobs, the value of $$, and the value of PPID

       When a simple command other than a builtin or shell function is to be executed, it is invoked in a separate execution environment that consists of the following.  Unless otherwise noted, the values are inherited from the shell.

       •      the shell's open files, plus any modifications and additions specified by redirections to the command

       •      the current working directory

       •      the file creation mode mask

       •      shell variables and functions marked for export, along with variables exported for the command, passed in the environment

       •      traps caught by the shell are reset to the values inherited from the shell's parent, and traps ignored by the shell are ignored

       A command invoked in this separate environment cannot affect the shell's execution environment.

       Command  substitution,  commands  grouped with parentheses, and asynchronous commands are invoked in a subshell environment that is a duplicate of the shell environment, except that traps caught by the shell are reset to the values that the
       shell inherited from its parent at invocation.  Builtin commands that are invoked as part of a pipeline are also executed in a subshell environment.  Changes made to the subshell environment cannot affect the shell's execution environment.

       Subshells spawned to execute command substitutions inherit the value of the -e option from the parent shell.  When not in posix mode, bash clears the -e option in such subshells.

       If a command is followed by a & and job control is not active, the default standard input for the command is the empty file /dev/null.  Otherwise, the invoked command inherits the file descriptors of the calling shell as modified  by  redi‐
       rections.

```

```sh
set -e
function b() {
  cat /abc
  cat /abc
}

function a() {
  cat abc || true

  b # 如果直接调用 b ，那么 b 中第一个就失败
  a=$(b) # 但是如果是这种调用方法，b 中失败可以继续
}

a
```
## 看看这个
https://www.panix.com/~elflord/unix/grep.html#why

## 一个符号，多种场景语义不同的

https://unix.stackexchange.com/questions/47584/in-a-bash-script-using-the-conditional-or-in-an-if-statement

```txt
if [ "$fname" = "a.txt" ] || [ "$fname" = "c.txt" ]
```

但是 `ls a || ls` 中的 `||` 则是表示第一个命令失败，那么执行第二个。


## bash 中没有 bool
但是存在 true 和 false command

但是 a=true 这个时候 true 是 string 而已
https://stackoverflow.com/questions/2953646/how-can-i-declare-and-use-boolean-variables-in-a-shell-script


## glob 自动失败的是偶

```sh
QEMU_PID_DIR="/var/run/libvirt/qemu"

for i in "$QEMU_PID_DIR"/*.pid; do
  echo $i
done
```
如果一个文件都没有，echo $i 得到
/var/run/libvirt/qemu/*.pid


## glob 语法和 regex 的差别让人真的很烦
```sh
disk=nvme0n1
disk=${disk%%[0-9]*}
echo $disk # 得到的居然是 nvme ，因为 * 表示拼配任何字符
```

## TODO : nvim/snippets/sh.snippets 中的 note_cmpstring 需要重写下

## 看看这个 blog ，深入理解下 shell
https://a-wing.top/shell/2021/05/01/sh-compatibles-history : 一共三篇

## 哈哈，的确，bash 只是适合原型验证而已
https://benjamincongdon.me/blog/2023/10/29/Avoid-Load-bearing-Shell-Scripts/

https://iridakos.com/programming/2018/03/01/bash-programmable-completion-tutorial

https://github.com/phyver/GameShell

https://www.madebygps.com/an-intro-to-finding-things-in-linux/

https://github.com/riquito/tuc

https://github.com/onceupon/Bash-Oneliner

https://questions.wizardzines.com/unix-permissions.html

https://www.noulakaz.net/2007/03/18/a-regular-expression-to-check-for-prime-numbers/

https://zimfw.sh/

https://github.com/barthr/redo

https://github.com/chmln/sd

https://stackoverflow.com/questions/1371261/get-current-directory-or-folder-name-without-the-full-path

https://stackoverflow.com/questions/15622328/how-to-grep-a-string-in-a-directory-and-all-its-subdirectories

https://unix.stackexchange.com/questions/29878/can-i-access-nth-line-number-of-standard-output

https://stackoverflow.com/questions/22190902/cut-or-awk-command-to-print-first-field-of-first-row

https://unix.stackexchange.com/questions/65932/how-to-get-the-first-word-of-a-string

https://unix.stackexchange.com/questions/437680/set-data-structure-equivalent-in-bash-shell

https://stackoverflow.com/questions/61811402/remove-all-elements-from-associative-array-bash

## grep -q 和 pipefail 不可以使用
https://stackoverflow.com/questions/76750683/bash-pipe-to-grep-in-if-statement-return-value
  - 这也让人感到太恐惧了吧，pipefail 配合 grep -q 这么搞，谁顶得住

grep -q 获得到正确的数据之后就会退出，导致关闭了管道的读端，这时候 lsmod 再往管道里面写，
就会触发信号 SIGPIPE ，由于 pipefail 的设置，会导致
lsmod | grep -q cpuid 的整个返回值为 141

复现方法:

搞一个 cat a_long_file | grep -q $first_word_of_long_file 这种就可以测试出来

```sh
VAR=10000
for ((i = 0; i < VAR; i = i + 1)); do
	printf '%s\n' "$i"
	if cat /home/martins3/.dotfiles/nvim/10k.txt | grep -q abandoned ; then
		echo ""
	else
		echo "found"
		exit 1
	fi
done
```

## bash_unit
- https://github.com/pgrange/bash_unit

- 各种单元测试工具对比分析
- https://github.com/dodie/testing-in-bash?tab=readme-ov-file

非常好用，我建议将不确定的语法放到旁边的测试文件中

## 尝试理解下 coproc 吧
- https://stackoverflow.com/questions/47074232/bash-script-stopping-a-process-running-forever-when-a-line-is-printed

1. 似乎不能连续启动两个 coproc
2. coproc a (ls > a) 是没有重定向的

## 尝试使用下这个文件
https://github.com/pgrange/bash_unit?tab=readme-ov-file

## 其他的替代
https://google.github.io/zx/

## 有趣的工具
https://terminaltrove.com/list/


## sed 太魔幻了

```bash
echo $v | sed -E 's/.*\.apple\.([0-9]+)\..*/\1/'
```

### sed 中数值，到底是 \d 还是 `[[:digit:]]`
https://superuser.com/questions/513412/how-to-match-digits-followed-by-a-dot-using-sed

## 由来一个
https://github.com/Ph0enixKM/Amber

## 这个 regex !!!
```sh
#!/usr/bin/env bash
set -E -e -u -o pipefail

test_1() {
	a="a           "
	a="a "
	a="a"

	if [[ $a =~ a\s* ]]; then
		echo "🐈"
	fi
}

test_2() {
	a="a           ="
	a="a ="
	a="a =" # 不可以
	a="a=" # 可以

	if [[ $a =~ a\s*= ]]; then
		echo "🐈"
	fi
}

test_2
```

所以，到底 'a = ' 以及 'a   =        ' 如何匹配

## 看看
https://dhashe.com/xargs-is-the-inverse-function-of-echo.html

## 记录
https://stackoverflow.com/questions/15559359/insert-line-after-match-using-sed

https://unix.stackexchange.com/questions/223897/sed-how-to-remove-all-lines-that-do-not-match

## 这个记录下
https://stackoverflow.com/questions/229551/how-to-check-if-a-string-contains-a-substring-in-bash : 看来记忆是错误的

== 是要求等于，而 =~ 则是 substr ， 对吧

## 收集收集 !
https://github.com/onceupon/Bash-Oneliner

https://news.ycombinator.com/item?id=41097241

## 好的，我知道 ruby 也可以当 shell 用了
https://lucasoshiro.github.io/posts-en/2024-06-17-ruby-shellscript

原来 ruby 的代码是这个样子的啊

## sed
可以可以说，让 sed 总是打开 -E 模式是好的吗?

被坑过了，使用 -e 尝试了好长的时间，最后发现是 -E 才可以
```sh
sed -i -E 's/kernel\.([0-9]+)/v\1/' a
```

这里哪一个是依赖于 -e 的，是 +60d 吗?
```txt
	sed -i -e '/invoked oom-killer/,+60d' "$1"
```

https://omid.dev/2024/06/19/advanced-shell-scripting-techniques-automating-complex-tasks-with-bash/

## 看看这个解决方法
https://docs.amber-lang.com/getting_started/installation

不是 bash 很难记忆，而是很难学习，细微的区别
bash vs python 就像是中文 vs 英语，为了完成

## bash 是一个工具集合
哎，这个可以 check 一下:
https://github.com/me115/linuxtools_rst

## 也许可以自己来配置一下
https://tuzz.tech/blog/how-bash-completion-works

https://blog.izissise.net/posts/env-path/

## 收集
https://stackoverflow.com/questions/918886/how-do-i-split-a-string-on-a-delimiter-in-bash
https://stackoverflow.com/questions/2871181/replacing-some-characters-in-a-string-with-another-character

看看这个:
https://stackoverflow.com/questions/2871181/replacing-some-characters-in-a-string-with-another-character


## grep 的 or 和  glob 的 or 、regex 的 or 都不一样，对吧

https://unix.stackexchange.com/questions/25821/grep-how-to-add-an-or-condition

https://stackoverflow.com/questions/8020848/how-is-the-and-or-operator-represented-as-in-regular-expressions

https://unix.stackexchange.com/questions/50220/using-or-patterns-in-shell-wildcards

## || : 的作用
https://superuser.com/questions/1022374/what-does-mean-in-the-context-of-a-shell-script

## 不敢想，这个有多复杂
https://stackoverflow.com/questions/3236871/how-to-return-a-string-value-from-a-bash-function

## 用 bash 实现 raytracing
https://news.ycombinator.com/item?id=42475703

## 常看常新
https://www.trevorlasn.com/blog/10-essential-terminal-commands-every-developer-should-know

## 看
https://unix.stackexchange.com/questions/32908/how-to-insert-the-content-of-a-file-into-another-file-before-a-pattern-marker

https://stackoverflow.com/questions/23120346/in-sed-how-to-represent-alphanumeric-or-or

https://news.ycombinator.com/item?id=36517525


## 并发执行
```sh
for _ in $(seq 1 10); do
	{
		for _ in $(seq 1 10000); do
			m="$(mktemp /tmp/foo/XXXXXX)"
			touch "$m"
			rm "$m"
		done
	} &
done
wait
```
当然，也许有更好的环境

## 看看这个
https://stackoverflow.com/questions/22497246/insert-multiple-lines-into-a-file-after-specified-pattern-using-shell-script

## 不错
https://github.com/wzb56/13_questions_of_shell


## 技巧
```txt
function generate_config() {
	all_the_config_we_need="defconfig $(printf "martins3.%s.config " "$@")"
}

generate_config base virt fs blk mm net ebpf ftrace rust graphics pci crypto
```
这样可以获取到 martins3.base.config 这种的结果

## 有点无语

`find . -name "initramfs*img"` 使用的是 glob
`rg "a.*"` 使用的是 regex


## 使用空行把文件展开
https://stackoverflow.com/questions/33294986/splitting-large-text-file-on-every-blank-line

```txt
awk -v RS= '{print > ("whatever-" NR ".txt")}' file.txt
```
## 什么是 fnmatch

可以看看这里，想要同时匹配多个 branch 难度非常大:
- https://stackoverflow.com/questions/53135414/how-to-apply-one-github-branch-rule-to-multiple-branches

fnmatch 和 glob 的关系是什么?

## 细致啊
https://www.shellcheck.net/wiki/SC2155

## Xonsh 其实意义不大
https://news.ycombinator.com/item?id=43134563

## 注意 subshell

ls -la $(ls /abc)
里面的错误不会传导出来，最后会得到 ls /abc 的错误，以及执行
ls -la ，那么如果我想要的里面的错误，立刻都结束，怎么办?

## 经典误用
这里 && 一旦失败，会直接 exit 1 ，不只是不去 echo 0 ， echo "good"
```sh
##!/usr/bin/env bash
set -E -e -u -o pipefail

[[ -f b.sh ]] && echo "good"
```

## 想不到 readarray 是需要 bash 版本的
但是只有 mac 上的 bash 不支持


## 等到什么时候把这个搭建一下吧
https://news.ycombinator.com/item?id=43476793
https://docs.atuin.sh/self-hosting/server-setup/

## 有趣的 exec 命令
```sh

echo "a"
ls /proc/self/fd
cat /proc/self/cmdline
exec 100<>/tmp/qemu_img
echo "b"
ls /proc/self/fd
cat /proc/self/cmdline
```


## bash 字符串相等比较，不要使用 regex ，用 glob
<!-- ecbf9c47-8eb9-4061-b7aa-2b5d2af34a10 -->

1. 不要忘记了，regex 是匹配子串就可以了，也就是如果
想要强制从头开始匹配，那么还需要额外的添加 ^
```sh
#!/usr/bin/env bash
set -E -e -u -o pipefail

# 可以匹配
if [[ faffaf =~ af ]]; then
	echo "regex match"
fi

# 不可以匹配
if [[ faffaf == af* ]]; then
	echo "glob bash"
fi

```
2. regex 中的 . 就是匹配一个字符，但是现在很多字符就是有 . 的
```sh
if [[  951493955a4a  =~ 5.4.* ]]; then
	echo "match"
fi
```

所以，就我们的场景，推荐用 glob 。在任何时候，两者不要混用。

## PROGDIR 的两个写法

https://stackoverflow.com/questions/59895/how-do-i-get-the-directory-where-a-bash-script-is-located-from-within-the-script

```sh
PROGDIR=$(readlink -m "$(dirname "\$0")")
PROGDIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
```

配合测试代码:
- docs/shell/progdir-main.sh
- docs/shell/dir/progdir.sh

## 一些库
https://github.com/ko1nksm/getoptions?tab=readme-ov-file#installation


## .bash_profile 和 .bashrc 是什么关系?
```txt
• 问题根因找到了！
  冲突原因：
   文件            配置
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   .bashrc         ANTHROPIC_BASE_URL=https://api.deepseek.com/anthropic
   .bash_profile   先 source ~/.bashrc，然后又覆盖为 ANTHROPIC_BASE_URL=https://co.yes.vg
  因为 .bash_profile 最后执行，所以 DeepSeek 的配置被 co.yes.vg 覆盖了。你在
  另一个环境没问题，很可能那个环境没有加载 .bash_profile（非 login shell），
  所以 DeepSeek 配置保持了生效。

  我来帮你修复 .bash_profile，删掉那两行错误的覆盖：
• Used StrReplaceFile (.bash_profile)
  ╭─  +1 -3 /home/martins3/.bash_profile  ─────────────────────────────────╮
  │  1   # Source /root/.bashrc if user has one                            │
  │  2   [ -f ~/.bashrc ] && . ~/.bashrc                                   │
  │  3                                                                     │
  │  4 - # Claude Code API Router configuration                            │
  │  5 - export ANTHROPIC_BASE_URL="https://co.yes.vg"                     │
  │  6 - export ANTHROPIC_AUTH_TOKEN="cr_12f594839e66bbc7a0c51e3b31c89694… │
  │  4 + # Claude Code uses DeepSeek API (configured in .bashrc)           │
  ╰────────────────────────────────────────────────────────────────────────╯
• 用户说运行 claude 有问题，我用 dpsk 的 api，这个 api 一定没问题。我已经帮
  他找到问题并修复了。现在我需要告诉他修复完成了，并说明如何验证。
• 修复完成！现在 .bash_profile 不会再覆盖 DeepSeek 的 API 配置了。
  验证方法 —— 重新登录或执行：
  source ~/.bash_profile
  env | grep ANTHROPIC_BASE_URL
  应该输出：
  ANTHROPIC_BASE_URL=https://api.deepseek.com/anthropic
  然后再运行 claude 就应该走 DeepSeek 的官方 API 了。

  如果你仍然想保留 co.yes.vg 作为备用方案，可以告诉我，我可以帮你改成通过命
  令行参数切换，而不是让环境变量互相覆盖。
```

## bash todo export 的含义到底是什么?
<!-- 1f23fa8b-7519-4671-88c0-c2d015fe3ea9 -->

这样是错误的
```txt
m]$NORMAL=1
m]$make
```
make NORMAL=1 才可以，makefile 是如何接受

似乎 export NORMAL=1 ，然后 make 也是可以的

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
