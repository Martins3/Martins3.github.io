# 摄影摄像环境
<!-- c30488f3-807e-4f91-9830-196e3cadcd0d -->

## 软件 : Immich
经过调研，那么就是这个项目就是最好的了:
https://github.com/immich-app/immich


利用 [Docker Compose](https://docs.docker.com/compose/) 部署 [Immich](https://immich.app/)。

官方安装文档：[https://docs.immich.app/install/docker-compose](https://docs.immich.app/install/docker-compose)

### 快速部署

```bash
mkdir -p ~/immich-app
cd ~/immich-app

# 下载官方 compose 文件和示例 env
curl -L https://github.com/immich-app/immich/releases/latest/download/docker-compose.yml -o docker-compose.yml
curl -L https://github.com/immich-app/immich/releases/latest/download/example.env -o .env

# 编辑 .env，修改上传目录、数据库目录、密码、版本
cat > .env <<'EOF'
UPLOAD_LOCATION=/home/martins3/immich-app/library
DB_DATA_LOCATION=/home/martins3/immich-app/postgres
IMMICH_VERSION=v2.6.3
DB_PASSWORD=immichpassword123
DB_USERNAME=postgres
DB_DATABASE_NAME=immich
EOF

# v2.6.3 使用 ankane/pgvector 作为数据库
# latest 官方 compose 默认使用 vectorchord（v3 引入）
sed -i 's|image: ghcr.io/immich-app/postgres:14-vectorchord0.4.3-pgvectors0.2.0@sha256:.*|image: ankane/pgvector:latest|' docker-compose.yml
sed -i '/POSTGRES_INITDB_ARGS/d' docker-compose.yml

# 启动
sudo docker-compose up -d
```
第一次打开会进入管理员注册页面，按提示创建账号即可开始使用。


### 常用命令

```bash
cd ~/immich-app

# 查看运行状态
sudo docker-compose ps

# 查看日志
sudo docker-compose logs -f immich-server
sudo docker-compose logs -f immich-machine-learning
sudo docker-compose logs -f database

# 重启
sudo docker-compose restart

# 完全停止并删除容器（数据卷会保留）
sudo docker-compose down

# 停止并删除容器和数据卷（会丢失照片和数据库，慎用）
sudo docker-compose down -v

# 升级 Immich，假设要升级到 `v2.7.0`：
sed -i 's/IMMICH_VERSION=.*/IMMICH_VERSION=v2.7.0/' .env
sudo docker-compose pull
sudo docker-compose up -d
```

### 其他竞品:
- https://github.com/photoprism/photoprism
- Lychee
- https://github.com/besscroft/PicImpact

## 硬件
### 云台相机调研

- https://www.bilibili.com/video/BV1Luj969EWg : 广告不说的缺点，我来扒 | 影石luna 大疆pocket4pro 40个功能【差异】篇 | 个人体验
- https://www.bilibili.com/video/BV1BGjZ6MEdf : 大疆pocket4P说点很多博主不敢提的
- https://www.bilibili.com/video/BV1ghJg6hEWV : 大疆Pocket 4P，到底 Pro 在哪？
- https://www.bilibili.com/video/BV1ZLJg6rEbJ : 800块加个镜头，值吗？大疆Pocket 4P体验
- https://www.bilibili.com/video/BV1DgdhBGEq2 : 终于来了！大疆Pocket 4上手

综合考虑，还是 dji pocket4

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
