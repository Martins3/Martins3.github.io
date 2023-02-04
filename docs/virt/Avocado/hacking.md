# Avocado : 这个东西真垃圾啊

## 安装
yum install xz tcpdump iproute iputils gcc glibc-headers nc git python3-devel
pip3 install Pillow # centos 7 上有问题的
pip3 install git+https://github.com/avocado-framework/avocado-vt

avocado vt-bootstrap --vt-type qemu # 初始化环境，包括下载 qcow2
avocado plugins
avocado list --vt-type qemu # 这个有问题吧？@todo
avocado run type_specific.io-github-autotest-qemu.migrate.default.tcp

avocado run io-github-autotest-qemu.shutdown

似乎这个命令执行之后，就无法使用 avocado 这个命令了:
pip3 install avocado
这个命令应该是不可以重复执行的
## 常用命令
- avocado plugings

## 文档
- vt : https://avocado-vt.readthedocs.io/en/stable/GetStartedGuide.html#installing-via-system-package-manager
  - https://avocado-vt.readthedocs.io/en/latest/
- 本身 : https://avocado-framework.readthedocs.io/en/latest/guides/user/chapters/plugins.html
- QEMU 的测试:
  - https://github.com/autotest/tp-qemu




## 问题

### 这个文件在哪里？
examples/tests/sleeptenmin.py.data/sleeptenmin.yaml

### 安装
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

### kvm Intel
```txt
9 - Checking for modules kvm, kvm-intel
Module kvm is not loaded. You might want to load it
Module kvm-intel is not loaded. You might want to load it
```

### avocado-vt 的 TestProviders 是什么？
https://avocado-vt.readthedocs.io/en/stable/WritingTests/TestProviders.html

### 运行第一个例子遇到问题
似乎内核有点问题，自己编译的内核总是有 ovs 的报错
bridge 开机之后自动出现的:

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

切换 QEMU 版本之后
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

## 分析下如何实现一个测试
cd $AVOCADO_DATA/avocado-vt/test-providers.d/downloads/io-github-autotest-qemu
上面说的似乎目录是不对的
/root/avocado/data/avocado-vt/virttest/test-providers.d/downloads/io-github-autotest-qemu

## 如何使用 Cartesian

## 在我的环境中搭建可以，需要解决 redir 的问题

## Autotest
Autotest 的最基本的执行都会出现问题:
https://autotest.readthedocs.io/en/latest/main/local/ClientQuickStart.html

## erroa
- https://avocado-framework.readthedocs.io/en/latest/guides/user/chapters/installing.html
  - 最后一段命令的渲染有问题
- https://avocado-framework.readthedocs.io/en/latest/guides/user/chapters/assets.html
  - -help 应该是 --help
- https://avocado-framework.readthedocs.io/en/latest/guides/user/chapters/advanced.html#custom-runnable-identifier
  - 第一个 JOB ID 的位置

## 记录
- virtest 是什么？
  - 之前 Autotest 下的 virttest 现在的 avocado 下使用 vt
  - https://avocado-vt.readthedocs.io/en/latest/Introduction.html#about-virt-test-1

## [ ] Python 编程框架中，到底提供了什么接口

## 参数传递

### 为什么这种方法不行
```sh
avocado run sleeptest.py --test-parameter sleep_length=10
```

```txt
➜  share avocado run sleeptest.py --test-parameter sleep_length=10

JOB ID     : cf40aad1cbe85905158f0bd784fb00b34c34b4ea
JOB LOG    : /root/avocado/job-results/job-2023-02-01T16.03-cf40aad/job.log
 (1/1) sleeptest.py:SleepTest.test: STARTED
 (1/1) sleeptest.py:SleepTest.test:  ERROR: must be real number, not str (0.03 s)
RESULTS    : PASS 0 | ERROR 1 | FAIL 0 | SKIP 0 | WARN 0 | INTERRUPT 0 | CANCEL 0
JOB TIME   : 1.00 s

Test summary:
sleeptest.py:SleepTest.test: ERROR
```

### 这个方法是可以的
的确是可以显示的:
avocado variants --mux-yaml sleeptest-example.yaml --json-variants-dump variants.json

似乎需要安装这个 plugin
- https://avocado-framework.readthedocs.io/en/latest/plugins/optional/varianter_yaml_to_mux.html#yaml-to-mux-plugin
- pip install avocado-framework-plugin-varianter-yaml-to-mux

avocado run sleeptest.py --mux-yaml sleeptest-example.yaml

```txt
sleep_length: 2
timeout: 3
```

- [ ] 尝试写一个参数试试

examples/tests/sleeptenmin.py.data/sleeptenmin.yaml

调试 : avocado --show test run sleeptest.py
提前展示运行的内容 : avocado variants -m sleeptest.yaml --summary 2 --variants 2

## [ ] 解决 redir 的问题

avocado-vt io-github-autotest-qemu.shutdown
