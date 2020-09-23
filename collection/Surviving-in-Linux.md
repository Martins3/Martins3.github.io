## tar

https://www.cyberciti.biz/faq/how-do-i-compress-a-whole-linux-or-unix-directory/


## kdiff3
// 相比diff有什么好处吗？

## 用户组

## install software without admin 

### zsh
[reference](https://stackoverflow.com/questions/15293406/install-zsh-without-root-access)

# Ubuntu(deepin) 

## 终端版本
gawk, sed, wc, grep, etc.

1. cowsay
2. lolcat
3. nyancat
4. musicbox
5. nmap 网络诊断
6. dig dns分析
7. aria2c ：度盘下载全靠它
8. iptraf-ng ：网络流量分析
9. bwm-ng ：实时网速查看
10. glances ：综合信息查看
11. smartctl ：监视硬盘健康
12. heepie
15. figlet  艺术字体
16. tmux

i7z cpu软件神器，hardinfo 整体主机硬件情况 类似鲁大师AIDA64screenfetch查看主机概要信息nethogs 按进程实时统计网络带宽五星推荐 nload 监控主机网络流量findmnt 树形结构列出所有已经加载的文件系统神器 iotop实时检测监视磁盘io信息使用状态bmon 实时监视指定网卡流量ncdu 磁盘使用分析器cloc 代码统计工具 类似wcnmap 强大扫描器（有windows版本anbox Linux下完全运行安卓系统虽然是沙盒arpwatch arp活动监视器pidstat进程和内核统计信息atop htop强大的资源管理器glances 系统主机 目前运行状态信息 包括开机时间bwm-ng 实时网速查看pyDash 是一个轻量且基于 web 的 Linux 性能监测工具，它是用 Python 和 Django 在 Linux 上使用 Meld 比较文件夹（图像化）极速蜗牛：apt-fastngxtop：在命令行实时监控 Nginx 的神器Bash Getopts - 让你的脚本支持命令行参数Linux 命令行下嗅探 HTTP 流量的工具：httprymps-Youtube命令行youtube播放器fping hping ping的升级版vmstat iostat dastat cfdisk 磁盘分区工具yes 死机神器mc 文件管理器ip 新工具 ifconfig会被这个所淘汰！比如 ip route list 查看当前主机路由表。w3m elink 终端浏览器 简直是神器！（终端浏览器有很多）ipcs 进程间通信设施状态pgreg 和pidof 查找基于名称来查找进程pkill 按进程的名称杀死进程！（这个工具我很惊讶！）dd （有语法，我常用的是将iso写入光盘U盘）col 过滤其他指令控制字符iptraf （网卡监听工具）神器！jion 合并指定文件中相同字段sl 火车彩蛋 oneko猫捉老鼠 espeak星球大战！（无卵用！这类彩蛋有很多！）swaks 邮件伪造cv简直就是linux神器（显示复制拷贝进度条！）hydra九头龙 暴力破解cpustat 是 Linux 下一个强大的系统性能测量程序 （新工具）arbtt，一个开源的自动记录软件， GUI工具Psensor，它允许你在Linux中监控硬件温度 lshw命令是一个综合性硬件查询工具 ctrl+a 终端开始，ctrl+e（终端结尾）（方便）last lastlog（运维神器和我们普通用户没关系）iftop网络流量 nc（catnet）瑞士军刀aptitude apt源管理工具x86info是一个为了展示基于x86架构的CPU信息 cpuid命令的功能就相当于一个专用的CPU信息工具 brctl网桥工具mtr网络诊断工具tee 指定的文件保存制定文件列表中spilt指定文件分割成若干小文件（多半用于分割日志）chroot(卧槽我现在还不会用这个命令！)Nessus的漏洞扫描 route-n（查看ip与路由） cat/proc/net/arp（查看arp）cat /proc/net/netlink（查看哎呀忘了）msfconsole后门工具（逃不过杀软，除非加壳。安卓木马要生成签名！）crunch字典生成工具（生成tb字典，使用小心大字典打开生成会让电脑死机！）PentestBox.exe windows10下渗透工具smb分析工具ptunnel tcphashcat多线程密码破解ttyload 在终端中用彩色显示 Linux 的平均负载resize -s 调节中端samdump2破解xp vista系统账户工具（我天朝pe秒了他）Zypper是SUSE Linux中用于安装，升级，卸载，管理仓库、进行各种包查询的命令行接口 （ SUSE Linux）仅能用于！boxex 是 linux 下的一款命令行工具，可以用字符组成盒子把你的文字包围在里面。 (挺有意思的！)开源的世界软件，有很多未知，目前能想到这是就只有这些了。安装和用法自行百度。我会补充!，错误私信告知。我就不一一截图麻烦！看到下面的按赞按钮了吗?你不点一下吗！


## 安装Java

### Install Oracle Java
1. http://www.oracle.com/technetwork/java/javase/downloads/index-jdk5-jsp-142662.html
2. 需要注册，除了邮件地址, others aren't necessary to be true.
3. usermail is your email address
4. extract it
5. set the PATH in zsh
```sh
export JAVA_HOME=/develop/jdk1.5.0_22
export JRE_HOME=JAVA_HOME/jre
export CLASSPATH=JAVAHOME/lib:JRE_HOME/lib:$CLASSPATH
export PATH=JAVAHOME/bin:JRE_HOME/bin:$PATH
```

# Deepin

### 切换软件源
1. [deepin](https://www.deepin.org/mirrors/packages/) 提供可替换列表
2. 直接sudo vim 替换 将原来的
```
deb [by-hash=force] http://packages.deepin.com/deepin unstable main contrib non-free
#deb-src http://packages.deepin.com/deepin unstable main contrib non-free
```
中间的 `http://packages.deepin.com/deepin` 中间替换为该目录中间的任意的网址，　比如在华科的话，
可以替换为`http://mirrors.hustunique.com/deepin/`
3. sudo apt update  
当链接的网络需要认证的时候，本步骤一般会报错。
