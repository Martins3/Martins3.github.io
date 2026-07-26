# 使用 drgn 来学习内核
<!-- 15d529a2-5499-49f5-9d08-71569631c780 -->

相关资料:
- https://lwn.net/Articles/952942/
- https://lwn.net/Articles/789641/
- https://utcc.utoronto.ca/~cks/space/blog/linux/DrgnKernelPokingPraise
- https://developers.facebook.com/blog/post/2021/12/09/drgn-how-linux-kernel-team-meta-debugs-kernel-scale/

## 环境准备
- https://drgn.readthedocs.io/en/latest/installation.html
- https://drgn.readthedocs.io/en/latest/getting_debugging_symbols.html

### Fedora 环境

Fedora 44 直接安装系统包即可:

```sh
sudo dnf install drgn
drgn --version
```

准备 debuginfo
```sh
mkdir -p  /lib/modules/$(uname -r)
scp martins3@10.0.2.2:/home/martins3/data/kernel/linux-build/vmlinux  /lib/modules/$(uname -r)
```

很容易走通，这两个东西真的震撼我了，的确比使用 crash 好太多了
```py
task = find_task(115)
cmdline(task)
```
用这个来分析内核真的不错的

```sh
cd /home/martins3/data/vn
sudo drgn --debug-directory ~/data/kernel/linux-full/vmlinux drgn-kvm-analysis.py

sudo drgn --debug-directory ~/data/kernel/linux-full/vmlinux drgn-kvm-vm-parser.py
```

## 使用的经典案例
1. 使用 drgn 来分析 workqueue : ~/docs/kernel/irq/softirq/workqueue.md
2. ./scripts/ 下
3. 查看全系统 inode-backed page cache 按文件占用:

```sh
cd /home/martins3/data/vn/docs/kernel/tutorial/drgn/scripts
sudo drgn -c /proc/kcore ./page_cache_by_file.py --top 50
```

4. 分析 QEMU/VFIO 使用 iommufd 时的核心对象关系:

```sh
cd /home/martins3/data/vn/docs/kernel/tutorial/drgn/scripts
sudo drgn -k ./iommufd_relationship.py --pid $(cat ~/data/hack/vm/yyds-nv/s/pid)
```

## TODO
https://drgn.readthedocs.io/en/latest/tutorials/blk_rq_qos_crash.html
oracle 的扩展 : https://github.com/oracle-samples/drgn-tools

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
