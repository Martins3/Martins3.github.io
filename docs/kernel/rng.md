# rng
```txt
virtual/misc/hw_random🔒 🐙
🧀  grep . *
dev:10:183
grep: power: Is a directory
rng_current:none
grep: rng_quality: No such device
rng_selected:0
grep: subsystem: Is a directory
uevent:MAJOR=10
uevent:MINOR=183
uevent:DEVNAME=hwrng
virtual/misc/hw_random🔒 🐙
🤒  pwd
/sys/devices/virtual/misc/hw_random
```

cat /sys/devices/virtual/misc/hw_random/rng_available

## 这个就很深入了
https://www.zx2c4.com/projects/linux-rng-5.17-5.18/

wireguard 的作者写的

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
