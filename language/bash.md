编写可靠 bash 脚本的一些技巧 - 腾讯技术工程的文章 - 知乎
https://zhuanlan.zhihu.com/p/123989641

https://vedipen.com/2020/linux-bash-vs-windows-powershell/

https://geekgo.tech/archive : 这个人的blog 还挺有意思的

https://gist.github.com/premek/6e70446cfc913d3c929d7cdbfe896fef

https://redandblack.io/blog/2020/bash-prompt-with-updating-time/ : 在bash 中间加入时间 prompt 的功能

https://wangdoc.com/bash/index.html : 简单的教程

https://blog.balthazar-rouberol.com/shell-productivity-tips-and-tricks.html : 高效和 shell 交互

https://www.dolthub.com/blog/2020-03-23-testing-dolt-bats/ : 使用 bash 做集成测试

https://github.com/wfxr/forgit : 功能强大，但是只有 200行，的确值的关注
https://github.com/mydzor/bash2048/blob/master/bash2048.sh : 300行的2048


https://github.com/koalaman/shellcheck : 虽然不能集成到 vim 中间，但是其提供一些错误案例，而且可以静态使用的

https://github.com/alexanderepstein/Bash-Snippets : 可以学习，也可以当做工具
https://notes.pythonic.life/3-Linux/Bash/bash%E7%9A%84%E9%AB%98%E7%BA%A7%E6%8A%80%E5%B7%A7.html : some trick / snippets of bash

> 且称之为三大圣经

https://github.com/jlevy/the-art-of-command-line : **重点** 其实 bash 和 command line 可以分为两个部分讨论
https://github.com/learnbyexample/Command-line-text-processing : 文本处理，终于来了
https://github.com/dylanaraps/pure-bash-bible

https://cjting.me/2020/08/16/shell-init-type/ : 不错不错，讲解 bash 的启动

## 学到了
https://samizdat.dev/help-message-for-shell-scripts/ : 使用 sed 输出 help
  - https://news.ycombinator.com/item?id=23763166 更加优雅的方法

https://stackoverflow.com/questions/669452/is-double-square-brackets-preferable-over-single-square-brackets-in-ba : 为什么条件是 sed


## bash function
1. () 和　{} 的区别是什么 ?

## function parameter
https://unix.stackexchange.com/questions/129072/whats-the-difference-between-and
https://stackoverflow.com/questions/12314451/accessing-bash-command-line-args-vs

- 当没有 quote 的时候，没有什么区别，都是按照空格进行拆分的
- 当 quote :
    - `"$*"` : 将所有的参数作为一个字符串
    - `"$@"` : 将参数，按照 quote 逐个拆分

## 对于数组的循环为什么这么复杂啊 !
> TODO 理解一下每一个符号的含义
for i in "${arrayName[@]}"; do
  echo $i
done

> TODO 而且 if 的功能也是诡异的
> 2 后面的空格不可以省略，否则GG

