## 还是存在这么多错误哦

```txt
测试项                  结果   问题类别
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
generic/090             FAIL   日志回放/崩溃恢复
generic/092             FAIL   unwritten extent 处理
generic/127             FAIL   数据一致性（mmap）
generic/129             FAIL   空间管理/FD 处理
generic/100             PASS   -
generic/109             PASS   -
generic/247             FAIL   静默失败
generic/249             FAIL   pwrite 失败
generic/255             FAIL   extent 转换
generic/257             FAIL   lseek 不支持
generic/286             FAIL   稀疏文件创建
generic/316             PASS   -
generic/339             PASS   -
generic/340-346, 354    FAIL   Bus error (mmap)
generic/360             FAIL   符号链接长度限制
generic/363             FAIL   数据一致性
generic/394             FAIL   文件大小限制
generic/409, 410, 589   FAIL   挂载命名空间
generic/464             FAIL   磁盘满处理
generic/471             PASS   -
generic/473             FAIL   大文件写入
generic/522             PASS   -
generic/531             FAIL   dmesg 错误
generic/563             FAIL   读写性能范围
generic/604             PASS   -
generic/637             FAIL   lseek/目录操作
generic/650             FAIL   目录循环检测
generic/676             FAIL   目录读取 EOF
generic/707, 732        FAIL   磁盘满+mv 操作
generic/736             PASS   -
generic/740             FAIL   魔术数保护
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
