## make it run

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

- [ ] Firecracker also provides a *metadata service* that securely shares configuration information between the host and guest operating system. 
- [ ] The *jailer* provides a second line of defense in case the virtualization barrier is ever compromised.

- [ ] CPU type : T2, E3


## Rust language

- [ ] https://stackoverflow.com/questions/30938499/why-is-the-sized-bound-necessary-in-this-trait
- https://doc.rust-lang.org/nomicon/hrtb.html
