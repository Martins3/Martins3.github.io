# 并发编程中违反直觉的例子


读写总是需要是 atomic 的
```txt
a = a + 1;                a = a + 1;
if (a == 1) {             if (a == 1) {
  critical_section();       critical_section();
}                         }
```

```txt
while (true) {                        while (true) {
  while (flag != false) {               while (flag != false) {
    ;                                     ;
  }                                     }
  flag = true;                          flag = true;
  critical_section();                   critical_section();
  flag = false;                         flag = false;
}                                     }
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
