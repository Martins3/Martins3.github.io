
bcc/ext4-fault-read.py 的观察结果:

如果没有部署:
```txt
2025-09-19 12:43:11,sleep,1
2025-09-19 12:43:11,hostname,1
2025-09-19 12:43:11,ld-2.28.so,2
2025-09-19 12:43:11,libc-2.28.so,3

2025-09-19 12:43:12,sleep,1
2025-09-19 12:43:12,hostname,1
2025-09-19 12:43:12,ld-2.28.so,2
2025-09-19 12:43:12,libc-2.28.so,2
```

部署之后:
```txt
2025-09-19 12:45:02,libacl.so.1.1.2253,28
2025-09-19 12:45:02,ld-2.28.so,47
2025-09-19 12:45:02,libgcc_s-7.3.0-20211123.so.1,1
2025-09-19 12:45:02,libattr.so.1.1.2448,28
2025-09-19 12:45:02,libgmp.so.10.4.0,1
2025-09-19 12:45:02,_cffi_backend.cpython-310-x86_6,1
2025-09-19 12:45:02,_decimal.cpython-310-x86_64-lin,1
2025-09-19 12:45:02,libm-2.28.so,5
2025-09-19 12:45:02,librt-2.28.so,1
2025-09-19 12:45:02,_sha512.cpython-310-x86_64-linu,1
2025-09-19 12:45:02,_hashlib.cpython-310-x86_64-lin,1
2025-09-19 12:45:02,_json.cpython-310-x86_64-linux-,1
2025-09-19 12:45:02,_bcrypt.abi3.so,1
2025-09-19 12:45:02,libc-2.28.so,54
2025-09-19 12:45:02,libuuid.so.1.3.0,1
2025-09-19 12:45:02,_blake2.cpython-310-x86_64-linu,1
2025-09-19 12:45:02,libpcre.so.1.2.12,6
2025-09-19 12:45:02,libblkid.so.1.1.0,1
2025-09-19 12:45:02,libdl-2.28.so,36
2025-09-19 12:45:02,_random.cpython-310-x86_64-linu,1
2025-09-19 12:45:02,bash,8
2025-09-19 12:45:02,libreadline.so.8.0,1
2025-09-19 12:45:02,libnss_files-2.28.so,34
2025-09-19 12:45:02,_rust.abi3.so,7
2025-09-19 12:45:02,resource.cpython-310-x86_64-lin,1
2025-09-19 12:45:02,libmpfr.so.6.1.0,1
2025-09-19 12:45:02,libpopt.so.0.0.1,28
2025-09-19 12:45:02,logrotate,28
2025-09-19 12:45:02,libsmartcols.so.1.1.0,1
2025-09-19 12:45:02,_uuid.cpython-310-x86_64-linux-,1
2025-09-19 12:45:02,cat,1
2025-09-19 12:45:02,unicodedata.cpython-310-x86_64-,1
2025-09-19 12:45:02,gawk,1
2025-09-19 12:45:02,_heapq.cpython-310-x86_64-linux,1
2025-09-19 12:45:02,libtinfo.so.6.2,10
2025-09-19 12:45:02,_multibytecodec.cpython-310-x86,1
2025-09-19 12:45:02,_contextvars.cpython-310-x86_64,1
2025-09-19 12:45:02,_psutil_linux.abi3.so,1
2025-09-19 12:45:02,libudev.so.1.6.15,1
2025-09-19 12:45:02,grep,7
2025-09-19 12:45:02,_cbson.cpython-310-x86_64-linux,1
2025-09-19 12:45:02,_bisect.cpython-310-x86_64-linu,1
2025-09-19 12:45:02,libpcre2-8.so.0.10.0,29
2025-09-19 12:45:02,ethtool,7
2025-09-19 12:45:02,_openssl.abi3.so,100
2025-09-19 12:45:02,libpthread-2.28.so,35
2025-09-19 12:45:02,_psutil_posix.abi3.so,1
2025-09-19 12:45:02,_opcode.cpython-310-x86_64-linu,1
2025-09-19 12:45:02,libsigsegv.so.2.0.5,1
2025-09-19 12:45:02,_cmessage.cpython-310-x86_64-li,1
2025-09-19 12:45:02,lsblk,1
2025-09-19 12:45:02,libselinux.so.1,29
2025-09-19 12:45:02,libmount.so.1.1.0,1
2025-09-19 12:45:02,_queue.cpython-310-x86_64-linux,1
2025-09-19 12:45:02,_ssl.cpython-310-x86_64-linux-g,1
```

## 如果使用 bpf 来调试，那么就是为这个目录
<!-- 79ba070c-67c5-4eda-821c-54c30eeaf78d -->

../ext4/

比想象的简单很多，在 nixos 环境中也是
可以构建的，不知道为什么会那么复杂

而且学到了一个调试方法，

可以用这个方法来调试，如果无法加载到内核中:
sudo bpftool prog load ./ext4_trace.bpf.o /sys/fs/bpf/test_prog

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
