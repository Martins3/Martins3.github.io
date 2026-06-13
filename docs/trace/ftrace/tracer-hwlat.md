### hwlat
https://www.kernel.org/doc/html/latest/trace/ftrace.html 中的 Hardware Latency Detector 章节
https://www.kernel.org/doc/html/latest/trace/hwlat_detector.html

用来调试固件的延迟的，所以显然，在虚拟化中，如果 guest 的 steal 很高，那么

echo hwlat > current_tracer

这是正常的
```txt
           <...>-1546    [004] d....   108.291091: #97    inner/outer(us):   10/50    ts:1708259353.369824393 count:3
           <...>-1546    [005] d....   109.315087: #98    inner/outer(us):   29/30    ts:1708259354.503370405 count:4
           <...>-1546    [006] d....   110.275090: #99    inner/outer(us):  110/111   ts:1708259355.407659756 count:17
```

这是 steal 高的
```txt
           <...>-1573    [001] dn...    11.378677: #1     inner/outer(us): 7682/7010  ts:1708259426.783694902 count:1040
           <...>-1573    [002] dn...    12.287614: #2     inner/outer(us): 2587/2141  ts:1708259427.787697450 count:901
           <...>-1573    [003] dn...    13.363839: #3     inner/outer(us): 1629/1973  ts:1708259428.795790888 count:848
           <...>-1573    [004] dn...    14.321745: #4     inner/outer(us): 1982/2206  ts:1708259429.814312548 count:729
           <...>-1573    [005] dn...    15.405588: #5     inner/outer(us): 1737/1766  ts:1708259430.860025744 count:879
           <...>-1573    [006] dn...    16.354655: #6     inner/outer(us): 2091/1977  ts:1708259431.867859341 count:682
           <...>-1573    [007] dn...    17.393665: #7     inner/outer(us): 2268/1662  ts:1708259432.877325894 count:590
           <...>-1573    [008] d....    18.482652: #8     inner/outer(us): 2047/1845  ts:1708259433.883855345 count:585
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
