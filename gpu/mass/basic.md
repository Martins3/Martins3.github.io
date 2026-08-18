## gpustack

```txt
  3. 配置 Docker 使用 NVIDIA 运行时
  • 执行 nvidia-ctk runtime configure --runtime=docker，自动生成了 /etc/docker/daemon.json，内容如下：
    {
      "runtimes": {
          "nvidia": {
              "args": [],
              "path": "nvidia-container-runtime"
          }
      }
    }
  4. 重启 Docker 服务
  • systemctl restart docker
  • 验证 docker info 已显示 Runtimes: runc io.containerd.runc.v2 nvidia
  5. 清理之前创建失败的容器
  • docker rm gpustack（之前因端口冲突创建失败，容器处于错误状态）
```

添加节点，然后添加 ip ，会自动制作一个环境出来。

## Neutree


```bash
     ./bin/neutree-cli launch neutree-core \
       --jwt-secret "neutree-local-dev-secret-32chars" \
       --admin-password "admin123" \
       --version "v1.1.0-nightly-20260531"
```


✅ 本地部署完成

所有核心容器已启动并健康运行：

┌───────────────┬────────────┬────────────────────────┐
│ 容器          │ 状态       │ 访问地址               │
├───────────────┼────────────┼────────────────────────┤
│ neutree-api   │ ✅ Up      │ http://localhost:13000 │
├───────────────┼────────────┼────────────────────────┤
│ neutree-core  │ ✅ Up      │ http://localhost:3001  │
├───────────────┼────────────┼────────────────────────┤
│ kong (网关)   │ ✅ Healthy │ http://localhost:18080 │
├───────────────┼────────────┼────────────────────────┤
│ postgres      │ ✅ Healthy │ -                      │
├───────────────┼────────────┼────────────────────────┤
│ auth (GoTrue) │ ✅ Healthy │ -                      │
├───────────────┼────────────┼────────────────────────┤
│ postgrest     │ ✅ Up      │ -                      │
└───────────────┴────────────┴────────────────────────┘

│ 端口说明：原本 3000 和 80 被系统其他服务占用，已自动调整为 13000 和 18080。

────────────────────────────────────────────────────────────────────────────────

如何访问

1. Web UI
   打开浏览器访问：http://localhost:13000
2. API 文档 / 调试
   API 入口：http://localhost:13000/api/v1/
3. Kong 网关（推理流量入口）
   http://localhost:18080

默认账号
• 管理员密码：admin123（启动时通过 --admin-password 设置，首次启动后建议尽快修改）

常用操作

```bash
  # 查看所有容器状态
  docker ps

  # 查看 neutree-api 日志
  docker logs -f neutree-api

  # 查看 neutree-core 日志
  docker logs -f neutree-core

  # 一键停止
  cd neutree-deploy/neutree-core && docker compose -p neutree-core down

  # 彻底清理（包括数据库数据）
  cd neutree-deploy/neutree-core && docker compose -p neutree-core down -v
```

如果你想继续探索功能（比如创建集群、部署模型推理服务），可以告诉我下一步想做什么。
