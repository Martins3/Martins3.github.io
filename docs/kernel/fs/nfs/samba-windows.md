# windows 作为 samba server

https://github.com/samba-team/samba

## server 配置
创建目录
New-Item -Path "C:\Share" -ItemType Directory -Force

然后在图形界面中，右键配置共享

在 linux 中执行如下即可:

```txt
smbclient -L //10.0.0.8/ -U "martins3\97936"
```

如果可以看到如下的结果，那么说明是可以联通的:
```txt
Password for [MARTINS3\97936]:

        Sharename       Type      Comment
        ---------       ----      -------
        ADMIN$          Disk      远程管理
        C$              Disk      默认共享
        IPC$            IPC       远程 IPC
        Share           Disk
```

手动连接:
```txt
sudo mount -t cifs //10.0.0.8/Share mnt \
    -o username=97936,password=1@3456aB,domain=MARTINS3,vers=3.0,iocharset=utf8
```

## client 配置

#### 不要使用密码
sudo mkdir -p /etc/samba
sudo vim /etc/samba/credentials_martins3

```txt
username=97936
password=你的实际密码
domain=MARTINS3
```

sudo chmod 600 /etc/samba/credentials_martins3
sudo chown root:root /etc/samba/credentials_martins3


sudo mount -t cifs //10.0.0.8/Share mnt \
    -o credentials=/etc/samba/credentials_martins3,vers=3.0,iocharset=utf8,uid=$(id -u),gid=$(id -g)

#### 自动 mount
最后添加到 /etc/fstab 中如下内容:
```txt
//10.0.0.8/Share  /home/martins3/hack cifs  credentials=/etc/samba/credentials_martins3,vers=3.0,iocharset=utf8,uid=1000,gid=1000,file_mode=0755,dir_mode=0755,nofail  0  0
```
执行
sudo mount -a

## 基本观察

可以看看 linux 上的 Samba 的源码是哪里的
```txt
  ├─samba-smbd.service
  │ ├─  4169 /nix/store/mcyfbsbxr2hjr53v2yyvqnsnlg894xk2-samba-4.20.1/sbin/smbd
  │ ├─  4229 smbd: notifyd
  │ ├─  4231 smbd: cleanupd
  │ └─760872 smbd: client [10.0.0.8]
```

## 性能测试

```txt
[global]
time_based=1
runtime=1000
ioengine=io_uring
iodepth=1024
numjobs=2
bs=4k

[trash]
rw=randread
filename=/home/martins3/mnt/a.dump
size=10G
```

```txt
trash: (g=0): rw=randread, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=io_uring, iodepth=1024
...
fio-3.39
Starting 2 processes
trash: Laying out IO file (1 file / 10240MiB)
q^Cs: 2 (f=2): [r(2)][28.7%][r=84.1MiB/s][r=21.5k IOPS][eta 11m:54s]
```

考虑到网络只有 1Gb ，所以其实还是还不错的。

## 问题

### 在映射的目录中，linux 无法构建软连接

```txt
ln: failed to create symbolic link '/home/martins3/hack/vm/fedora/iso': Operation not permitted
```

### 在映射的目录中，linux 无法构建unix domain socket

这些报错都是完全看不懂的
```txt
🧀   nc -l -U a.sock
nc: Permission denied

 sudo nc -l -U a.sock
[sudo] password for martins3:
Ncat: bind to a.sock: Permission denied. QUITTING.
```

### 有时候遇到这个错误
```txt
[Sun Sep 21 00:17:16 2025] CIFS: VFS: \\10.0.0.8 has not responded in 180 seconds. Reconnecting...
[Sun Sep 21 09:38:10 2025] CIFS: VFS: \\10.0.0.8 has not responded in 180 seconds. Reconnecting...
[Sun Sep 21 10:00:51 2025] CIFS: VFS: Autodisabling the use of server inode numbers on \\10.0.0.8\Share
[Sun Sep 21 10:00:51 2025] CIFS: VFS: The server doesn't seem to support them properly or the files might be on different servers (DFS)
[Sun Sep 21 10:00:51 2025] CIFS: VFS: Hardlinks will not be recognized on this mount. Consider mounting with the "noserverino" option to silence this message.
```

### 如果 linux 虚拟机和 client 断开之后，那么就会遇到这个错误
```txt
[2218377.222530] CIFS: VFS: \\10.0.0.8 has not responded in 180 seconds. Reconnecting...
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
