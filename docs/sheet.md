# Martins3 的 Check Sheet

启发于: https://xieby1.github.io/cheatsheet.html#debug-log

不想相同的问题 stackoverflow 两次。

## Shell
- export PS1="\W" : 命令提示符只有 working dir

## tar
- tar cvzf name_of_archive_file.tar.gz name_of_directory_to_tar
  - https://unix.stackexchange.com/questions/46969/compress-a-folder-with-tar
  - z : 使用 gzip 压缩
- tar -xvf

## systemd
- systemctl --user list-timers --all
- systemctl list-timers --all


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

## sshfs
```sh
mkdir mnt
sshfs martins3@10.0.2.2:path_to_repo  ~/mnt
```
