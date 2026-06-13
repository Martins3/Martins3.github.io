## docker 代理的方法
### 使用 aliyun 加速
```json
{
  "registry-mirrors": [
    "https://hub-mirror.c.163.com",
    "https://mirror.baidubce.com"
  ]
,
"default-address-pools":
 [
 {"base":"10.10.0.0/16","size":24}
 ]
}
```
https://cr.console.aliyun.com/cn-hangzhou/instances/mirrors
修改为:
```sh
sudo tee /etc/docker/daemon.json << 'EOF'
{
  "registry-mirrors": ["https://123213413421324.mirror.aliyuncs.com"]
}
EOF
sudo systemctl daemon-reload
sudo systemctl restart docker
```

### 配置代理的方法

https://stackoverflow.com/questions/58841014/set-proxy-on-docker

实际上，这个也是可以工作的
```sh
mkdir -p /etc/systemd/system/docker.service.d
IP=192.168.31.76
cat << _EOF_ > /etc/systemd/system/docker.service.d/http-proxy.conf
[Service]
Environment="HTTP_PROXY=http://$IP:7897/"
Environment="HTTPS_PROXY=http://$IP:7897/"
_EOF_
sudo systemctl daemon-reload
sudo systemctl restart docker
```
这个配置了之后，然后这个测试，效率大概是 1M
```txt
docker pull fedora:43
```

这个才是正确的，来自于官方文档:
```sh
IP=192.168.31.76
cat << _EOF_ > .docker/config.json
{
 "proxies": {
   "default": {
     "httpProxy": "http://$IP:7897",
     "httpsProxy": "http://$IP:7897",
     "noProxy": "127.0.0.0/8"
   }
 }
}
_EOF_
```

## podman
配置代理的方法:
https://podman-desktop.io/docs/proxy

在 $HOME/.config/containers/containers.conf 中添加:
```txt
[engine]
env = ["http_proxy=http://192.168.31.76:7897", "https_proxy=http://192.168.31.76:7897"]
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
