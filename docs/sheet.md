# Martins3 的 Check Sheet

启发于: https://xieby1.github.io/cheatsheet.html#debug-log

不想相同的问题 stackoverflow 两次。

## Shell
- export PS1="\W" : 命令提示符只有 working dir

## rpm
- rpm -ivh --force --nodeps url
- rpm -qf 可以找到一个文件对应的包
- rpm2cpio shim-15.4-2.oe2203.src.rpm | cpio -idmv  : 解压一个 rpm 包
- rpm -i --force -nodeps url 可以自动下载安装内核
- yum install whatprovides xxd
- yum install epel-release
  - 为了安装 sshfs
  - https://support.moonpoint.com/os/unix/linux/centos/epel_repository.php
  - 但是在 Centos 8 中，这个方法没有用: yum --enablerepo=powertools install fuse-sshfs

- Centos 8 安装 neovim
```sh
yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm
yum install -y neovim python3-neovim
```

- sudo yum list installed : 展示自动安装的内核

## tar
- tar czf name_of_archive_file.tar.gz name_of_directory_to_tar
  - https://unix.stackexchange.com/questions/46969/compress-a-folder-with-tar

## tools
- ipcs : 查看 share memory 的

## systemd
- systemctl --user list-timers --all
- systemctl list-timers --all

## ps
- ps --ppid 2 -p 2 -o uname,pid,ppid,cmd,cls
  - 列举出来所有的内核线程
  - https://unix.stackexchange.com/questions/411159/linux-is-it-possible-to-see-only-kernel-space-threads-process

## dd
- dd if=/dev/zero of=pmem count=4 bs=10M

## git
- git ls-files --others --exclude-standard >> .gitignore
  - 将没有被跟踪的文件添加到 .gitignore 中

## centos
- nmcli networking off
- nmcli networking on

## sudo
- 让 sudo https://unix.stackexchange.com/questions/83191/how-to-make-sudo-preserve-path

## git
- git reset : 将所有的内容 unstage
- git commit --amend -m "an updated commit message"
- git fetch origin && git reset --hard origin/master : 和远程完全相同
- git checkout -- fs/ : 将 unstage 的取消掉
