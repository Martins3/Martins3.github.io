## kernel 中的 apparmor 是做啥的?
```txt
        apparmor=       [APPARMOR] Disable or enable AppArmor at boot time
                        Format: { "0" | "1" }
                        See security/apparmor/Kconfig help text
                        0 -- disable.
                        1 -- enable.
                        Default value is set via kernel config option.
```
https://codisec.com/veles/

https://news.ycombinator.com/item?id=41069256

- https://apparmor.net/
- https://kubernetes.io/docs/tutorials/security/apparmor/

- https://wirekat.com/apparmor-vs-selinux-a-comparison

- https://news.ycombinator.com/item?id=43445662 : Landlock
  - No root. No containers. No SELinux/AppArmor configs.

似乎 apparmor 和 selinux 是一个并列的机制。

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
