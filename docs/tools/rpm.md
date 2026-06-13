# RPM

- https://rpm-packaging-guide.github.io/

## 如何不依赖 tarball 来实现
- https://stackoverflow.com/questions/17655265/is-it-possible-to-build-an-rpm-package-without-making-a-tar-gz-archive

```txt
Source1: php.conf
Source2: php.ini
Source3: macros.php

install -m 644 $RPM_SOURCE_DIR/php.conf $RPM_BUILD_ROOT/etc/httpd/conf.d
```

## %setup 修改 tarball 的位置
- https://stackoverflow.com/questions/4261565/newbie-rpmbuild-error

## %post 和 %posttrans
- 在安装的时候，自动启动服务，只是:
  - https://docs.fedoraproject.org/en-US/packaging-guidelines/Scriptlets/#_syntax

posttrans 和 post 似乎只有这里才有意义
https://stackoverflow.com/questions/22456217/rpm-scriptlet-ordering-for-install-remove-upgrade-using-yum

## uninstall 的脚本

- https://stackoverflow.com/questions/29390573/rpm-spec-missing-uninstall-section
  - %install : 安装文件的位置
  - %files : 这里列举了所有的文件，当 uninstall 的时候，这些文件可以被自动删除
  - %preun 和 %postun : uninstll 的时候脚本

## rpm
- yum install epel-release
  - 为了安装 sshfs
  - https://support.moonpoint.com/os/unix/linux/centos/epel_repository.php
  - 但是在 Centos 8 中，这个方法没有用: yum --enablerepo=powertools install fuse-sshfs
- rpm -qp --scripts ls.rpm
  - 查询 rpm 安装的执行脚本
- Centos 8 安装 neovim
```sh
yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm
yum install -y neovim python3-neovim
```
- rpm -qlp : 检查 rpm 中包含的内容

- sudo yum list installed : 展示自动安装的内核
- 新安装的 Centos8 无法安装任何软件:
  - 报错： Error: Failed to download metadata for repo 'appstream': Cannot prepare internal mirrorlist: No URLs in mirrorlist
  - https://stackoverflow.com/questions/70963985/error-failed-to-download-metadata-for-repo-appstream-cannot-prepare-internal
```txt
# sed -i 's/mirrorlist/#mirrorlist/g' /etc/yum.repos.d/CentOS-*
# sed -i 's|#baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g' /etc/yum.repos.d/CentOS-*
# dnf distro-sync
# dnf -y install java
```

## 如何创建软链接
https://serverfault.com/questions/82193/creating-symlink-in-usr-bin-when-creating-an-rpm

```txt
%{__ln_s} libcursor.so.%{version} %{buildroot}/usr/lib64/libcursor.so.1
```

## 文档
http://ftp.rpm.org/max-rpm/

## [ ] https://unix.stackexchange.com/questions/330186/where-does-modprobe-load-a-driver-that-udev-requests
modporbe 是如何工作的

## 查找 changelog

rpm -q --changelog php | less

## -ba 和 -bb

https://bodhileafy.wordpress.com/2017/09/11/howto-rpmbuild-ba-and-rpmbuild-bb/

> rpmbuild -ba and rpmbuild -bb are for spec files.
> rpmbuild -ba will generate both source and binary – src.rpm and .rpm
> rpmbuild -bb will generate binary only .rpm

## %post 的作用
https://docs.fedoraproject.org/en-US/packaging-guidelines/Scriptlets/#_syntax

## yum clean all

使用 aliyun 的源之后，

```txt
curl https://mirrors.aliyun.com/repo/Centos-7.repo -o /etc/yum.repos.d/CentOS-Base.repo
sed -i -e '/mirrors.cloud.aliyuncs.com/d' -e '/mirrors.aliyuncs.com/d' /etc/yum.repos.d/CentOS-Base.repo
```

yum makecache 存在如下报错:
```txt
[root@localhost 14:22:59 yum.repos.d]$ yum makecache
Loaded plugins: fastestmirror, langpacks, product-id, search-disabled-repos, subscription-manager
This system is not registered with an entitlement server. You can use subscription-manager to register.
base                                                                                                                                            | 3.6 kB  00:00:00
Not using downloaded base/repomd.xml because it is older than what we have:
  Current   : Wed Jul 10 11:42:30 2024
  Downloaded: Fri Oct 30 04:03:00 2020
extras                                                                                                                                          | 2.9 kB  00:00:00
Not using downloaded extras/repomd.xml because it is older than what we have:
  Current   : Wed Jul 10 11:42:34 2024
  Downloaded: Wed Apr  3 01:41:49 2024
updates                                                                                                                                         | 2.9 kB  00:00:00
Not using downloaded updates/repomd.xml because it is older than what we have:
  Current   : Wed Jul 10 11:42:52 2024
  Downloaded: Sat Jun 22 00:30:50 2024
Loading mirror speeds from cached hostfile
Metadata Cache Created
```

这个时候 yum clean all 可以解决问题

## rpm macros
https://github.com/openeuler-mirror/openEuler-rpm-config/blob/master/macros

## 依赖关系
rpm -qp mypackage.rpm --provides
rpm -qp mypackage.rpm --requires

### 避免额外依赖

https://stackoverflow.com/questions/16598201/disable-rpmbuild-automatic-requirement-finding
在 spec 中添加
```txt
AutoReqProv: no
```


## rpmbuild --showrc
https://www.ichenfu.com/2017/11/20/rpmbuild-not-strip/

rpmbuild --showrc nixos 下，似乎很多环境都不太正常的

## rpm uninstll 和 remove 的区别

https://stackoverflow.com/questions/7398834/rpm-upgrade-uninstalls-the-rpm

```txt
Install the first time:          1
Upgrade:                         2 or higher
                                 (depending on the number of versions installed)
Remove last version of package:  0
```

## 如何让 rpmbuild 不去 strip 一些文件


## 文件的冲突标准是，两个 pkg 提供的包是不是一模一样的

## 调试小技巧
- repoquery : 展示 repo 中有什么东西

## 真的给我干蒙了
```txt
 rpm --eval '%{__spec_install_post}'
```

## --test 来测试而不是真的安装
rpm -ivh --test python3-perf
rpm -Uvh --test python3-perf

## rpm 常用命令
<!-- 7234ce11-b513-4b7e-885c-d8230c78595f -->

- rpm -qa 查询当前系统中安装的所有的包
- rpm -ivh --force --nodeps url
- rpm -Uvh url : vh 升级的时候打出来进度条和日志，但是，-U 和 -i 的区别是升级和安装。
- rpm -qf 可以找到一个文件对应的包
- rpm -q --changelog php
- rpm -ql bison.rpm 检查 rpm 中存在多少文件
- rpm -qp bison.rpm 检查这就是一个 rpm 文件
- rpm -q --scripts 执行脚本
- rpm -ql 系统已经存在的一个包
- rpm -qlp htop.el7.x86_64.rpm
- yum whatprovides xxd

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
