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
