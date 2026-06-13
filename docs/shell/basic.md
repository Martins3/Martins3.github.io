# shell 常用命令
## shell edit
<!-- bb55f4fd-1ae7-4a6e-b025-c99c1f11cdde -->

- "| 字符    | Ctrl + B | Ctrl + F | Ctrl + H | Ctrl + D |"
- "| 单词    | Alt + B  |  Alt + F | Ctrl + W | Alt + D  |"
- "| 行首/尾 | Ctrl + A | Ctrl + E | Ctrl + U | Ctrl + K |"

Ctrl + W , Ctrl + K 和 windows terminal 有冲突

看看这个东西:
https://blog.hofstede.it/shell-tricks-that-actually-make-life-easier-and-save-your-sanity/

## bash 重定向
<!-- 49955d18-f2e8-4e35-90fd-52b4db82968d -->

https://wizardzines.com/comics/redirects/

- ls > /dev/null
- ls 2> /dev/null
- ls &> /dev/null # 使用 bash 即可
- cat < file

似乎真的掌握了（不存在其他的干扰的情况下)

## curl
<!-- 2420894c-84e1-4dbd-850b-4bdf0ca28250 -->
- curl -LO --output-dir . www.baidu.com
	- -L 如果发生了重定向，那么继续重定向
	- -O 使用远程的名称
	- --output-dir 只有比较新的版本才支持

## dd
<!-- 8a32b388-fc2c-4cb4-bcf0-bee554994b93 -->

- dd if=/dev/zero of=pmem oflag=direct count=4 bs=10M # 基本测试
- dd if=/dev/sda1 of=partition-backup.img bs=4M  iflag=direct status=progress

读写各自有自己的 flags 的，很合理的:
```txt
   iflag=direct                只读源设备，不污染页缓存
   iflag=direct oflag=direct   设备↔设备克隆，双端绕过缓存
```


## ps
<!-- f99451ee-050a-40f3-b980-0246efa3cd90 -->

- ps --ppid 2 -p 2 -o uname,pid,ppid,cmd,cls
  - 列举出来所有的内核线程
  - https://unix.stackexchange.com/questions/411159/linux-is-it-possible-to-see-only-kernel-space-threads-process
  - @todo 理解一下这是啥含义
- ps -elf # @todo 这个几个都是啥含义
- ps aux --sort -rss # 对于内存数量排序


## screen
<!-- 3897717b-8a29-441e-8693-d5d1ce8b914c -->

- screen -d -m sleep 1000
- screen -r
- screen -list
- screen -wipe # 移除死掉的

## stress

- stress-ng --vm-bytes 40000M --vm-keep --vm 8

## gnome

- eog # 在终端中打开图片
- nautilus --browser . # 在终端中打开当前目录

## Shell

- export PS1="\W" : 命令提示符只有 working dir

## tar

### tar.gz
- tar cvzf name_of_archive_file.tar.gz name_of_directory_to_tar
  - https://unix.stackexchange.com/questions/46969/compress-a-folder-with-tar
  - z : 使用 gzip 压缩
- tar -xvf

### gz
gzip -d file.gz

### ouch
ouch -h


## centos
- nmcli networking off
- nmcli networking on

## sudo

- 让 sudo https://unix.stackexchange.com/questions/83191/how-to-make-sudo-preserve-path

## ssh

- kill unresponsive hung SSH session : `~.`

## wget

递归拷贝:
https://stackoverflow.com/questions/273743/using-wget-to-recursively-fetch-a-directory-with-arbitrary-files-in-it

## fd
<!-- dba7382d-e61e-4d1c-bfc2-93a4af44332e -->

fd 使用的是 regex

```sh
fd ".*\.md" | wc -l
```

## qemu-img 转换
qemu-img  convert image_name disk.raw

## 如何将 tmux 的 pane 变成一个新的 windows
<!-- 52b7f245-5ad5-47b7-a110-9c68e0858c90 -->
Ctrl-h !

## manual 中 [] 和 <> 的含义是什么?
<!-- 605525ca-8a36-4fe8-afb1-7f836475be1b -->

- `<>` 是必须存在
- `[]` 是可选

man git-checkout
```txt
git checkout (-b|-B) <new-branch> [<start-point>]
```

## stress-ng 常用命令
<!-- 3bd02bb3-fd73-4d25-a289-724a7693f53f -->

行为模式: 每个进程分配 4GB 内存后，进入休眠状态 1000 秒，期间持占用内存
stress-ng --vm-bytes 4000M --vm-hang 1000 --vm-keep --vm 8

两个核心，测试 10M
stress-ng --vm 2 --vm-bytes 10M

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
