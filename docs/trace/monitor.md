- https://github.com/pyroscope-io/pyroscope : 连续观测
- https://github.com/parca-dev/parca : 连续观测
- https://github.com/cloudflare/ebpf_exporter

# 观测
- influxdb 有 v1 和 v2 之分
- grafana 默认密码 admin/admin

### [prometheus](https://prometheus.io)
集群的管理

- https://github.com/winsiderss/systeminformer


## grafana
```sh
# 验证: grafana 的默认刷新时间是 1 分钟的
for((i=0; i < 10000; i++)); do
  curl -d "test,tag=1111 time=12,this=$i" -X POST 'http://127.0.0.1:8428/write'
  sleep 1
done
```

初始化
```sh
cd ~/core
git clone https://github.com/VictoriaMetrics/VictoriaMetrics
cd  ~/core/VictoriaMetrics/deployment/docker
docker compose up -d
docker compose down # 删除
```
登录 127.0.0.1:3000

## 需要统计的
- 启动 qemu 次数
- 启动 shell 次数
- ls 次数
- nvim 次数

## page fault 次数

## 内存的碎片化程度

## buddy 的状态之类的

## kvm 的状态，利用 kvm_stat 长期监测

## io 和 网络流量，就是使用 sar 之类的观测就可以了

## 到底是谁在使用 shared memory

## 记录下一天共启动 qemu 多少次

## 参考
- https://github.com/adriannovegil/awesome-observability

## 就个人使用而言，这个更加好
https://github.com/netdata/netdata

## VictoriaMetrics

```sh
#!/usr/bin/env bash

set -E -e -u -o pipefail

cd ~/core
git clone https://github.com/VictoriaMetrics/VictoriaMetrics
cd VictoriaMetrics/deployment/docker
docker compose up -d
# docker compose down # 删除
echo "default user/passwd : admin admin"
google-chrome-stable 127.0.0.1:3000
```
想不到，这个 docker 构建网络还是从 172.17 开始的

```txt
curl -d 'measurement,tag1=value1,tag2=value2 field1=123,field2=1.23' -X POST 'http://localhost:8428/write'
```

## Vector
从这里开始阅读，简单清晰啊: https://vector.dev/docs/about/

具体案例:
https://github.com/vectordotdev/vector-demos/blob/main/aggregator/vector/agent/http/vector.toml


果然已经有人做了:
https://github.com/vectordotdev/vector/issues/6508

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
