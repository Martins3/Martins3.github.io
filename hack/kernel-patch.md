# 配置
git config --global core.editor "vim"

在 .github 上的设置:
```
[sendemail]
  smtpencryption = tls
  smtpserver = smtp.gmail.com
  smtpuser = hubachelor@gmail.com
  smtppass = ***********
  smtpserverport = 587
```

在 gmail 上的设置 https://myaccount.google.com/lesssecureapps, 否则 git sendemail 无法使用

## 使用
git commit --amend
proxychains4  git send-email 0001-change-mmap-flags-from-PROT_EXEC-to-PROT_READ.patch --to   ltp@lists.linux.it

[^1]: http://houjingyi233.com/2019/07/15/%E7%BB%99linux%E5%86%85%E6%A0%B8%E6%8F%90%E4%BA%A4%E4%BB%A3%E7%A0%81/
[^2]: https://zhuanlan.zhihu.com/p/138315470
