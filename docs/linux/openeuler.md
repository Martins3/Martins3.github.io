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

## Guest 初始化
- stress-ng

```sh
wget https://download-ib01.fedoraproject.org/pub/epel/7/x86_64/Packages/s/stress-ng-0.07.29-2.el7.x86_64.rpm
yum install stress-ng-0.13.00-5.el8.x86_64.rpm

yum install -y zsh
sh -c "$(curl -fsSL https://raw.github.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"
```
