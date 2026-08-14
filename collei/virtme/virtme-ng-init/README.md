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
