# 一些新的发现

通过 cgroup_subsys_on_dfl(memory_cgrp_subsys) 可以发现，
似乎 linux 支持:

1. cgrp subsys 可以选择自己是在 v1 上，还是在 v2 上的
2. 系统可以同时存在 v1 和 v2

例如这里的:
```c
/**
 * mem_cgroup_uncharge_skmem - uncharge socket memory
 * @memcg: memcg to uncharge
 * @nr_pages: number of pages to uncharge
 */
void mem_cgroup_uncharge_skmem(struct mem_cgroup *memcg, unsigned int nr_pages)
{
	if (!cgroup_subsys_on_dfl(memory_cgrp_subsys)) {
		memcg1_uncharge_skmem(memcg, nr_pages);
		return;
	}

	mod_memcg_state(memcg, MEMCG_SOCK, -nr_pages);

	refill_stock(memcg, nr_pages);
}
```

## 可以看看这个 config 有什么用
CONFIG_CGROUP_DEBUG


## 看看 cgroup 的 kernel config

1. config MEMCG_V1 是专门被抽象出来的，可以默认关闭 v1
2. 一直以为 v1 和 v2 有有一个大的开关才对，但是似乎没有，而是一系列的小开关
```txt
config MEMCG_V1
	bool "Legacy cgroup v1 memory controller"
	depends on MEMCG
	default n
	help
	  Legacy cgroup v1 memory controller which has been deprecated by
	  cgroup v2 implementation. The v1 is there for legacy applications
	  which haven't migrated to the new cgroup v2 interface yet. If you
	  do not have any such application then you are completely fine leaving
	  this option disabled.

	  Please note that feature set of the legacy memory controller is likely
	  going to shrink due to deprecation process. New deployments with v1
	  controller are highly discouraged.

	  Say N if unsure.
```


## 系统服务的配置如何进行的?

```txt
cat /etc/systemd/system/consul-exporter.service.d/99-svcres.conf
[Service]

MemoryLimit=104857600
CPUQuota=5%
```

## 关于 cgroup 中最重要的问题，似乎其他的都是技术细节
<!-- 150e4de4-11ad-4080-8966-2c5a670c4e75 -->

关于 cgroup 应该
1. 如果 cgroup 多个层级的限制，是如何一步步的形成
2. v1 和 v2 关于层级的差别
	- 多个资源如何限制的?

## 调研一下看 cgroup 的好工具是什么

systemd-cgtop 的确是个好工具啊，有没有类似的工具看 cgroup 内存排序的
```sh
#!/usr/bin/env bash
set -E -e -u -o pipefail
PROCS_FILE=/sys/fs/cgroup/memory/system.slice/
cd $PROCS_FILE
for d in */; do [ -f "$d/memory.usage_in_bytes" ] && echo "$(cat "$d/memory.usage_in_bytes") $d"; done | sort -rn
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
