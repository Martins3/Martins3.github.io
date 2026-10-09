## cppman

下载现成的 HTML 离线包，再用 cppman 自己的转换器批量生成缓存，这样大部分页面无需逐页访问被
  Cloudflare 拦住的网站。

  具体步骤：

  1. 下载 cppreference 英文离线归档（2025-02-09）
     (https://github.com/PeterFeicht/cppreference-doc/releases/tag/v20250209)，解压到
     ~/.local/share/cppreference-offline/。

  2. 读取 cppman 自带的 SQLite 索引 index.db，获得 6027
     页的标题和 URL。把 URL 映射到归档中的 HTML 文件，例
     如：

     /w/cpp/container/vector → w/cpp/container/
     vector.html

     成功匹配 5966 页；剩余 61 页从 cppreference.net 下
     载补齐。

  3. 调用已安装 cppman 的原有函数：

     HTML → util.fixupHTML() → html2groff() → gzip 压缩

     输出到 cppman 默认目录：

     ~/.cache/cppman/cppreference.com/<页面标题>.3.gz

     因此原来的 cppman 命令直接就能读取。

  4. 两页标题太长，生成的文件名超过文件系统 255 字节限
     制。我复制了一份索引到缓存目录，仅缩短这两页标题中
     重复的命名空间前缀；查询关键词和正文完整标题保留，
     没有修改 Nix 安装包。

  5. 用 unshare -Urn 创建无法访问外网的网络命名空间，在
     里面运行 cppman --cache-all，结果是 6027 成功、0 失
     败；另外检查了全部 gzip 文件，并实际打开六个条目验
     证显示。

  每页的来源记录在 /home/martins3/.local/share/
  cppreference-offline/cache-manifest.json，备份和恢复方
  法在 /home/martins3/.local/share/cppreference-offline/
  README.txt。


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
