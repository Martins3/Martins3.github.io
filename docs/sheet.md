# Martins3 的 Check Sheet

启发于: https://xieby1.github.io/cheatsheet.html#debug-log

不想相同的问题 stackoverflow 两次。

## Shell
- export PS1="\W" : 命令提示符只有 working dir

## rpm
- yum install epel-release
  - 为了安装 sshfs
  - https://support.moonpoint.com/os/unix/linux/centos/epel_repository.php
  - 但是在 Centos 8 中，这个方法没有用: yum --enablerepo=powertools install fuse-sshfs
- rpm -qp --scripts ls.rpm
  - 查询 rpm 安装的执行脚本
- Centos 8 安装 neovim
```sh
yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm
yum install -y neovim python3-neovim
```

- sudo yum list installed : 展示自动安装的内核
- 新安装的 Centos8 无法安装任何软件:
  - 报错： Error: Failed to download metadata for repo 'appstream': Cannot prepare internal mirrorlist: No URLs in mirrorlist
  - https://stackoverflow.com/questions/70963985/error-failed-to-download-metadata-for-repo-appstream-cannot-prepare-internal
```txt
# sed -i 's/mirrorlist/#mirrorlist/g' /etc/yum.repos.d/CentOS-*
# sed -i 's|#baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g' /etc/yum.repos.d/CentOS-*
# dnf distro-sync
# dnf -y install java
```

## tar
- tar cvzf name_of_archive_file.tar.gz name_of_directory_to_tar
  - https://unix.stackexchange.com/questions/46969/compress-a-folder-with-tar
  - z : 使用 gzip 压缩
- tar -xvf

## systemd
- systemctl --user list-timers --all
- systemctl list-timers --all

## ps
- ps --ppid 2 -p 2 -o uname,pid,ppid,cmd,cls
  - 列举出来所有的内核线程
  - https://unix.stackexchange.com/questions/411159/linux-is-it-possible-to-see-only-kernel-space-threads-process

## dd
- dd if=/dev/zero of=pmem count=4 bs=10M

## centos
- nmcli networking off
- nmcli networking on

## sudo
- 让 sudo https://unix.stackexchange.com/questions/83191/how-to-make-sudo-preserve-path

## git
- git ls-files --others --exclude-standard >> .gitignore
  - 将没有被跟踪的文件添加到 .gitignore 中
- git reset : 将所有的内容 unstage
- git commit --amend -m "an updated commit message"
- git fetch origin && git reset --hard origin/master : 和远程完全相同
- git checkout -- fs/ : 将 unstage 的修改删除掉

## bpftrace

## sshfs
```sh
mkdir mnt
sshfs martins3@10.0.2.2:path_to_repo  ~/mnt
```
