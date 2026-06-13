# openclaw 记录

## 基本安装
主要参考:
https://github.com/openclaw/openclaw/blob/main/docs/channels/feishu.md

openclaw onboard : 然后在其中自动选择，一路都很流畅，没有问题:

```txt
🤒  openclaw gateway status

🦞 OpenClaw 2026.2.15 (3fe22ea) — I'm not saying your workflow is chaotic... I'm just bringing a linter and a helmet.
Service: systemd (enabled)
File logs: /tmp/openclaw/openclaw-2026-02-18.log
Command: /nix/store/ah9c4cigcp8da3207d4ilammbviri8dh-nodejs-22.22.0/bin/node /home/martins3/.npm-packages/lib/node_modules/openclaw/dist/index.js
gateway --port 18789
Service file: ~/.config/systemd/user/openclaw-gateway.service
Service env: OPENCLAW_GATEWAY_PORT=18789

Config (cli): ~/.openclaw/openclaw.json
Config (service): ~/.openclaw/openclaw.json

Gateway: bind=lan (0.0.0.0), port=18789 (service args)
Probe target: ws://172.28.255.122:18789
Dashboard: http://172.28.255.122:18789/
Probe note: bind=lan listens on 0.0.0.0 (all interfaces); probing via 172.28.255.122.

Runtime: running (pid 21627, state active, sub running, last exit 0, reason 0)
RPC probe: ok

Listening: *:18789
Troubles: run openclaw status
Troubleshooting: https://docs.openclaw.ai/troubleshooting
```

网页的访问方法:
```txt
Dashboard link (with token):
http://172.28.255.122:18789/#token=4365483bc7a6cc4ba7919fca0882d8f7
Copy/paste this URL in a browser on this machine to control OpenClaw.
No GUI detected. Open from your computer:
ssh -N -L 18789:127.0.0.1:18789 martins3@172.28.255.122
Then open:
http://localhost:18789/
http://localhost:18789/#token=4365483bc7a6cc4ba7919fca0882d8f7
Docs:
https://docs.openclaw.ai/gateway/remote
https://docs.openclaw.ai/web/control-ui
```

关于 feishu 的操作
```sh
openclaw pairing list feishu
openclaw pairing approve feishu <CODE>
```

同时我参考了: https://www.youtube.com/watch?v=2L0A4VW7CD0 ，从 3.19 的部分就不用看:

## 基本的 workflow
1. 将其 claw 犯过的错误记录下来

## 有待尝试的内容
1. pdf
2. 网页

## 其他项目
- https://github.com/HKUDS/nanobot : 只有 4000 行，超级轻量的项目
- https://github.com/qwibitai/nanoclaw : 大约 5000 行
- https://github.com/AlexAnys/feishu-openclaw : 进阶
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
