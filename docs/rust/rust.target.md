# 收集一些必须 rust 来解决的

## hyperlight
https://opensource.microsoft.com/blog/2024/11/07/introducing-hyperlight-virtual-machine-based-security-for-functions-at-scale/

对于 rust 的理解不够，无法 demo 这个问题:

有办法解决下吗?
https://github.com/hyperlight-dev/hyperlight
https://news.ycombinator.com/item?id=42078476
如果真的可以方便的使用，那么我感觉这是一个很好的测试 kvm 的入口

即便是将
```diff
diff --git a/Justfile b/Justfile
index 15544bdf41ed..b342e83d18a8 100644
--- a/Justfile
+++ b/Justfile
@@ -36,16 +36,16 @@ tar-static-lib: (build-rust-capi "release") (build-rust-capi "debug")
 # BUILDING
 build-rust-guests target=default-target:
     cd src/tests/rust_guests/callbackguest && cargo build --profile={ { if target == "debug" { "dev" } else { target } }}
-    cd src/tests/rust_guests/callbackguest && cargo build --profile={ { if target == "debug" { "dev" } else { target } }}  --target=x86_64-pc-windows-msvc
+    # cd src/tests/rust_guests/callbackguest && cargo build --profile={ { if target == "debug" { "dev" } else { target } }}  --target=x86_64-pc-windows-msvc
     cd src/tests/rust_guests/simpleguest && cargo build --profile={ { if target == "debug" { "dev" } else { target } }}
-    cd src/tests/rust_guests/simpleguest && cargo build --profile={ { if target == "debug" { "dev" } else { target } }} --target=x86_64-pc-windows-msvc
+    # cd src/tests/rust_guests/simpleguest && cargo build --profile={ { if target == "debug" { "dev" } else { target } }} --target=x86_64-pc-windows-msvc
     cd src/tests/rust_guests/dummyguest && cargo build --profile={ { if target == "debug" { "dev" } else { target } }}

 @move-rust-guests target=default-target:
     cp { { callbackguest_source }}/{ { target }}/callbackguest* { { rust_guests_bin_dir }}/{ { target }}/
-    cp { { callbackguest_msvc_source }}/{ { target }}/callbackguest* { { rust_guests_bin_dir }}/{ { target }}/
+    # cp { { callbackguest_msvc_source }}/{ { target }}/callbackguest* { { rust_guests_bin_dir }}/{ { target }}/
     cp { { simpleguest_source }}/{ { target }}/simpleguest* { { rust_guests_bin_dir }}/{ { target }}/
-    cp { { simpleguest_msvc_source }}/{ { target }}/simpleguest* { { rust_guests_bin_dir }}/{ { target }}/
+    # cp { { simpleguest_msvc_source }}/{ { target }}/simpleguest* { { rust_guests_bin_dir }}/{ { target }}/
     cp { { dummyguest_source }}/{ { target }}/dummyguest* { { rust_guests_bin_dir }}/{ { target }}/

```

使用 https://github.com/hyperlight-dev/hyperlight 的时候，发现了一个问题

执行 just rg
```txt
error[E0463]: can't find crate for `core`
  |
  = note: the `x86_64-unknown-none` target may not be installed
  = help: consider downloading the target with `rustup target add x86_64-unknown-none`

For more information about this error, try `rustc --explain E0463`.
error: could not compile `log` (lib) due to 1 previous error
warning: build failed, waiting for other jobs to finish...
error: could not compile `scopeguard` (lib) due to 1 previous error
error: could not compile `bitflags` (lib) due to 1 previous error
error: could not compile `itoa` (lib) due to 1 previous error
error: could not compile `ryu` (lib) due to 1 previous error
error: could not compile `memchr` (lib) due to 1 previous error
error: could not compile `anyhow` (lib) due to 1 previous error
error: could not compile `serde` (lib) due to 1 previous error
error: Recipe `build-rust-guests` failed on line 38 with exit code 101

```

```txt
🤒  rustup target add x86_64-unknown-none
info: syncing channel updates for '1.81.0-x86_64-unknown-linux-gnu'
info: latest update on 2024-09-05, rust version 1.81.0 (eeb90cda1 2024-09-04)
info: downloading component 'cargo'
  8.3 MiB /   8.3 MiB (100 %)   5.4 MiB/s in  2s ETA:  0s
info: downloading component 'clippy'
info: downloading component 'rust-docs'
 15.9 MiB /  15.9 MiB (100 %)   5.2 MiB/s in  4s ETA:  0s
info: downloading component 'rust-std'
 26.8 MiB /  26.8 MiB (100 %)   4.6 MiB/s in  7s ETA:  0s
info: downloading component 'rustc'
 66.9 MiB /  66.9 MiB (100 %)   3.6 MiB/s in 20s ETA:  0s
info: downloading component 'rustfmt'
info: installing component 'cargo'
info: installing component 'clippy'
info: installing component 'rust-docs'
info: installing component 'rust-std'
 26.8 MiB /  26.8 MiB (100 %)  24.9 MiB/s in  1s ETA:  0s
info: installing component 'rustc'
 66.9 MiB /  66.9 MiB (100 %)  26.9 MiB/s in  2s ETA:  0s
info: installing component 'rustfmt'
info: downloading component 'rust-std' for 'x86_64-unknown-none'
 11.3 MiB /  11.3 MiB (100 %)   4.8 MiB/s in  3s ETA:  0s
info: installing component 'rust-std' for 'x86_64-unknown-none'
```

或者说，rust 中的如下命令如何 nix 化
```txt
rustup target add x86_64-unknown-none
rustup target add x86_64-pc-windows-msvc
```

编译问题都让 ai 来做吧

### 原来是为了这个项目吗?
https://github.com/hyperlight-dev/hyperlight-wasm

## https://github.com/danobi/vmtest

作为入门的

## virtifsd


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
