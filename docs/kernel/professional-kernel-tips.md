# 从新手到专业

- [ ] 不知道有什么方法订阅这里的 merge 信息 : https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/log/
- [ ] https://brennan.io/2021/05/05/kernel-mailing-lists-thunderbird-nntp/
  - http://pan.rebelbase.com/
- [ ] 应该将 nvim 中继承的 git blame 技术好好重新分析一下，例如，已经进入到一个文件之后，如何获取没一行的 commit message

## 内核关系

- linux 的分支:
  - 主线 rc-1
  - release

- 各个 subsystem 的

## 参与社区
- http://vger.kernel.org/lkml/

## 关于其他的 distribution 的关系

### redhat
好像，他的内核是没有逐个 commit 的，只是存在版本的发布和对应的包:

- https://stackoverflow.com/questions/41314978/can-we-git-clone-the-redhat-kernel-source-code-and-see-the-changes-made-by-them
- https://github.com/linux-audit/audit-userspace : audit 模块的测试，那么是否存在所有的测试整理一下，免得很尴尬。

## 测试，ci 和分析

- [ ] https://github.com/google/syzkaller
- [ ] https://github.com/kernelci/kernelci-core
- [ ] https://www.kernel.org/doc/html/latest/dev-tools/index.html
- https://www.kernel.org/doc/Documentation/admin-guide/tainted-kernels.rst

## kernel patch

### 配置
在 .gitconf 上的设置:
```plain
[sendemail]
  smtpencryption = tls
  smtpserver = smtp.gmail.com
  smtpuser = hubachelor@gmail.com
  smtppass = ***********
  smtpserverport = 587
```
在 gmail 上的设置 https://myaccount.google.com/lesssecureapps, 否则 git sendemail 无法使用

### 使用

```sh
git commit --amend
proxychains4 git send-email 0001-change-mmap-flags-from-PROT_EXEC-to-PROT_READ.patch --to ltp@lists.linux.it
```

[^1]: http://houjingyi233.com/2019/07/15/%E7%BB%99linux%E5%86%85%E6%A0%B8%E6%8F%90%E4%BA%A4%E4%BB%A3%E7%A0%81/
[^2]: https://zhuanlan.zhihu.com/p/138315470
