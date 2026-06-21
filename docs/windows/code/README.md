# Windows API 测试项目

本项目包含多个Windows API使用示例，展示了Windows系统编程的各种功能。

## 构建方法

### 使用CMake命令行构建

```bash
mkdir build
cd build
cmake ..
cmake --build .
# msbuild .\win_heap_demo.vcxproj -t:Rebuild /t:ClangTidy  -p:Configuration=Debug -p:Platform=X64
```

## TODO
这个还 win_echo_server_iocp.cpp 有点 bug 的

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
