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

## %post
- 在安装的时候，自动启动服务，只是:
  - https://docs.fedoraproject.org/en-US/packaging-guidelines/Scriptlets/#_syntax

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
