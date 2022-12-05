## 设计文档
- https://assets.amazon.science/96/c6/302e527240a3b1f86c86c3e8fc3d/firecracker-lightweight-virtualization-for-serverless-applications.pdf

## make it run
I think what we should do when coming back
- [ ] vmm
- [ ] mmds / Dumbo
- [ ] devices
- [ ] then the code flow
- [ ] modified the code and test it
  - [ ] where is the log
  - [ ] curl doesn't reply
- [ ] https://github.com/firecracker-microvm/firecracker-containerd
- [ ] learn some syntax from unstaged code in the source tree

## Overview
All the projects that are implementing serverless based on containers should embrace Firecracker wholeheartedly.
It complements containers so well, and the best thing is that it can be managed by Kubernetes

- [ ] https://github.com/rust-vmm/vm-device : 只有一千行，也许就是用于学习  firecracker 的基础了, 其实这是一个大的 project 的小部分，其他的可以好好分析一下

- [ ] 调查一下和 kata 的关系: https://katacontainers.io/

- [ ] net_gen
- [ ] virtio
  - [ ] device
- [ ] firecracker
  - [ ] run with api / without api
- [ ] vmm

- [ ] Restful API
  - in the /api_server

- [ ] Firecracker also provides a *metadata service* that securely shares configuration information between the host and guest operating system.
- [ ] The *jailer* provides a second line of defense in case the virtualization barrier is ever compromised.

- [ ] CPU type : T2, E3

- [ ] show the log

## some simple code flow
- main
  - run_with_api
    - bind_and_run
      - handle_request
        - try_from_request
          - parse_put_actions

## vsock
csm : VsockConnection
- new_peer_init
- new_local_init

- local_port
- peer_port

VSock ==> VsockChannel ==> VsockConnection

VSock<VsockBackend>::process ==> VSock<VsockBackend>::handle_txq_event ==> VsockChannel::recv_pkt ==> apply_conn_mutation + VsockConnection::recv_pkt

vmm/src/vmm_config/vsock.rs::VsockBuilder::create_unixsock_vsock will initlize related data.

## vmm

rpc_interface.rs                                       136            132           1362
builder.rs                                             161            114           1070
resources.rs                                            98             75            805
vstate/vcpu/mod.rs                                      98            185            658
device_manager/mmio.rs                                  74             84            656
lib.rs                                                  70            113            534
device_manager/persist.rs                               52             51            459
vmm_config/drive.rs                                     60             68            437
persist.rs                                              79             54            427
s/integration_tests.rs                                  70             89            409
vstate/vcpu/x86_64.rs                                   37            109            346
vstate/vm.rs                                            48             48            308
memory_snapshot.rs                                      49             49            289
s/mock_seccomp/mod.rs                                   18             15            277
vmm_config/net.rs                                       44             59            260
vmm_config/logger.rs                                    27             30            231
signal_handler.rs                                       36             42            212
vmm_config/balloon.rs                                   34             33            182
default_syscalls/filters.rs                              5             29            166
vstate/vcpu/aarch64.rs                                  36             38            159
default_syscalls/mod.rs                                 20             23            147
device_manager/legacy.rs                                17             14            136
vmm_config/mod.rs                                       22             42            131
vmm_config/machine_config.rs                            14             24            124
vmm_config/vsock.rs                                     23             17            110
vstate/system.rs                                        22             16             76
s/mock_resources/mod.rs                                 17              2             74
vmm_config/metrics.rs                                   12             10             60
s/mock_resources/make_noisy_kernel.sh                   15              9             44
vmm_config/boot_source.rs                                7             26             42
vmm_config/snapshot.rs                                   9             26             41
vmm_config/mmds.rs                                       5              8             26
version_map.rs                                           8              8             25
s/mock_devices/mod.rs                                    6              2             16
s/test_utils/mod.rs                                      5              2             16
vmm_config/instance_info.rs                              1              7              8
default_syscalls/macros.rs                               2             12              8
vstate/mod.rs                                            1              2              3
device_manager/mod.rs                                    1              9              3

## MMDS

### unix domain socket

> This module implements the Unix Domain Sockets backend for vsock - a mediator between
> guest-side AF_VSOCK sockets and host-side AF_UNIX sockets. The heavy lifting is performed by
> `muxer::VsockMuxer`, a connection multiplexer that uses `super::csm::VsockConnection` for
> handling vsock connection states.
> Check out `muxer.rs` for a more detailed explanation of the inner workings of this backend.

## Rust language
- [ ] https://stackoverflow.com/questions/30938499/why-is-the-sized-bound-necessary-in-this-trait
- https://doc.rust-lang.org/nomicon/hrtb.html

### lib
- [ ] https://doc.rust-lang.org/beta/core/num/struct.Wrapping.html

## What we should run
- https://github.com/containers/libkrun
- https://github.com/cloud-hypervisor/cloud-hypervisor

## 中文的参考文档
https://aws.amazon.com/cn/blogs/china/deep-analysis-aws-firecracker-principle-virtualization-container-runtime-technology/?nc1=b_rp

## 相关工具
- https://github.com/weaveworks/ignite : firecracker 的管理工具

## blog
https://www.talhoffman.com/2021/07/18/firecracker-internals/
