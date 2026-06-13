# 如何给 OpenEuler 提交打包

## OpenEuler 背景介绍 - [21.03 发布白皮书](https://www.openeuler.org/whitepaper/openEuler-whitepaper-2103.pdf)
> 2021 年 3 月 31 日，openEuler 21.03 创新版如期而至，该版本不仅将内核切换到 Linux Kernel 5.10, 还在内核方向实
现内核热升级、内存分层扩展等多个创新特性，加速提升多核性能，构筑千核运算能力。

## 使用
- [ ] pkgship

## 测试内容

打包方法参考此处:
- https://gitee.com/openeuler/community

## openEuler 提供的打包方法
- https://gitee.com/openeuler/community/blob/master/zh/contributors/package-install.md#https://gitee.com/openeuler/community/blob/master/zh/contributors/prepare-environment.md

## openEuler pkgship 查询包的方法
- https://pkgmanage.openeuler.org/Packagemanagement

## 安装包下载
- exe:
  - https://mirrors.aliyun.com/openeuler/openEuler-22.03-LTS/everything/x86_64/Packages/
  - https://repo.openeuler.org/openEuler-25.03/OS/x86_64/Packages/
- src: https://repo.openeuler.org/openEuler-22.03-LTS/source/Packages/
- debug: https://repo.openeuler.org/openEuler-20.03-LTS/debuginfo/aarch64/Packages/

## 最新在 sp3
https://mirrors.aliyun.com/openeuler/openEuler-20.03-LTS-SP3/update/aarch64/Packages/

## 找到最新的内核 : 在这个链接中找最新的年份
https://mirrors.aliyun.com/openeuler/

## 找到历史上所有的包
https://repo.openeuler.org/openEuler-20.03-LTS/update/x86_64/Packages/

## 内核开发
- https://blog.51cto.com/u_15127420/3292855 : 找到原文档的内容


## lenovo
https://pubs.lenovo.com/uefi_amd_4th/uefi_setting_guide.pdf

## vmware
https://www.yellow-bricks.com/2022/08/30/introducing-vsphere-8/
https://blogs.vmware.com/virtualblocks/2022/08/30/announcing-vsan-8-with-vsan-express-storage-architecture/

## 检查固件版本
https://support.huawei.com/enterprise/en/doc/EDOC1100072476/9d516f4b/querying-the-firmware-version-in-the-os

## 使用 20.03 的源
把这个替换 /etc/yum.repos.d/kernel.repo
```txt
[OS]
name=openEuler-20.03-LTS-OS
baseurl=https://mirrors.tuna.tsinghua.edu.cn/openeuler/openEuler-20.03-LTS/OS/$basearch/
enabled=1
gpgcheck=1
gpgkey=https://mirrors.tuna.tsinghua.edu.cn/openeuler/openEuler-20.03-LTS/OS/$basearch/RPM-GPG-KEY-openEuler

[everything]
name=openEuler-20.03-LTS-Everything
baseurl=https://mirrors.tuna.tsinghua.edu.cn/openeuler/openEuler-20.03-LTS/everything/$basearch/
enabled=1
gpgcheck=1
gpgkey=https://mirrors.tuna.tsinghua.edu.cn/openeuler/openEuler-20.03-LTS/OS/$basearch/RPM-GPG-KEY-openEuler
```

## 使用 22.03 的源
```txt
[OS]
name=openEuler-22.03-LTS-OS
baseurl=https://mirrors.tuna.tsinghua.edu.cn/openeuler/openEuler-22.03-LTS/OS/$basearch/
enabled=1
gpgcheck=1
gpgkey=https://mirrors.tuna.tsinghua.edu.cn/openeuler/openEuler-22.03-LTS/OS/$basearch/RPM-GPG-KEY-openEuler

[everything]
name=openEuler-22.03-LTS-Everything
baseurl=https://mirrors.tuna.tsinghua.edu.cn/openeuler/openEuler-22.03-LTS/everything/$basearch/
enabled=1
gpgcheck=1
gpgkey=https://mirrors.tuna.tsinghua.edu.cn/openeuler/openEuler-22.03-LTS/OS/$basearch/RPM-GPG-KEY-openEuler
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
