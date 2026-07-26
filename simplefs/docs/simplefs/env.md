1. 创建 fsgqa 用户/组

在虚拟机中执行
```sh
sudo useradd -m fsgqa
sudo useradd 123456-fsgqa
sudo useradd fsgqa2
sudo groupadd fsgqa
```

3. 安装 dm-flakey 模块

4. 安装 fsverity-utils

```sh
sudo yum install fsverity fio dbench libaio-devel xfsprogs-devel
```

使用这个 nix ，然后执行 ./configure ，然后 make 就可以了:

```nix
with import <nixpkgs> { };

# 各种 C 环境合集都放这里了
pkgs.llvmPackages.stdenv.mkDerivation {
  name = "C test";
  buildInputs = with pkgs; [
    cmake
    libpcap
    liburing
    libtraceevent
    glib
    xfsprogs
    util-linux
    acl
    attr
    libcap
    gdbm
    e2fsprogs
    btrfs-progs
    pkg-config
    fuse3
    libaio
    numactl
    # glibc.static # 可以静态编译
    # 2025-05-09 发现添加上这个，编译运行，程序会直接 crash 的。
  ];
  LD_LIBRARY_PATH = "${lib.makeLibraryPath [ libaio ]}";
}
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
