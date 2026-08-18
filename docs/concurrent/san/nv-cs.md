# nvidia compute sanitizer
<!-- efbc782f-c41a-4bcd-b1fe-e850d95daabf -->

https://github.com/NVIDIA/compute-sanitizer-samples

基本使用:
```txt
compute-sanitizer --tool=memcheck ./$(DEBUG_TARGET) oob 256 1
compute-sanitizer --tool=memcheck --leak-check=full ./$(DEBUG_TARGET) leak 256 1
compute-sanitizer --tool=racecheck --racecheck-report=analysis --print-limit=10 ./$(DEBUG_TARGET) race 32 1
compute-sanitizer --tool=initcheck --print-limit=10 ./$(DEBUG_TARGET) uninit 1 1
compute-sanitizer --tool=synccheck --print-limit=1 ./$(DEBUG_TARGET) sync 32 1
```

配合 docs/concurrent/san/code/compute-saninizer.cu 使用

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
