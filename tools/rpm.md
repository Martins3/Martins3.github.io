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
