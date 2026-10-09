## cppman

由于 cppreference.com 现在添加了 Cloudflare 的防护，导致
cppman --cache-all 无法执行成功

利用 codex 搞了一个有点逆天的解决方法，基本思路为

1. 下载 cppreference 英文离线归档（2025-02-09）
   (https://github.com/PeterFeicht/cppreference-doc/releases/tag/v20250209)，解压到
   ~/.local/share/cppreference-offline/。

2. 读取 cppman 自带的 SQLite 索引 index.db，获得 6027
   页的标题和 URL。把 URL 映射到归档中的 HTML 文件，例如：
   /w/cpp/container/vector → w/cpp/container/vector.html

具体的实现在: ./cache_cppman.py

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
