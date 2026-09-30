# libkrun

快速体验:

```sh
sudo dnf install crun-krun
podman run --rm --runtime=krun fedora:latest uname -a # 可以发现和主机的版本不同
podman run -it --rm --runtime=krun fedora:latest
```

确认当前的确运行的虚拟机就是和虚拟机不同的:

```sh
container_id=$(podman ps --latest --quiet)
container_pid=$(podman inspect --format '{{.State.Pid}}' "$container_id")
ls -l "/proc/$container_pid/fd" | grep kvm
ps -T -p "$container_pid"
```

的确是毫秒级别的启动了。

## links

想不到 libkrun 可以做这么有意思的: https://github.com/nohajc/anylinuxfs

https://github.com/containers/libkrun

那么，显然从 kata-containers 入手还是太痛苦了， 可以从 libkrun 入手来学习 rust
的

- https://github.com/boxlite-ai/boxlite
  - 基于 libkrun
  - https://github.com/boxlite-ai/boxlite/tree/main/docs/architecture

https://news.ycombinator.com/item?id=32447995

- https://github.com/containers/krunvm
- https://github.com/containers/libkrun

https://github.com/containers/libkrunfw

- https://news.ycombinator.com/item?id=32447995 : krunvm is a CLI-based utility
  for creating microVMs from OCI images https://github.com/containers/krunvm

## 附录

nix 环境本地运行的脚本参考:

```sh
#!/usr/bin/env bash
set -E -e -u -o pipefail

repo_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
cd "$repo_dir"

libclang_path=$(nix build --no-link --print-out-paths nixpkgs#llvmPackages_22.libclang.lib)
libkrunfw_path=$(nix build --no-link --print-out-paths nixpkgs#libkrunfw)

LIBCLANG_PATH="$libclang_path/lib" \
	LD_LIBRARY_PATH="$libclang_path/lib" \
	make debug

PATH=/usr/bin:/bin /usr/bin/gcc \
	-O2 -g -Iinclude -Ltarget/debug \
	-Wl,-rpath,"$repo_dir/target/debug" \
	-o target/debug/libkrun-local-demo \
	demo.c -lkrun \
	-lkrun_init

LD_LIBRARY_PATH="$repo_dir/target/debug:$libkrunfw_path/lib" \
	"$repo_dir/target/debug/libkrun-local-demo" \
	/ \
	/bin/sh \
	-c \
	'echo guest-kernel; uname -r; echo guest-machine; uname -m; cat /etc/fedora-release'
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
