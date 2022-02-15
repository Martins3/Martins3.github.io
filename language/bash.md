编写可靠 bash 脚本的一些技巧 - 腾讯技术工程的文章 - 知乎
https://zhuanlan.zhihu.com/p/123989641

https://gist.github.com/premek/6e70446cfc913d3c929d7cdbfe896fef

https://redandblack.io/blog/2020/bash-prompt-with-updating-time/ : 在bash 中间加入时间 prompt 的功能
https://wangdoc.com/bash/index.html : 简单的教程
https://blog.balthazar-rouberol.com/shell-productivity-tips-and-tricks.html : 高效和 shell 交互

https://www.dolthub.com/blog/2020-03-23-testing-dolt-bats/ : 使用 bash 做集成测试

https://github.com/mydzor/bash2048/blob/master/bash2048.sh : 300行的2048
https://notes.pythonic.life/3-Linux/Bash/bash%E7%9A%84%E9%AB%98%E7%BA%A7%E6%8A%80%E5%B7%A7.html : some trick / snippets of bash

> 且称之为三大圣经

https://github.com/learnbyexample/Command-line-text-processing : 文本处理，终于来了
https://github.com/dylanaraps/pure-bash-bible

https://cjting.me/2020/08/16/shell-init-type/ : 不错不错，讲解 bash 的启动

## 学到了
https://samizdat.dev/help-message-for-shell-scripts/ : 使用 sed 输出 help
  - https://news.ycombinator.com/item?id=23763166 更加优雅的方法

https://stackoverflow.com/questions/669452/is-double-square-brackets-preferable-over-single-square-brackets-in-ba : 为什么条件是 sed

## get help
1. whatis
2. tldr
3. cheat.sh
4. apropos 模糊查询man

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

# 命令缩写
su:switch user 切换用户
ps: process status(进程状态，类似于windows的任务管理器) 常用参数：－auxf
fsck：File system check
cat: Concatenate
uname: Unix name
df: Disk free
du: Disk usage
dd: 本来应根据其功能描述“Convert an copy”命名为“cc”，但“cc”已经被用以代表“C Complier”，所以命名为“dd”
tar：Tape archive
ldd：List dynamic dependencies
文件结尾的"rc"（如.bashrc、.xinitrc等）：Resource configuration
.a（扩展名a）：Archive，static library
.so（扩展名so）：Shared object，dynamically linked library
.o（扩展名o）：Object file，complied result of C/C++ source file
RPM：Red hat package manager
dpkg: Debian package manager
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
fifo = first in, first out
grub = grand unified bootloader
ifs = internal field seperators
lilo = linux loader
php = personal home page tools = php hypertext preprocessor
ps = prompt string
perl = "pratical extraction and report language" = "pathologically eclectic rubbish lister"
python 得名于电视剧monty python's flying circus
tcl = tool command language
tk = toolkit
vt = video terminal
yast = yet another setup tool
apache = "a patchy" server
ar = archiver
as = assembler
awk = "aho weiberger and kernighan" 三个作者的姓的第一个字母
bash = bourne again shell
bc = basic (better) calculator
bg = background
biff = 作者heidi stettner在u.c.berkely养的一条狗,喜欢对邮递员汪汪叫。
cal = calendar
cat = catenate
cd = change directory
chgrp = change group
chmod = change mode
chown = change owner
chsh = change shell
cmp = compare
cobra = common object request broker architecture
comm = common
cp = copy
cpio = copy in and out
cpp = c pre processor
cron = chronos 希腊文时间
cups = common unix printing system
cvs = current version system
daemon = disk and execution monitor
dc = desk calculator
dd = disk dump
diff = difference
dmesg = diagnostic message
du = disk usage
ed = editor
egrep = extended grep
elf = extensible linking format
elm = electronic mail
emacs = editor macros
eval = evaluate
ex = extended
exec = execute
fd = file descriptors
fg = foreground
fgrep = fixed grep
fmt = format
fsck = file system check
fstab = filesystem table

fvwm = fuck virtual window manager

gawk = gnu awk
gpg = gnu privacy guard
groff = gnu troff
hal = hardware abstraction layer
joe = joe's own editor
ksh = korn shell
lame = lame ain't an mp3 encoder
lex = lexical analyser
lisp = list processing = lots of irritating superfluous parentheses
lpr = line print
lsof = list open files
m4 = macro processor version 4
man = manual pages
mawk = mike brennan's awk
mc = midnight commander
mkfs = make filesystem
mknod = make node
motd = message of the day
mozilla = mosaic godzilla
mtab = mount table
mv = move
nano = nano's another editor
nawk = new awk
nl = number of lines
nm = names
nohup = no hangup
nroff = new roff
od = octal dump
passwd = password
pg = pager
pico = pine's message composition editor
pine = "program for internet news & email" = "pine is not elm"
pirntcap = printer capability
popd = pop directory
pr = pre
pty = pseudo tty
pushd = push directory
pwd = print working directory
rc = runcom = run command, rc还是plan9的shell
rev = reverse
rm = remove
rn = read news
roff = runoff
rpm = rpm package manager = redhat package manager
rsh, rlogin, rvim中的r = remote
rxvt = our xvt
sed = stream editor
seq = sequence
shar = shell archive
slrn = s-lang rn
ssh = secure shell
ssl = secure sockets layer
stty = set tty
su = substitute user
svn = subversion
tar = tape archive
tcsh = tenex c shell
telnet = teminal over network
termcap = terminal capability
terminfo = terminal information
tex = τέχνη的缩写，希腊文art
tr = traslate
troff = typesetter new roff
tsort = topological sort
tty = teletypewriter
twm = tom's window manager
udev = userspace dev
umask = user's mask
wall = write all
wine = wine is not an emulator
xargs = extended arguments
xdm = x display manager
xlfd = x logical font description
xmms = x multimedia system
xrdb = x resources database
xwd = x window dump
yacc = yet another compiler compiler
fish = the friendly interactive shell
su = switch user
mime = multipurpose internet mail extensions
ecma = european computer manufacturers association
