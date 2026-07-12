# Kata Containers 与 Kubernetes 的关系
<!-- 6ae57cca-8347-4637-8c49-feb68ac544d8 -->

```
┌─────────────────────────────────────────────────────────────────────┐
│                        Kubernetes (编排层)                           │
│                     负责 Pod 调度、服务发现、扩缩容                    │
└───────────────────────────┬─────────────────────────────────────────┘
                            │ CRI (Container Runtime Interface)
                            ▼
┌─────────────────────────────────────────────────────────────────────┐
│              containerd / CRI-O (CRI 实现 - 高级运行时)              │
│                     负责镜像拉取、存储、生命周期管理                   │
└───────────────────────────┬─────────────────────────────────────────┘
                            │ 调用 OCI Runtime (通过 shim v2)
            ┌───────────────┼───────────────┐
            ▼               ▼               ▼
     ┌──────────┐    ┌──────────┐    ┌──────────────┐
     │   runc   │    │   crun   │    │ kata-runtime │  ← OCI 运行时
     │ (默认)   │    │          │    │              │
     └──────────┘    └──────────┘    └──────┬───────┘
                                            │
                                            ▼
                                   ┌─────────────────┐
                                   │   轻量级 VM     │
                                   │  (QEMU/FC/...)  │
                                   │  + kata-agent   │
                                   └─────────────────┘
```

## RuntimeClass 机制

Kubernetes 1.12 引入 **RuntimeClass**，允许集群同时支持多种容器运行时：

```yaml
# 1. 创建 RuntimeClass 资源
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata-qemu        # 在 Pod 中引用的名称
handler: kata-qemu       # 对应 containerd/CRI-O 配置的 runtime handler
---
# 2. 在 Pod 中指定使用 Kata
apiVersion: v1
kind: Pod
metadata:
  name: secure-app
spec:
  runtimeClassName: kata-qemu  # 使用 Kata Containers
  containers:
  - name: app
    image: myapp:latest
```

## 混合部署架构

```
┌────────────────────────────────────────────────────────────────────┐
│                         Kubernetes 集群                             │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────┐ │
│  │   Node 1        │  │   Node 2        │  │   Node 3            │ │
│  │  (通用计算)      │  │  (安全敏感)      │  │  (安全敏感)          │ │
│  │                 │  │                 │  │                     │ │
│  │ ┌─────────────┐ │  │ ┌─────────────┐ │  │ ┌─────────────────┐ │ │
│  │ │ Pod A       │ │  │ │ Pod C       │ │  │ │ Pod D           │ │ │
│  │ │ (runc)      │ │  │ │ (kata)      │ │  │ │ (kata)          │ │ │
│  │ └─────────────┘ │  │ │ ┌─────────┐ │ │  │ │ ┌─────────────┐ │ │ │
│  │ ┌─────────────┐ │  │ │ │ Guest   │ │ │  │ │ │ Guest VM    │ │ │ │
│  │ │ Pod B       │ │  │ │ │ VM      │ │ │  │ │ │             │ │ │ │
│  │ │ (runc)      │ │  │ │ │ +容器    │ │ │  │ │ │ + 容器      │ │ │ │
│  │ └─────────────┘ │  │ │ └─────────┘ │ │  │ │ └─────────────┘ │ │ │
│  └─────────────────┘  │ └─────────────┘ │  │ └─────────────────┘ │ │
│                       └─────────────────┘  └─────────────────────┘ │
└────────────────────────────────────────────────────────────────────┘
```

## Kata vs runc 在 K8s 中的对比

| 维度 | runc (默认) | Kata Containers |
|------|-------------|-----------------|
| **隔离级别** | Linux namespace/cgroup | 硬件虚拟化 + namespace |
| **安全性** | 共享宿主机内核 | 独立 guest kernel，硬件级隔离 |
| **启动速度** | ~100ms | ~100-500ms |
| **资源开销** | 低 | 较高（每个 Pod 需额外内存） |
| **适用场景** | 内部可信应用 | 多租户、不可信代码、安全敏感 |
| **K8s 支持** | 默认 | 通过 RuntimeClass 指定 |

## 典型使用场景

| 场景 | 说明 |
|------|------|
| **多租户 SaaS** | 用户提交不可信代码执行，需要强隔离 |
| **金融/政务** | 合规要求高的场景，需要 VM 级隔离 |
| **CI/CD 流水线** | 执行来自外部的构建脚本，防止逃逸 |
| **边缘计算** | 设备被物理接触风险高，需强化隔离 |

## 集成原理

1. **Kubelet** 通过 CRI 调用 containerd/CRI-O
2. **containerd** 根据 Pod 的 `runtimeClassName` 选择对应的 runtime handler
3. **kata-runtime** 创建轻量级 VM，在 VM 内启动容器
4. **kata-agent** 在 VM 内管理容器生命周期，与宿主机通过 vsock 通信