if [[ $# != 2 ]]
then
  echo usage : test.sh fio file name
fi



https://stackoverflow.com/questions/35296169/bash-empty-string-command
> 含有对于 [[ ]] 的解释




# 常用命令

## basic




## stream pipe and lists
3. 2> standrad error
4. $> merge output and error

https://stackoverflow.com/questions/818255/in-the-shell-what-does-21-mean


## get help
1. whatis
2. tldr
3. cheat.sh
4. apropos 模糊查询man

## 终端上编辑


## process
1. ps top htop
2. gb fg  jobs       cmd &
3. kill
4. shutdown

## media
1. mount – Mount a file system
1. umount – Unmount a file system
1. fsck – Check and repair a file system
1. fdisk – Partition table manipulator
1. mkfs – Create a file system
1. fdformat – Format a floppy disk
1. dd – Write block oriented data directly to a device
1. genisoimage (mkisofs) – Create an ISO 9660 image file
1. wodim (cdrecord) – Write data to optical storage media
1. md5sum – Calculate an MD5 checksum

## network
1. ping - Send an ICMP ECHO\_REQUEST to network hosts
1. traceroute - Print the route packets trace to a network host
1. ip - Show / manipulate routing, devices, policy routing and tunnels
1. netstat - Print network connections, routing tables, interface statistics, masquerade connections, and multicast memberships
1. ftp - Internet file transfer program
1. wget - Non-interactive network downloader
1. ssh - OpenSSH SSH client (remote login program)

## search files
1. locate – Find files by name
1. find – Search for files in a directory hierarchy
1. xargs – Build and execute command lines from standard input
1. stat – Display file or file system status

## archive
1. gzip – Compress or expand files
1. bzip2 – A block sorting file compressor
1. tar – Tape archiving utility
1. zip – Package and compress files
1. rsync – Remote file and directory synchronization


## 还有四章没有看


# bash script
一个应该是简单易懂的教程
https://ryanstutorials.net/bash-scripting-tutorial/bash-variables.php


## snippets
1. change file extension
```
for file in *.html; do
    mv "$file" "$(basename "$file" .html).txt"
done
```

2. 修复强制关机导致的磁盘错误
```
fsck -y /dev/sdb1
```

3. 处理路径的缩写
```
STR="/path/to/foo.cpp"
echo ${STR%.cpp}    # /path/to/foo
echo ${STR##*.}     # cpp (extension)
echo ${STR##*/}     # foo.cpp (basepath)
```

## 资源和工具
1. https://explainshell.com/
2. https://wangchujiang.com/linux-command/
3.

## 具体问题的链接
1. [obscure but useful](https://github.com/jlevy/the-art-of-command-line/blob/master/README.md)
1. https://askubuntu.com/questions/939294/difference-between-let-expr-and
1. https://ss64.com/bash/syntax-execute.html
2. [`$*`和`$@`的区别](https://stackoverflow.com/questions/2761723/what-is-the-difference-between-and-in-shell-scripts)


# 一些链接
1. [Bash hand book](https://github.com/denysdovhan/bash-handbook)
1. [Bash guide](https://github.com/Idnan/bash-guide)
1. [Pure bash bible](https://github.com/dylanaraps/pure-bash-bible)
1. [The art of command line](https://github.com/jlevy/the-art-of-command-line/blob/master/README-zh.md#%E4%BB%85%E9%99%90-os-x-%E7%B3%BB%E7%BB%9F)


# 问题
如下代码段在./lan/bash/a.sh 的形式和直接复制粘贴的方式执行结果不同。
```
< /home/shen/Core/Vn/Readme-Template.md | while read line; do
  echo $line
done
```
# 双引号的作用是什么
echo J8 fuck
echo "J8 fuck"
并没有什么区别

# 各种括号 的作用是什么
{}
()
(())
[[]]
# 命令缩写
su:switch user 切换用户
ps: process status(进程状态，类似于windows的任务管理器) 常用参数：－auxf
ps -auxf 显示进程状态
df: disk free 其功能是显示磁盘可用空间数目信息及空间结点信息。换句话说，就是报告在任何安装的设备或目录中，还剩多少自由的空间。
pwd：Print working directory
su：Swith user
cd：Change directory
ls：List files
ps：Process Status
rmdir：Remove directory
mkfs: Make file system
fsck：File system check
cat: Concatenate
uname: Unix name
df: Disk free
du: Disk usage
fg: Foreground
bg: Background
chown: Change owner
chgrp: Change group
umount: Unmount
dd: 本来应根据其功能描述“Convert an copy”命名为“cc”，但“cc”已经被用以代表“C Complier”，所以命名为“dd”
tar：Tape archive
ldd：List dynamic dependencies
文件结尾的"rc"（如.bashrc、.xinitrc等）：Resource configuration
Knnxxx / Snnxxx（位于rcx.d目录下）：K（Kill）；S(Service)；nn（执行顺序号）；xxx（服务标识）
.a（扩展名a）：Archive，static library
.so（扩展名so）：Shared object，dynamically linked library
.o（扩展名o）：Object file，complied result of C/C++ source file
RPM：Red hat package manager
dpkg：Debian package manager
apt：Advanced package tool（Debian或基于Debian的发行版中提供）

bin = BINaries #下面的是一些二进制程序文件
/dev = DEVices  #下面的是一些硬件驱动
/etc = ETCetera #目录存放着各种系统配置文件, 类似于windows下的system
/lib = LIBrary
/proc = PROCesses
/sbin = Superuser BINaries
/tmp = TeMPorary
/usr = Unix Shared Resources
/var = VARiable ?
/boot=boot #下面的是开机启动文件
FIFO = First In, First Out
GRUB = GRand Unified Bootloader
IFS = Internal Field Seperators
LILO = LInux LOader
PHP = Personal Home Page Tools = PHP Hypertext Preprocessor
PS = Prompt String
Perl = "Pratical Extraction and Report Language" = "Pathologically Eclectic Rubbish Lister"
Python 得名于电视剧Monty Python's Flying Circus
Tcl = Tool Command Language
Tk = ToolKit
VT = Video Terminal
YaST = Yet Another Setup Tool
apache = "a patchy" server
apt = Advanced Packaging Tool
ar = archiver
as = assembler
awk = "Aho Weiberger and Kernighan" 三个作者的姓的第一个字母
bash = Bourne Again SHell
bc = Basic (Better) Calculator
bg = BackGround
biff = 作者Heidi Stettner在U.C.Berkely养的一条狗,喜欢对邮递员汪汪叫。
cal = CALendar
cat = CATenate
cd = Change Directory
chgrp = CHange GRouP
chmod = CHange MODe
chown = CHange OWNer
chsh = CHange SHell
cmp = compare
cobra = Common Object Request Broker Architecture
comm = common
cp = CoPy
cpio = CoPy In and Out
cpp = C Pre Processor
cron = Chronos 希腊文时间
cups = Common Unix Printing System
cvs = Current Version System
daemon = Disk And Execution MONitor
dc = Desk Calculator
dd = Disk Dump
df = Disk Free
diff = DIFFerence
dmesg = diagnostic message
du = Disk Usage
ed = editor
egrep = Extended GREP
elf = Extensible Linking Format
elm = ELectronic Mail
emacs = Editor MACroS
eval = EVALuate
ex = EXtended
exec = EXECute
fd = file descriptors
fg = ForeGround
fgrep = Fixed GREP
fmt = format
fsck = File System ChecK
fstab = FileSystem TABle

fvwm = Fuck Virtual Window Manager

gawk = GNU AWK
gpg = GNU Privacy Guard
groff = GNU troff
hal = Hardware Abstraction Layer
joe = Joe's Own Editor
ksh = Korn SHell
lame = Lame Ain't an MP3 Encoder
lex = LEXical analyser
lisp = LISt Processing = Lots of Irritating Superfluous Parentheses
lpr = Line PRint
lsof = LiSt Open Files
m4 = Macro processor Version 4
man = MANual pages
mawk = Mike Brennan's AWK
mc = Midnight Commander
mkfs = MaKe FileSystem
mknod = MaKe NODe
motd = Message of The Day
mozilla = MOsaic GodZILLa
mtab = Mount TABle
mv = MoVe
nano = Nano's ANOther editor
nawk = New AWK
nl = Number of Lines
nm = names
nohup = No HangUP
nroff = New ROFF
od = Octal Dump
passwd = PASSWorD
pg = pager
pico = PIne's message COmposition editor
pine = "Program for Internet News & Email" = "Pine is not Elm"
pirntcap = PRINTer CAPability
popd = POP Directory
pr = pre
pty = pseudo tty
pushd = PUSH Directory
pwd = Print Working Directory
rc = runcom = run command, rc还是plan9的shell
rev = REVerse
rm = ReMove
rn = Read News
roff = RunOFF
rpm = RPM Package Manager = RedHat Package Manager
rsh, rlogin, rvim中的r = Remote
rxvt = ouR XVT
seamoneky = 我
sed = Stream EDitor
seq = SEQuence
shar = SHell ARchive
slrn = S-Lang rn
ssh = Secure SHell
ssl = Secure Sockets Layer
stty = Set TTY
su = Substitute User
svn = SubVersioN
tar = Tape ARchive
tcsh = TENEX C shell
telnet = TEminaL over Network
termcap = terminal capability
terminfo = terminal information
tex = τέχνη的缩写，希腊文art
tr = traslate
troff = Typesetter new ROFF
tsort = Topological SORT
tty = TeleTypewriter
twm = Tom's Window Manager
tz = TimeZone
udev = Userspace DEV
ulimit = User's LIMIT
umask = User's MASK
uniq = UNIQue
wall = write all
wine = WINE Is Not an Emulator
xargs = eXtended ARGuments
xdm = X Display Manager
xlfd = X Logical Font Description
xmms = X Multimedia System
xrdb = X Resources DataBase
xwd = X Window Dump
yacc = yet another compiler compiler
Fish = the Friendly Interactive SHell
su = Switch User
MIME = Multipurpose Internet Mail Extensions
ECMA = European Computer Manufacturers Association
