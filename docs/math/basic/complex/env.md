## 首次使用

1. 创建虚拟环境并安装依赖：

   ```text
   uv venv .venv
   uv pip install --python .venv/bin/python numpy matplotlib
   ```

2. 在 NixOS 上直接运行会因 manylinux wheel 找不到 `libstdc++.so.6`、
   `libgcc_s.so.1`、`libz.so.1` 而报 `ImportError`。`shell.nix` 已把这些
   原生库加入 `LD_LIBRARY_PATH`，先进入 `nix-shell`：

   ```text
   nix-shell
   ```

3. 运行脚本（每次运行都会覆盖 `figures/` 下的结果）：

   ```text
   .venv/bin/python python/experiments.py
   ```

   脚本在 `figures/` 下输出九组 SVG 图和 `results.json`。


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
