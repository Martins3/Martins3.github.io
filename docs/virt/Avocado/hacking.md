# Avocado

## 安装
yum install xz tcpdump iproute iputils gcc glibc-headers nc git python3-devel
pip3 install Pillow # centos 7 上有问题的
pip3 install git+https://github.com/avocado-framework/avocado-vt

avocado vt-bootstrap --vt-type qemu # 初始化环境，包括下载 qcow2
avocado plugins
avocado list --vt-type qemu # 这个有问题吧？@todo
avocado run type_specific.io-github-autotest-qemu.migrate.default.tcp

avocado run io-github-autotest-qemu.shutdown

## 文档
- https://avocado-vt.readthedocs.io/en/stable/GetStartedGuide.html#installing-via-system-package-manager

## 问题
启动之后遇到如下报错:
```txt
➜  ~ avocado vt-bootstrap --vt-type qemu
No python imaging library installed. Screendump and Windows guest BSOD detection are disabled. In order to enable it, please install python-imaging or
 the equivalent for your distro.
No python imaging library installed. PPM image conversion to JPEG disabled. In order to enable it, please install python-imaging or the equivalent for
 your distro.
Failed to load plugin from module "avocado_vt.plugins.vt": ImportError("Bootstrap missing. Execute 'avocado vt-bootstrap' or disable this plugin to ge
t rid of this message") :
  File "/usr/local/lib/python3.10/site-packages/avocado/core/extension_manager.py", line 94, in __init__
    plugin = ep.load()
  File "/usr/lib/python3.10/site-packages/pkg_resources/__init__.py", line 2458, in load
    return self.resolve()
  File "/usr/lib/python3.10/site-packages/pkg_resources/__init__.py", line 2464, in resolve
    module = __import__(self.module_name, fromlist=['__name__'], level=0)
  File "/usr/local/lib/python3.10/site-packages/avocado_vt/plugins/vt.py", line 43, in <module>
    raise ImportError("Bootstrap missing. "

Failed to load plugin from module "avocado_vt.plugins.vt_list": ImportError("Bootstrap missing. Execute 'avocado vt-bootstrap' or disable this plugin
to get rid of this message") :
  File "/usr/local/lib/python3.10/site-packages/avocado/core/extension_manager.py", line 94, in __init__
    plugin = ep.load()
  File "/usr/lib/python3.10/site-packages/pkg_resources/__init__.py", line 2458, in load
    return self.resolve()
  File "/usr/lib/python3.10/site-packages/pkg_resources/__init__.py", line 2464, in resolve
    module = __import__(self.module_name, fromlist=['__name__'], level=0)
  File "/usr/local/lib/python3.10/site-packages/avocado_vt/plugins/vt_list.py", line 25, in <module>
    from .vt import add_basic_vt_options, add_qemu_bin_vt_option
  File "/usr/local/lib/python3.10/site-packages/avocado_vt/plugins/vt.py", line 43, in <module>
    raise ImportError("Bootstrap missing. "
```

第一个问题，可以通过 `pip install Pillow` 解决

## 这个检测有问题的
```txt
9 - Checking for modules kvm, kvm-intel
Module kvm is not loaded. You might want to load it
Module kvm-intel is not loaded. You might want to load it
```

## [ ] 似乎内核有点问题，自己编译的内核总是有 ovs 的报错
bridge 开机之后自动出现的

## 使用 centos 7 也是遇到了问题

```txt
(1/3) type_specific.io-github-autotest-qemu.migrate.default.tcp.default: STARTED
(1/3) type_specific.io-github-autotest-qemu.migrate.default.tcp.default: CANCEL: Failed to get cpu info with policy ['virttest'] (1.59 s)
(2/3) type_specific.io-github-autotest-qemu.migrate.default.tcp.with_filter_off.with_post_copy: STARTED
(2/3) type_specific.io-github-autotest-qemu.migrate.default.tcp.with_filter_off.with_post_copy: CANCEL: Failed to get cpu info with policy ['virttest'] (1.58 s)
(3/3) type_specific.io-github-autotest-qemu.migrate.default.tcp.with_filter_off.with_multifd: STARTED
(3/3) type_specific.io-github-autotest-qemu.migrate.default.tcp.with_filter_off.with_multifd: CANCEL: Got host qemu version:1.5.3, which is not in [4.0.0,) (1.50 s)
```

根本不知道这是啥含义了啊！
```txt
2023-01-31 01:08:56,062 avocado.virttest.env_process WARNI| Could not get host cpu family
2023-01-31 01:08:56,112 avocado.virttest.env_process DEBUG| KVM version: 3.10.0-1160.71.1.el7.x86_64
2023-01-31 01:08:56,112 avocado.virttest.env_process DEBUG| KVM userspace version(qemu): 1.5.3 (qemu-kvm-1.5.3-175.el7_9.6)
2023-01-31 01:08:56,112 avocado.virttest.env_process INFO | Test requires qemu version: [4.0.0,)
```

## 切换 QEMU 版本之后
```txt
(1/3) type_specific.io-github-autotest-qemu.migrate.default.tcp.default: STARTED
(1/3) type_specific.io-github-autotest-qemu.migrate.default.tcp.default:  PASS (89.85 s)
(2/3) type_specific.io-github-autotest-qemu.migrate.default.tcp.with_filter_off.with_post_copy: STARTED
(2/3) type_specific.io-github-autotest-qemu.migrate.default.tcp.with_filter_off.with_post_copy:  PASS (45.08 s)
(3/3) type_specific.io-github-autotest-qemu.migrate.default.tcp.with_filter_off.with_multifd: STARTED
(3/3) type_specific.io-github-autotest-qemu.migrate.default.tcp.with_filter_off.with_multifd:  CANCEL: Got host qemu version:2.12.0, which is not in [4.0.0,) (5.35 s)
```

## [ ] oe22.04 中似乎 ovs 安装不正常
注意，是无需手动配置 bridge 的
