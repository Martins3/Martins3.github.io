# 使用 drgn 来学习内核
<!-- 15d529a2-5499-49f5-9d08-71569631c780 -->

(在当前的环境中继续看看 drgn 能不能用吧，转换成容易分析的脚本，
最好是可以一键生成各种结构体的关系)

相关资料:
- https://lwn.net/Articles/952942/
- https://lwn.net/Articles/789641/
- https://utcc.utoronto.ca/~cks/space/blog/linux/DrgnKernelPokingPraise
- https://developers.facebook.com/blog/post/2021/12/09/drgn-how-linux-kernel-team-meta-debugs-kernel-scale/

## 环境准备
- https://drgn.readthedocs.io/en/latest/installation.html
- https://drgn.readthedocs.io/en/latest/getting_debugging_symbols.html

```txt
drgn wq_dump.py
```

### 使用 uv 尝试一下

先使用 p 进入虚拟环境:
```sh
uv pip install drgn
```

准备 debuginfo
```sh
mkdir -p  /lib/modules/$(uname -r)
scp martins3@10.0.2.2:/home/martins3/data/kernel/linux-build/vmlinux  /lib/modules/$(uname -r)
```

很容易走通，这两个东西真的震撼我了，的确比使用 crash 好太多了
```txt
task = find_task(115)
cmdline(task)
```
用这个来分析内核真的不错的

如果可以解决 python 的代码高亮、自动报错问题，那就太好了
2026-01-10 又尝试了一下，问了 AI 看了文档，似乎不容易解决 python 的代码问题

使用方法，例如:
```txt
cd /home/martins3/data/vn
sudo .venv/bin/drgn --debug-directory ~/data/kernel/linux-full/vmlinux drgn-kvm-analysis.py

sudo .venv/bin/drgn --debug-directory ~/data/kernel/linux-full/vmlinux drgn-kvm-vm-parser.py
```

## 使用 drgn 来分析 workqueue

参考: docs/kernel/int-workqueue.md

## oracle 的扩展
https://github.com/oracle-samples/drgn-tools

## 这个看看
https://drgn.readthedocs.io/en/latest/tutorials/blk_rq_qos_crash.html

## 基本的想法，用这个来看那些文件占据了 page cache 的空间

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