---

- [ ] https://katacontainers.io/

从这里入手，显然这是不容易的，需要评估一下到底需要花费多少时间:
https://github.com/kata-containers/kata-containers/tree/main/docs/install


## 这是真的吗? 真的就是使用 virtiofs 来作为入口?
<!-- bba14845-cea1-4820-94e1-4bc6fb89d441 -->

```txt
  1. Image 拉取与解压（与 runc 相同）

  高级运行时（containerd 或 CRI-O）负责处理 OCI image：

  • 拉取：从 registry 下载镜像 layers
  • 存储：使用 snapshotter（如 overlayfs、devicemapper）解压 layers
  • 合并：生成最终的 rootfs（位于宿主机目录，如 /var/lib/containerd/...）

  ▌ 这一步与 runc 容器完全一致，Kata 并不直接处理镜像格式转换。

  2. Rootfs 共享给 Guest VM

  Kata Containers 通过以下方式将准备好的 rootfs 共享到轻量级 VM 内：

  方式一：virtio-fs（默认，推荐）

  # QEMU 命令行示例
  -device vhost-user-fs-pci,chardev=char0,tag=kataShared
  -fsdev local,id=fs0,path=/run/kata-containers/shared/sandboxes/xxx,security_model=none

  • 宿主机运行 virtiofsd 守护进程，将 rootfs 目录导出
  • Guest VM 挂载 kataShared 标签的文件系统到 /run/kata-containers/shared/containers
  • 容器 rootfs 通过 bind mount 的方式挂载进容器

  Guest VM 内看到:
  /run/kata-containers/shared/containers/<container-id>/rootfs  →  容器的 rootfs

  方式二：virtio-9p（旧版，已不推荐）

  早期的共享文件系统方案，性能较差。

  方式三：块设备（Block Device）

  如果使用 devicemapper 存储驱动：

  • 直接使用块设备（如 /dev/vda）作为容器的 rootfs
  • 通过 virtio-block 或 virtio-scsi 热插拔到 VM

  3. 容器启动流程

  1. kata-runtime 接收 OCI create/start 命令
  2. 启动轻量级 VM（包含优化的 guest kernel + mini-OS/initrd）
  3. VM 内的 kata-agent 启动（通过 virtio-vsock/serial 与宿主机通信）
  4. kata-agent 接收 gRPC 命令，使用 libcontainer（runc 同款代码）创建容器
  5. 容器进程在 VM 内的独立命名空间中运行

  ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
  关键组件说明

   组件                      作用                                        位置
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   kata-runtime              OCI 运行时，处理 OCI 命令                   宿主机
   containerd-shim-kata-v2   Shim v2 实现，整合 runtime + shim + proxy   宿主机
   virtiofsd                 共享文件系统守护进程                        宿主机
   kata-agent                容器生命周期管理（使用 libcontainer）       Guest VM 内
   Guest Kernel              优化的轻量级内核（启动快、内存占用小）      Guest VM 内
   Guest Image               mini-OS（initrd 或 rootfs image）           Guest VM 内

  ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
  与传统容器的区别

   特性              runc 容器                  Kata Containers
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   隔离机制          Linux namespace + cgroup   硬件虚拟化 (VM) + namespace
   内核              共享宿主机内核             独立的 guest kernel
   rootfs 提供方式   直接挂载（overlayfs）      virtio-fs / 9p / 块设备共享
   OCI Image 使用    直接作为 rootfs            通过共享文件系统映射到 VM
   启动速度          快（~100ms）               较快（~100-500ms）
   安全性            依赖内核隔离               硬件级隔离

  ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
  配置示例

  在 containerd 中配置 Kata：

  # /etc/containerd/config.toml
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kata]
    runtime_type = "io.containerd.kata.v2"

  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kata.options]
    ConfigPath = "/opt/kata/share/defaults/kata-containers/configuration.toml"

  在 Kubernetes 中使用：

  apiVersion: v1
  kind: Pod
  metadata:
    name: kata-nginx
  spec:
    runtimeClassName: kata-qemu  # 指定 Kata runtime
    containers:
    - name: nginx
      image: nginx:latest

  ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
  总结

  Kata Containers 使用 OCI image 的核心流程：

  1. 镜像处理：由 containerd/CRI-O 拉取并解压 OCI image 为 rootfs（与 runc 相同）
  2. 文件共享：通过 virtio-fs 将宿主机上的 rootfs 共享给 Guest VM
  3. 容器创建：VM 内的 kata-agent 使用 libcontainer 创建容器（与 runc 同代码库）
  4. 运行隔离：容器在独立 VM 中运行，拥有独立的内核和更强的隔离性

  这种方式让 Kata Containers 能够无缝兼容现有的 OCI 镜像生态，同时提供接近虚拟机的安全性。
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
