# tailscale

## 客户端部署
```sh
#!/usr/bin/env bash
set -E -e -u -o pipefail

# 参考 https://tailscale.com/kb/1053/install-static
# 下载，解压然后使用如下脚本安装就可以了

sudo cp ./systemd/tailscaled.service /etc/systemd/system/
sudo cp ./systemd/tailscaled.defaults /etc/default/tailscaled
sudo cp tailscaled /usr/sbin/tailscaled
mkdir -p ~/.local/bin
sudo cp tailscale ~/.local/bin
sudo systemctl enable tailscaled
sudo systemctl start tailscaled
sudo systemctl status tailscaled

# 登录
sudo tailscale up

# login 之后可以看到这个 ip
# tailscale ip -4
```

## 理论

```txt
  方案 2：部署私有 DERP（最稳定）

  如果你经常需要在这两台机器之间传输数据，自建国内 DERP 是最稳定的方案：

  # 在国内 VPS 上，使用 Docker 一键部署
  docker run -d \
    --name derper \
    -p 443:443 \
    -p 3478:3478/udp \
    -v /certs:/certs \
    -e DERP_DOMAIN=derp.yourdomain.com \
    -e DERP_CERT_MODE=letsencrypt \
    ghcr.io/tailscale/derper:latest

  然后在 Tailscale ACL 中添加你的 DERP：

  {
    "derpMap": {
      "Regions": {
        "900": {
          "RegionID": 900,
          "RegionCode": "china",
          "Nodes": [{
            "Name": "1",
            "HostName": "derp.yourdomain.com",
            "DERPPort": 443
          }]
        }
      }
    }
  }
```

### 实际操作
https://jwinks.com/p/tailscale-derp/

没有域名，直接使用一个 docker 就可以了
```txt
docker run -d \
  --name derper \
  --restart always \
  -p 12345:12345 \
  -p 3478:3478/udp \
  -e DERP_ADDR=:12345 \
  -e DERP_CERTS=/app/certs \
  -e DERP_VERIFY_CLIENTS=false \
  ghcr.io/yangchuansheng/ip_derper:latest
```

如果发现没有 ipv4 的地址:
```txt
# tailscale netcheck

Report:
        * Time: 2026-02-25T12:47:29.1448687Z
        * UDP: false
        * IPv4: (no addr found)
        * IPv6: no, but OS has support
```

```txt
  解决: 去阿里云控制台 → 安全组 → 入方向规则，添加：

  TCP 12345 运行
  UDP 3478/3478   允许  (STUN)
```


## 常用排查命令
### tailscale netcheck
```txt
tailscale netcheck
2026/02/25 15:17:32 portmap: monitor: gateway and self IP changed: gw=192.168.31.1 self=192.168.31.76

Report:
        * Time: 2026-02-25T07:17:37.0159802Z
        * UDP: true
        * IPv4: yes, 27.29.242.114:22631
        * IPv6: no, but OS has support
        * MappingVariesByDestIP: false
        * PortMapping:
        * CaptivePortal: false
        * Nearest DERP: San Francisco
        * DERP latency:
                - sfo: 193.8ms (San Francisco)
                - nue: 206.1ms (Nuremberg)
                - hel: 219.9ms (Helsinki)
                - hnl: 226.1ms (Honolulu)
                - tor: 233.7ms (Toronto)
                - dfw: 237.9ms (Dallas)
                - iad: 238.2ms (Ashburn)
                - nyc: 240.5ms (New York City)
                - den: 242.5ms (Denver)
                - tok: 244.1ms (Tokyo)
                - lhr: 247.3ms (London)
                - ams: 254ms   (Amsterdam)
                - sea: 261.3ms (Seattle)
                - par: 261.5ms (Paris)
                - mia: 261.8ms (Miami)
                - lax: 261.8ms (Los Angeles)
                - ord: 262.9ms (Chicago)
                - waw: 266.9ms (Warsaw)
                - hkg: 269.3ms (Hong Kong)
                - mad: 274.3ms (Madrid)
                - sin: 282.4ms (Singapore)
                - syd: 300.6ms (Sydney)
                - fra: 302.1ms (Frankfurt)
                - dbi: 334.3ms (Dubai)
                - sao: 351.2ms (São Paulo)
                - blr: 391.6ms (Bengaluru)
                - jnb: 403.4ms (Johannesburg)
                - nai: 405.7ms (Nairobi)
```

### tailscale status
```txt
 tailscale status
100.105.106.81  windows  Martins3@  windows  -
100.68.116.54   13900k   Martins3@  linux    idle, tx 11579892 rx 30293972
100.106.194.63  macos    Martins3@  macOS    offline, last seen 107d ago

# Health check:
#     - An update from version 1.90.1 to 1.94.2 is available. Run `tailscale update` or `tailscale set --auto-update` to update now.
```


- direct IP:port → 直连（显示对方公网IP和端口）
- relay 或无标记 → 通过 DERP 中继

如果两个都直接接入到 wifi 中，那么结果如下:
```txt
-> tailscale status
100.68.116.54   13900k               Martins3@    linux   -
100.106.194.63  macos                Martins3@    macOS   offline
100.105.106.81  windows              Martins3@    windows active; direct 192.168.2.228:41641, tx 14408448 rx 4060712

# Health check:
#     - An update from version 1.88.4 to 1.94.2 is available. Run `tailscale update` or `tailscale set --auto-update` to update now.

-> tailscale ping 100.105.106.81
pong from windows (100.105.106.81) via 192.168.2.228:41641 in 4ms
```

如果让 13900k 这个电脑接入接入到手机热点，那么看到就是这个效果，
可以看到发是存在使用了 relay 的:
```txt
tailscale status
100.68.116.54   13900k               Martins3@    linux   -
100.106.194.63  macos                Martins3@    macOS   offline
100.105.106.81  windows              Martins3@    windows active; relay "CN-NB"
```

```txt
tailscale ping 100.105.106.81

ping "100.105.106.81" timed out
ping "100.105.106.81" timed out
pong from windows (100.105.106.81) via DERP(CN-NB) in 178ms
pong from windows (100.105.106.81) via DERP(CN-NB) in 59ms
pong from windows (100.105.106.81) via DERP(CN-NB) in 193ms
ping "100.105.106.81" timed out
ping "100.105.106.81" timed out
pong from windows (100.105.106.81) via DERP(CN-NB) in 117ms
pong from windows (100.105.106.81) via DERP(CN-NB) in 80ms
pong from windows (100.105.106.81) via DERP(CN-NB) in 87ms
direct connection not established
```

原来所谓的打洞，就是 tailscale 可以直接跳过中继，让两个机器直连

## 记录

2026-04-07 最近经常观察到了一些异常，用这用这就断开了。

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
