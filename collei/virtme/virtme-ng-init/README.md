# collei virtme Rust init

This is the Rust init from virtme-ng, ported to collei's virtme mode. The code
retains the upstream initialization flow and adds collei's sudo and vsock SSH
integration.

`virtme-init-loader.sh` runs from the initramfs, loads the modules needed to
mount `ROOTFS`, installs `virtme-ng-init.out` into a private `/tmp`, and uses
`switch_root` to start it as PID 1.

Unlike upstream, this port does not require static glibc linkage: the Rust
binary is only executed after the real rootfs is active, so its dynamic loader
and libraries are available. `scripts/virtme.py` builds and packages the
binary automatically.

[Upstream project](https://github.com/arighi/virtme-ng), source baseline
`a68e4d6a85cc` (`virtme-init: Reap zombie processes`).

# Building

```bash
cargo test
cargo build --release --locked
```

# Credits

Author: Andrea Righi <andrea.righi@canonical.com>

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
