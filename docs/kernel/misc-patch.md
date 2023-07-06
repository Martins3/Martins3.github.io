# 年轻人的第一个 patch

- [谢宝友: 手把手教你给 Linux 内核发 patch](https://zhuanlan.zhihu.com/p/87955006)
- [send kernel patch](https://www.zhihu.com/question/332347632/answer/732307068)

## 核心参考
- [kernel newbie](https://kernelnewbies.org/FoundBug)
- [](https://www.youtube.com/watch?v=LLBrBBImJt4)

- [ ] C 语言注意事项 : https://www.kernel.org/doc/html/latest/translations/zh_CN/process/coding-style.html
- [ ] 内核开发通用的注意事项 : https://www.kernel.org/doc/html/latest/translations/zh_CN/process/4.Coding.html

- https://www.youtube.com/watch?v=LLBrBBImJt4&t=24s
- https://www.youtube.com/watch?v=XXix80GCvpo

## -mm 还是 linux-next ?

https://etsai.site/your-first-linux-kernel-patch/


## 未解之谜

- 客户端怎么配置来着?
- 最后整个网页上为什么存在内容来着?

## mutt 配置
- https://neomutt.org/

- https://gitlab.com/muttmua/mutt/-/wikis/UseCases/Gmail

## 如何缓存邮件
https://superuser.com/questions/1381093/getting-neomutt-to-download-all-headers-and-messages

## 更多的探讨
https://stackoverflow.com/questions/5055614/whats-in-your-muttrc

颜色主题安排上: https://github.com/altercation/mutt-colors-solarized

## maildir 设置 http://www.elho.net/mutt/maildir/

## offlineimap 的设置
https://hechao.li/2019/06/11/Set-up-OfflineIMAP_Mutt_on_Mac/

原来这是一个套件啊:

https://www.offlineimap.org/doc/use_cases.html

https://superuser.com/questions/927632/configuring-offlineimap-for-gmail-ssl-error

## [ ] vim 的 tab 配置是不是需要修改一下 ?

## proxychains
Sign in with app passwords


## 看看 Greg 的 work
http://kroah.com/log/blog/2019/08/14/patch-workflow-with-mutt-2019/


## mutt 如何配置为 tree 的模式

## 如何订阅 qemu 邮箱社区的邮件

```sh
proxychains4 -f /home/martins3/.dotfiles/config/proxychain.conf neomutt -f "imaps://imap.gmail.com/[Gmail]/Sent Mail"
```

## [ ] offlineimap 的自动同步让我感觉有点无语
同步非常的慢，必须使用 systemd 吗?

## 基于 Linux Next 来使用

git remote add linux-next https://git.kernel.org/pub/scm/linux/kernel/git/next/linux-next.git
git fetch --tags linux-next

## 进行的检查和查找 maintainer

./scripts/checkpatch.pl 000*
git send-email --to linux-doc@vger.kernel.org --cc corbet@lwn.net,linux-kernel@vger.kernel.org 0001-Documentation-sound-fix-typo-in-control-names.rst.patch

./scripts/get_maintainer.pl

## 使用 linux-next

git fetch --tags linux-next
git branch fix linux-next

## [ ] lkml 的 link 是如何放到 patch 中的

- [Apply Linux Kernel Patches from LKML](https://adam.younglogic.com/2022/10/apply-linux-kernel-patches-from-lkml/)

## [ ] 多个 mutt 用户 https://gist.github.com/miguelmota/9456162

### [ ] 订阅邮件
官方文档:

https://unix.stackexchange.com/questions/402094/participating-on-the-kernel-mailing-list

### 列表内容
http://vger.kernel.org/vger-lists.html

### [ ] 这种邮件怎么直接回复来着
https://www.spinics.net/lists/linux-newbie/

### [ ] 怎么让一个 channel 的邮件都在一个位置

### [ ] 搜索历史信息，观察这个事情是否已经被处理过

### [ ] kernel newbie 的历史邮件怎么感觉像是死掉了，从

```txt
http://vger.kernel.org/vger-lists.html#linux-newbie
```

似乎 kernelnewbies 可以直接在网页上
http://lists.kernelnewbies.org/mailman/listinfo/kernelnewbies

但是看上去，qq 邮箱中看对话还是很正常的。

看来，这不是 kernel newbie 的一个问题，相同界面上所有工具的类似的问题啊

### [ ] 到底是 IRC，还是什么东西?


## 常用的词汇
factor out

best,
martin

## 记录

```txt
🧀  ./scripts/get_maintainer.pl 0001-md-raid1-freeze-block-layer-queue-during-reshape.patch
Song Liu <song@kernel.org> (supporter:SOFTWARE RAID (Multiple Disks) SUPPORT)
linux-raid@vger.kernel.org (open list:SOFTWARE RAID (Multiple Disks) SUPPORT)
linux-kernel@vger.kernel.org (open list)
```

这里写了如何 reply 的:
- https://lore.kernel.org/linux-raid/658e3fbc-d7bd-3fc9-b82e-0ecb86fd8c49@huawei.com/#R

如何描述一个 race 的问题:
```txt
     shrink_slab                 unregister_shrinker
     ===========                 ===================

				/* wait for B */
			         wait_for_completion()
   rcu_read_lock()

   shrinker_put() --> (B)
				list_del_rcu()
                                 /* wait for rcu_read_unlock() */
				kfree_rcu()
```

## 记录 2

```txt
🧀  ./scripts/get_maintainer.pl 0001-docs-fix-typo-in-zh_TW-and-zh_CN-translation.patch
Alex Shi <alexs@kernel.org> (maintainer:CHINESE DOCUMENTATION)
Yanteng Si <siyanteng@loongson.cn> (maintainer:CHINESE DOCUMENTATION)
Jonathan Corbet <corbet@lwn.net> (maintainer:DOCUMENTATION)
Hu Haowen <src.res@email.cn> (maintainer:TRADITIONAL CHINESE DOCUMENTATION)
Xueshi Hu <xueshi.hu@smartx.com> (commit_signer:1/1=100%,authored:1/1=100%,added_lines:1/1=100%,removed_lines:1/1=100%)
linux-doc@vger.kernel.org (open list:DOCUMENTATION)
linux-kernel@vger.kernel.org (open list)
linux-doc-tw-discuss@lists.sourceforge.net (moderated list:TRADITIONAL CHINESE DOCUMENTATION)
```
cc 就是所有的都带上就可以了。

## 看上去，邮件实际上是在这个位置的
- https://kernel.org/lore.html
  - https://lore.kernel.org/lists.html : 这里找到的才是正确的

- [ ] 这里可以将所有的邮件全部都 mirror 下来

## [ ] 如果 maintainer 要求发起修改，如何处理

## 如何发送一个 patch set
- https://unix.stackexchange.com/questions/672247/how-do-i-send-a-git-patch-series-from-within-mutt


> It can be helpful to manually add In-Reply-To: headers to a patch (e.g., when using git send-email)
to associate the patch with previous relevant discussion,
e.g. to link a bug fix to the email with the bug report.
However, for a multi-patch series, it is generally best to avoid using In-Reply-To: to link to older versions of the series. This way multiple versions of the patch don’t become an unmanageable forest of references in email clients. If a link is helpful,
you can use the https://lkml.kernel.org/ redirector (e.g., in the cover email text) to link to an earlier version of the patch series.

- [x] 这里有告诉 fix 的引用的写法


看一个经典的例子:
```txt
From: Li Nan <linan122@huawei.com>

Commit 2ae6aaf76912 ("md/raid10: fix io loss while replacement replace
rdev") reads replacement first to prevent io loss. However, there are same
issue in wait_blocked_dev() and raid10_handle_discard(), too. Fix it by
using dereference_rdev_and_rrdev() to get devices.

Fixes: d30588b2731f ("md/raid10: improve raid10 discard request")
Fixes: f2e7e269a752 ("md/raid10: pull the code that wait for blocked dev into one function")
Signed-off-by: Li Nan <linan122@huawei.com>
```

### [ ]  patch set 中的 commit 应该如何写

邮件里面都是存在 patchset 0 的啊


## [ ] 格式化自己的代码片段

## [ ] 确认一件事情，patch 从 mutt 打开之后，被


## 固化常用的命令
git show HEAD | perl scripts/get_maintainer.pl --separator , --nokeywords --nogit --nogit-fallback --norolestats --nol

## 如何说明当前应用项目

```sh
git log -1 --pretty=fixes 54a4f0239f2e
```

## 引用 Link 的格式

```sh
Link: https://lore.kernel.org/r/30th.anniversary.repost@klaava.Helsinki.FI/
```

## 举个例子怎么写 v2 patch

- https://lore.kernel.org/linux-raid/20230704074149.58863-1-jinpu.wang@ionos.com/T/#u

注意看，里面的最后三个 `---` 补充一下修改了什么:

v2: addressed comments from Kuai
* Removed newline
* change the missing case in raid1_write_request
* I still kept the change for _wait_barrier and wait_read_barrier, as I did
 performance tests without them there are still lock contention from
 __wake_up_common_lock


## [ ] 似乎 kernelnewbies 的内容还没看

- https://kernelnewbies.org/PatchPhilosophy
