# Docker

- [lazydocker](https://github.com/jesseduffield/lazydocker)

## docker 如何实现的
- [如何制作最小的 docker image](https://devopsdirective.com/posts/2021/04/tiny-container-image/) : 将 build 和 run 分开
- [awesome-docker](https://github.com/veggiemonk/awesome-docker)


## 资源
https://stackoverflow.com/questions/19585028/i-lose-my-data-when-the-container-exits

## 这个东西有什么创新吗?
https://github.com/google/nsjail

## docker vs podman ?
- https://github.com/containers 搞了好多项目

## 为什么 podman 不需要一个 daemon ?
不知道咋实现的

## 其他环境中的的 docker
https://github.com/sickcodes/Docker-OSX
https://github.com/dockur/windows

## Failed to start Docker Application Container Engine.
的解决办法:

极度稀有的场景，都是在虚拟机被外部杀掉的场景:
https://stackoverflow.com/questions/40524602/error-creating-default-bridge-network-cannot-create-network-docker0-confli
```txt
sudo rm -rf /var/lib/docker/network
sudo systemctl start docker
```
## docker 常用命令
<!-- d6e4da5a-96c8-4f72-961c-eade03c520a0 -->

安装部署:
```sh
sudo yum -y install docker
sudo systemctl enable docker
sudo systemctl start docker
sudo usermod -a -G docker "$USER"
# sudo groupadd docker
# sudo gpasswd -a "$USER" docker
```

观测运维:
```sh
docker stats
```

## docker 不常用命令
save 和 load 一个 docker image 如此简单
docker save -o hello-world.tar hello-world:latest
docker load -i hello-world.tar

## 所以，docker build 相对于 docker run 是走的不同的网络吗?

## docker compose 做什么的
<!-- 7085ed00-7b54-4d50-8784-15c772d93a94 -->

```txt
1. 多容器编排的复杂性

 场景                不用 Compose             使用 Compose
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 启动一个 Web 应用   手动 docker run 3-4 次   docker compose up 一键启动
 容器间网络          手动创建网络、指定 IP    自动创建专用网络，服务名即 DNS
 依赖关系            手动控制启动顺序         depends_on 自动处理
 配置管理            一堆 -e -v -p 参数       统一写在 docker-compose.yml

2. 开发环境一致性

# docker-compose.yml 即文档
services:
  web:
    build: .
    ports: ["8080:80"]
    depends_on: [db, redis]
  db:
    image: postgres:15
    volumes: ["pgdata:/var/lib/postgresql/data"]
  redis:
    image: redis:alpine

解决: "在我机器上能跑" 的问题

3. 与 Kubernetes 的关系

          Docker Compose   Kubernetes
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 定位     单机/开发环境    生产级集群编排
 复杂度   低，YAML 简单    高，概念多
 规模     单机多容器       跨节点数千 Pod

────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
一句话总结

▌ Docker Compose = 单机多容器的"一键管理器"

解决开发、测试阶段快速搭建完整应用栈（Web + DB + Cache）的需求，是 K8s 的"轻量前身/补充"。
```

## 掌握下基本的 docker 技术吧
https://training.play-with-docker.com/

## docker image 的基本管理
<!-- 0ba272c0-8a7d-4b40-9589-96a2903cb9fd -->

1. docker images - 查看所有镜像及其大小
2. docker system df - 查看 Docker 整体磁盘使用情况
3. docker system df -v - 详细查看各镜像、容器、卷的占用

```txt
🧀  docker system df
TYPE            TOTAL     ACTIVE    SIZE      RECLAIMABLE
Images          50        12        59.18GB   29.13GB (49%)
Containers      54        1         568.5MB   568.5MB (99%)
Local Volumes   0         0         0B        0B
Build Cache     207       0         6.933GB   403.7MB
```

```txt
🧀  docker images
                                                                                                                                                  i Info →   U  In Use
IMAGE                                                                                                  ID             DISK USAGE   CONTENT SIZE   EXTRA
fedora:latest                                                                                          62d2b462296d        165MB             0B    U
ghcr.io/metacubex/metacubexd:latest                                                                    3bce53bc7fa2       55.6MB             0B    U
hello-world:latest                                                                                     1b44b5a3e06a       10.1kB             0B    U
martins3:fedora                                                                                        6335fa894a32       1.88GB             0B    U
openeuler/openeuler:22.03-lts-sp2                                                                      346898c7802a        317MB             0B
typesense/typesense:0.20.0                                                                             29958b412d6c        656MB             0B    U
```


2. 查看悬空镜像（dangling）

docker images -f "dangling=true"


一键清理所有未使用资源（推荐）
删除所有停止的容器、未使用的网络、悬空镜像、构建缓存
```txt
docker system prune -a
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
