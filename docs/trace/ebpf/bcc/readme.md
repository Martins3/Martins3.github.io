# bcc

bcc 最好用就是已经提供的 bcc tools 了

## bcc 脚本
环境配置方法:

在 fedora + 41 也是正常运行的:
```sh
cd ./docs/trace/ebpf/bcc

uv venv \
  --python /usr/bin/python3.14 \
  --system-site-packages \
  .venv

source .venv/bin/activate

python -c 'from bcc import BPF; print("BCC import passed")'
ty check .
nvim .
```
然后在 vn 目录下打开 nvim 就可以了

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
