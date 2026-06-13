# getty
## 这个错误意味着什么?
```txt
Mar 26 19:51:27 ensemble systemd: getty@tty1.service has no holdoff time, scheduling restart.
Mar 26 19:51:27 ensemble systemd: Stop job pending for unit, delaying automatic restart.
```

如果理解这个服务:
```txt
🧀  systemctl status getty@tty1.service
● getty@tty1.service - Getty on tty1
     Loaded: loaded (/etc/systemd/system/getty@.service; disabled; preset: ignored)
    Drop-In: /nix/store/fbfgn7937plaicb7lpkirnk2jvpyk7w4-system-units/getty@.service.d
             └─overrides.conf
     Active: active (running) since Mon 2025-03-24 14:42:33 CST; 1 week 0 days ago
 Invocation: 067022400a4845c7836d599345bb56a7
       Docs: man:agetty(8)
             man:systemd-getty-generator(8)
             https://0pointer.de/blog/projects/serial-console.html
   Main PID: 2021 (agetty)
         IP: 0B in, 0B out
         IO: 268K read, 0B written
      Tasks: 1 (limit: 76716)
     Memory: 524K (peak: 1.9M)
        CPU: 12ms
     CGroup: /system.slice/system-getty.slice/getty@tty1.service
             └─2021 /nix/store/62camdd58i8lfsv26wagljp1nqvvs4la-util-linux-2.39.4-bin/bin/agetty --login-program /nix/store/yr>
```
systemctl status getty@tty0.service 或者
systemctl status getty@tty2.service 也是有的，
但是都是 disable 的

可以看看类似还有多少个服务。

为什么是 /dev/tty1 啊?

## 做做测试

可以直接运行 agetty 吗?

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
