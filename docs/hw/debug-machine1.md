两个汇报的不同范围不同
```txt
🤒  cpupower frequency-info

analyzing CPU 2:
  driver: intel_pstate
  CPUs which run at the same hardware frequency: 2
  CPUs which need to have their frequency coordinated by software: 2
  maximum transition latency:  Cannot determine or is not supported.
  hardware limits: 800 MHz - 5.50 GHz
  available cpufreq governors: performance powersave
  current policy: frequency should be within 800 MHz and 5.50 GHz.
                  The governor "powersave" may decide which speed to use
                  within this range.
  current CPU frequency: Unable to call hardware
  current CPU frequency: 5.50 GHz (asserted by call to kernel)
  boost state support:
    Supported: yes
    Active: yes
qemu on  master [!?] via C v12.2.0-gcc via ❄️  impure (martins3-s-QEMU-env)
🧀  sudo dmidecode -t processor | grep -i "Max speed"
[sudo] password for martins3:
        Max Speed: 5800 MHz
```

连续运行两次，第二次性能从 122 下降到 146，现在还是感觉问题太高了。


## 13900K 的结果
https://linux-hardware.org/?probe=f15cf1d31b

- [ ] 中间有些内容还是存在警告的

## amd 的结果
https://linux-hardware.org/?probe=95c540792e
不知道为什么，联想的收集中丢失的数据更多

整个网站勉强可以一看，估计数据偏差特别大，不是每一个人都会上传的
https://linux-hardware.org/?view=trends&formfactor=notebook

## 原则上讲所有的核都是 5.4G Hz 才对

## 经过验证，晚上的时候，还是可以的

看看 2023-06-28 的评测，感觉还是 intel 的远远不及 amd 啊，积热严重，价格也很高
- https://www.bilibili.com/video/BV1vP411i7ne

## 实在是没有办法，尝试使用这种方法吧
taskset -ac 0-16 nvim

## 检查内存频率太低了
```txt
sudo dmidecode -t memory | grep -i "speed"
```


## 有趣
https://www.buildcores.com/builds/AemiXBLDk

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
