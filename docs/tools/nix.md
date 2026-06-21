# 如何给 nixpkgs 添加一个新的包

我发现 nixpkgs 下没有 crash 工具，这个问题已经持续好几年了， 我之前发现这个问题
workaround 一下并不复杂，随着我对于 nix 和 rpm 理解的加深，配合 codex ，我决定
自己来解决下这个问题。

## 基本测试方法

```bash
$ ls -la result
result -> /nix/store/ka5k4r3wikk8bg8y7y7iqlmwk7si80jb-crash-utility-9.0.2
```

所以 nix 构建出来的 crash 在这里：

```bash
./result/bin/crash
```

或者完整 store 路径：

```bash
/nix/store/ka5k4r3wikk8bg8y7y7iqlmwk7si80jb-crash-utility-9.0.2/bin/crash
```

注意 which crash 显示的是 /usr/bin/crash，那是系统里可能已经存在的另一个
crash（或者同名命令），不是我们刚构建的。要用 nix 这个需要写完整路径：

```bash
./result/bin/crash --version
```

如果 result 软链被删了，重新跑一遍构建即可：

```bash
nix-build -A crash-utility
./result/bin/crash --version
```

## 注意事项

### 1. fetchFromGitHub

如果就是在 github 上， 不要使用 fetchurl

### 2. 需要添加 maintainers

New packages without maintainers are not accepted; please add yourself in a
separate commit by following the instructions in `maintainers/README.md`.
(Please add it as the first commit in this PR instead of in another PR.)

### 3. 尽量的引用内部的源码

Does this specifically require gdb 16.2? If not, you could reference gdb.src
directly for automatic updates.

```txt
gdbSrc = fetchurl {
    url = "https://ftp.gnu.org/gnu/gdb/gdb-16.2.tar.gz";
```

不过 crash 是一个例外，在 crash 中指定了只能使用 16.2 版本

./gdb-16.2.patch

### 4. 记得 format
```sh
nix-shell --run treefmt
nix develop --command treefmt
nix fmt
```

仅仅 format 一个文件:
```sh
nix-shell --run "treefmt pkgs/by-name/cr/crash-utility/package.nix"
```

### 5. fetchFromGitHub
关于 fetchFromGitHub 的 name 参数默认为 source ，这个就是解压的默认目录。

### 6. sha25

不要使用 sha256 =

而是应该使用 hash="sha256-xxx" 这种格式，获取 sha256 的方法如下:

```sh
     nix-prefetch-url https://ftp.gnu.org/gnu/gdb/gdb-16.2.tar.gz \
       | xargs nix hash convert --hash-algo sha256 --from nix32 --to sri
```

### 7. 记得划分 out dev man

```nix
  outputs = [ "out" "dev" "man" ];
```

然后 postInstall 里也要对应调整安装路径：

```nix
  postInstall = ''
    install -Dm644 crash.8 $man/share/man/man8/crash.8
    install -Dm644 defs.h $dev/include/crash/defs.h
  '';
```

## PR
- https://github.com/NixOS/nixpkgs/pull/531858

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
