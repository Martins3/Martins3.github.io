# nmcli 基本使用
<!-- 86780f2c-09d9-49b3-9116-11ecd2f193bd -->

添加这个文件:
/etc/NetworkManager/conf.d/unmanaged.conf
```txt
[keyfile]
unmanaged-devices=interface-name:enp4s0;interface-name:enp5s0
```

## fedora 中 DNS 无法正常工作
<!-- 1605a4f5-eab0-4b3f-baf2-059a3a674f30 -->

不要看 /etc/resolv.conf 的结果，其中很有可能是
```txt
nameserver 127.0.0.53
```
这意味着您的所有 DNS 查询现在都会先发送给 systemd-resolved 在本地运行的 DNS 存根解析器，
然后由它去查询您在配置文件中指定的 8.8.8.8

解决办法，2026-02-03 使用这个尝试成功了:

```sh
# 2. 配置 systemd-resolved 的上游 DNS
sudo mkdir -p /etc/systemd/resolved.conf.d
sudo tee /etc/systemd/resolved.conf.d/dns.conf <<EOF
[Resolve]
DNS=8.8.8.8 1.1.1.1
FallbackDNS=9.9.9.9
EOF

# 3. 重启服务
sudo systemctl restart systemd-resolved

# 4. 验证
resolvectl status          # 检查 Global DNS Servers 是否生效
resolvectl query baidu.com # 测试解析
```

## 被逼无奈，没有办法

/etc/systemd/system/share.service 中添加

只能如此
```txt
[Unit]
Description=share
After=openvswitch.service
Before=remote-fs.target

[Service]
Type=oneshot
ExecStart=/usr/sbin/ip addr add 10.0.0.2/16 dev br-in
ExecStart=/usr/sbin/ip link set br-in up
ExecStart=/usr/sbin/ip link set enp4s0 up
# 也许该配置一下 sudo ip route add default via 192.168.16.3 dev br-in

[Install]
WantedBy=getty.target
```

## [ ] ifconfig 是被废除了吗?

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
