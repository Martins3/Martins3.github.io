## 更新 Go
https://gist.github.com/nikhita/432436d570b89cab172dcf2894465753

## env 美化
- https://github.com/adi1090x : 壁纸，系统等

## tar
https://www.cyberciti.biz/faq/how-do-i-compress-a-whole-linux-or-unix-directory/


## kdiff3
// 相比diff有什么好处吗？

## 教程

[小而精的命令行教程](https://linuxtools-rst.readthedocs.io/zh_CN/latest/base/index.html)
https://github.com/chenzhiwei/linux

## install software without admin
1. [安装 zsh](https://stackoverflow.com/questions/15293406/install-zsh-without-root-access)


## user
useradd -m -G wheel -s /bin/bash shen # add one user
su - shen # change to user


## arch linux
pacman -Syyu # 当某些软件无法下载的时候，可以参考使用这种方法实现检查


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

# Deepin 切换软件源
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

## 去除Linux 密码
在某些特殊情况下，我们不想反复输入密码，其实是可以将密码删除掉的。

首先，没有GUI工具可以完成这一件事情， 不过使用命令行也是一件很简单的事情。

一下假设当前用户名为shen
首先使用`sudo visudo` ，然后向文件中间添加
```
 shen ALL=(ALL) NOPASSWD:ALL
```
然后执行命令
```
 sudo passwd -d shen # `whoami`就是当前用户
```
但是当在终端中间敲下`su`的时候，依旧是弹出需要密码提示，原因是没有删除管理员的密码，需要做的事情很简单
```
sudo passwd -d root
```

## 解决 Chrome 密码无法自动填充
[https://www.v2ex.com/t/551427](https://www.v2ex.com/t/551427)
```
cd ~/.config/google-chrome/Default
rm Login\ Data
rm Login\ Data-journal
```
## [参考原文](https://askubuntu.com/questions/281074/can-i-set-my-user-account-to-have-no-password)
You can't do that using the GUI tool, but you can using the terminal.
1. First, if your user has sudo privileges, you must enable its NOPASSWD option. Otherwise, sudo will ask for a password even when you don't have one, and won't accept an empty password.To do so, open the sudoers configuration file with `sudo visudo`, and add the following line to the file, replacing david with your username:
```
    david ALL=(ALL) NOPASSWD:ALL
```
Close the editor to apply the changes, and test the effect on sudo in a new terminal.
2. Delete the password for your user by running this command:
```
    sudo passwd -d `whoami`
```

## wifi 搜索
问题描述:
1. 无法显示附近的wifi
2. Network Manager Applet 显示　device not managed

解决方法:
1. man 5 NetworkManager.conf 了解
2.  vi /etc/NetworkManager/NetworkManager.conf
```
# Configuration file for NetworkManager.
# See "man 5 NetworkManager.conf" for details.

[keyfile]
unmanaged-devices=interface-name:ap0;interface-name:ap1;interface-name:ap2;interface-name:wlp2s0
```
显示管理 wlp2s0 未被管理
3. 将`interface-name:wlp2s0` 去除
4. `sudo systemctl restart NetworkManager`

参考资料
1. [https://unix.stackexchange.com/questions/121356/wireless-networks-showing-device-not-managed-in-network-manager](https://unix.stackexchange.com/questions/121356/wireless-networks-showing-device-not-managed-in-network-manager)

## 禁用独显

> 具体配置不记得，但是内容大致就是安装并且配置bbswitch

如何检查自己是否正确的配置：
在系统刚刚启动的时候使用如下命令，如果输出类似下方的内容即可。
```
# sudo dmesg | grep bbswitch
[    1.977523] bbswitch: version 0.8
[    1.977529] bbswitch: Found integrated VGA device 0000:00:02.0: \_SB_.PCI0.GFX0
[    1.977535] bbswitch: Found discrete VGA device 0000:01:00.0: \_SB_.PCI0.RP01.PXSX
[    1.977687] bbswitch: detected an Optimus _DSM function
[    1.977774] bbswitch: disabling discrete graphics
[    2.023149] bbswitch: Succesfully loaded. Discrete card 0000:01:00.0 is off
```

注意:如果你使用Manjaro等对于更新内核非常激进的发行版，如果那一天忽然发现续航时间忽然大幅下降，有可能是对应内核版本的bbswitch没有安装。

1. https://wiki.deepin.org/wiki/%E6%98%BE%E5%8D%A1
2. [check whether it worked properly.](https://askubuntu.com/questions/239589/how-do-i-determine-whether-bumblebee-is-working-as-expected)
