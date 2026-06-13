# 和社区沟通

内核的工作流程有自己的一个体系，被详细的记录在 Documentation/process/index.rst 下，
至于为什么如此，参考 [Why kernel development still uses email](https://lwn.net/Articles/702177/) 。
我建议开始之前仔细阅读下:

- [A guide to the Kernel Development Process](https://www.kernel.org/doc/html/latest/process/development-process.html)
- [Submitting patches: the essential guide to getting your code into the kernel](https://www.kernel.org/doc/html/latest/process/submitting-patches.html)
- [Working with the kernel development community](https://www.kernel.org/doc/html/latest/process/index.html)

当然，为了阅读速度，可以看看 [中文翻译](https://www.kernel.org/doc/html/latest/translations/zh_CN/index.html)

## 再次重新阅读一下

内核中的文档虽然详细，可以参考如下几篇博客来检查下自己的想法，推荐程度从上到下，依次减弱。

- [你的第一个 Linux 内核补丁](https://etsai.site/your-first-linux-kernel-patch/)
- [谢宝友: 手把手教你给 Linux 内核发 patch](https://zhuanlan.zhihu.com/p/87955006)
- [我修了 Linux 内核的一个小 bug，应该如何把它推进主线呢？](https://www.zhihu.com/question/332347632/answer/732307068)

## 其他的参考

- [kernel newbie](https://kernelnewbies.org/FoundBug)
- [Write and Submit your first Linux kernel Patch : Youtube, Greg](https://www.youtube.com/watch?v=LLBrBBImJt4)
- [](https://www.youtube.com/watch?v=XXix80GCvpo)
- [ ] C 语言注意事项 : https://www.kernel.org/doc/html/latest/translations/zh_CN/process/coding-style.html
- [ ] 内核开发通用的注意事项 : https://www.kernel.org/doc/html/latest/translations/zh_CN/process/4.Coding.html


## 选择正确的分支

```txt
git remote add linux-next https://git.kernel.org/pub/scm/linux/kernel/git/next/linux-next.git
git fetch --tags linux-next
git checkout -b fix
```

访问 mm-everything :
https://git.kernel.org/pub/scm/linux/kernel/git/akpm/mm.git/log/?h=mm-everything

感觉 for-next 没啥区别

md 的分支在那里:
https://git.kernel.org/pub/scm/linux/kernel/git/song/md.git/commit/?h=md-next&id=c4fe7edfc73f750574ef0ec3eee8c2de95324463
好像真正的是在这个分区里面:
https://git.kernel.org/pub/scm/linux/kernel/git/song/md.git/log/?h=md-next

对比下 mm-tree 是如何工作的
https://git.kernel.org/pub/scm/linux/kernel/git/akpm/mm.git

找到各个 maintainer 的分支在那里? 检查文件 : MAINTAINERS 其中会高速在那个 tree 中。

## 使用 Git 发送邮件

主要参考 [Git: send patch using send-email & Gmail](https://blog.revathskumar.com/2019/08/git-send-patch-using-send-email-and-gmail.html)

### v2 patch

- https://lore.kernel.org/linux-raid/20230704074149.58863-1-jinpu.wang@ionos.com/T/#u

发送 v2 patchset

```sh
git format-patch -1 --subject-prefix="PATCH vY" -o /tmp/
```

```sh
git format-patch -3 --subject-prefix="PATCH v2" --cover-letter -o /tmp/x
```

```txt
O_TMPFILE is actually __O_TMPFILE|O_DIRECTORY. This means that the old
check for whether RESOLVE_CACHED can be used would incorrectly think
that O_DIRECTORY could not be used with RESOLVE_CACHED.

Cc: stable@vger.kernel.org # v5.12+
Fixes: 3a81fd02045c ("io_uring: enable LOOKUP_CACHED path resolution for filename lookups")
Signed-off-by: Aleksa Sarai <cyphar@cyphar.com>
---
Changes in v3:
- drop openat2 patch, as it's already in Christian's tree
- explain __O_TMPFILE usage in io_openat_force_async comment
- v2: https://lore.kernel.org/r/20230806-resolve_cached-o_tmpfile-v2-0-058bff24fb16@cyphar.com

Changes in v2:
- fix io_uring's io_openat_force_async as well.
- v1: <https://lore.kernel.org/r/20230806-resolve_cached-o_tmpfile-v1-1-7ba16308465e@cyphar.com>
```

## 自动化 git send email

- https://www.marcusfolkesson.se/blog/get_maintainers-and-git-send-email/

git send-email --identity=linux

但是有时候想要额外的添加一些用户，还是需要一个脚本的

基本操作流程:

```txt
git format-patch -1
./scripts/checkpatch.pl 000*
./scripts/get_maintainer.pl
git send-email --to linux-doc@vger.kernel.org --cc corbet@lwn.net,linux-kernel@vger.kernel.org 0001-Documentation-sound-fix-typo-in-control-names.rst.patch
```

## 代码风格

需要阅读一下 [Linux kernel coding style](https://www.kernel.org/doc/Documentation/process/coding-style.rst)

可以通过 clang-format 来格式化代码 [kerneldoc: clang-format](https://www.kernel.org/doc/html/latest/process/clang-format.html)

之后提交的时候再次用 scripts/checkpatch.pl 检查，一般问题不大。

但是我发现大多数文件被 clang-format 格式化一次，都是会产生变化。 内核每一个文件都存在 space 和 tab 互相混合使用的情况。

## 使用 thunerdird 来回复和查阅邮件

虽然 git 一样可以回复，例如 lore 的网址下面存在 https://lore.kernel.org/linux-raid/658e3fbc-d7bd-3fc9-b82e-0ecb86fd8c49@huawei.com/#R

```sh
  git send-email \
    --in-reply-to=658e3fbc-d7bd-3fc9-b82e-0ecb86fd8c49@huawei.com \
    --to=yukuai3@huawei.com \
    --cc=linux-raid@vger.kernel.org \
    --cc=song@kernel.org \
    --cc=somethine@somewhere \
    /path/to/YOUR_REPLY
```

但是实际上，这样不好。

## Thunerdird 基本使用
<!-- 40e9d469-8800-448f-ae81-f5297d597139 -->

官方文档:
https://unix.stackexchange.com/questions/402094/participating-on-the-kernel-mailing-list


- 用 thunerdird 看，用 nvim 以及 git send email 写
- https://useplaintext.email/#thunderbird

### 初始化

2026-04-20 : 其实没那么复杂，如果配置了代理，thunderbird 会自动的弹出来一个网页，
这个委托给 google 的认证过程，通过了就可以了，所以不存在使用各种诡异的 key 之类的。

无论是默认的 gmail 还是公司的邮箱都是可以这样配置的。

### 订阅邮件
列表内容: http://vger.kernel.org/vger-lists.html

具体操作参考:
- https://brennan.io/2021/05/05/kernel-mailing-lists-thunderbird-nntp/
  - 几乎是按照这个方法，但是最后是 右键 nntp.lore.kernel.org 来订阅的

这里的账号和名称是可以随便填写的，但是最好还是刚才创建的。

### 回复邮件

- https://github.com/Frederick888/external-editor-revived/wiki

1. 安装，参考 https://github.com/Frederick888/external-editor-revived/wiki/Linux
  1. 下载 external-editor-revived-vX.Y.Z.xpi
  2. 下载安装 ubuntu-latest-musl-native-messaging-host-vX.Y.Z.zip
执行
```sh
mkdir -p "$HOME/.mozilla/native-messaging-hosts"
./external-editor-revived | tee "$HOME/.mozilla/native-messaging-hosts/external_editor_revived.json"
```

## 如何引用 git commit

```sh
git log -1 --pretty=fixes 54a4f0239f2e
git kernel 48380368dec1
```

## 利用上 lore.kernel.org

https://lore.kernel.org/

搜索历史信息，观察这个事情是否已经被处理过

## [ ] lkml 的 link 是如何放到 patch 中的

这不是我们的事情

- [Apply Linux Kernel Patches from LKML](https://adam.younglogic.com/2022/10/apply-linux-kernel-patches-from-lkml/)

### [ ] 这种邮件怎么直接回复来着

https://www.spinics.net/lists/linux-newbie/

### [ ] kernel newbie 的历史邮件怎么感觉像是死掉了，从

http://vger.kernel.org/vger-lists.html#linux-newbie

似乎 kernelnewbies 可以直接在网页上
http://lists.kernelnewbies.org/mailman/listinfo/kernelnewbies

但是看上去，qq 邮箱中看对话还是很正常的。

看来，这不是 kernel newbie 的一个问题，相同界面上所有工具的类似的问题啊

## 写邮件的一些例子

### 如何描述一个 race 的问题

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

## 看上去，邮件实际上是在这个位置的

- https://kernel.org/lore.html
  - https://lore.kernel.org/lists.html : 这里找到的才是正确的

## 引用的格式

git log 中的 Link 是自动生成的
Link: https://lore.kernel.org/r/30th.anniversary.repost@klaava.Helsinki.FI/

## [ ] 似乎 kernelnewbies 的内容还没看

### https://kernelnewbies.org/PatchPhilosophy

Once you've reorganized your patchset, you should resend it via email. You'll need to do three additional things for subsequent versions:

1. use PATCHv2 (or PATCHv3 and so on) in the subject lines instead of PATCH,
2. add a patch changelog to the patch (for single patches) or cover letter (for patchsets),
3. and send the new patch series a reply to the previous one.

To document what changed in the new patchset, put a patch changelog at the bottom of your cover letter, just above the diffstat. A patch changelog should consist of a series of entries like this:

```txt
v2: Fixed ..., noted by Some Person <some.person@example.org>
    Reworked to use ..., as suggested by ...

v3: ...
If you're just sending a single patch, put the patch changelog after your commit message, below the -- line, but above the diffstat.
```

Finally, send your new patch or patch series as a new thread rather than as a reply to the previous patch or patch series. I.e., a new version of a patch (series) should begin a new thread.

我靠，这里我不懂，为什么一会是需要 reply ，一会是不要 reply

## b4

- https://pypi.org/project/b4/

b4 am $url
git am -i v2_20240313_fengli_nvme_tcp_export_the_nvme_tcp_wq_to_sysfs.mbx

```txt
positional arguments:
  {mbox,am,shazam,pr,ty,diff,kr,prep,trailers,send}
                        sub-command help
    mbox                Download a thread as an mbox file
    am                  Create an mbox file that is ready to git-am
    shazam              Like b4 am, but applies the series to your tree
    pr                  Fetch a pull request found in a message ID
    ty                  Generate thanks email when something gets merged/applied
    diff                Show a range-diff to previous series revision
    kr                  Keyring operations
    prep                Work on patch series to submit for mailing list review
    trailers            Operate on trailers received for mailing list reviews
    send                Submit your work for review on the mailing lists
```
原来 b4 也可以 send

使用 b4 下载一个 patch
```txt
b4 am -o- https://lore.kernel.org/lkml/20230810081319.65668-5-zhouchuyi@bytedance.com/\#r > a.diff
```

### 使用 b4 下载 patch 如何才可以将各个 patch 都分开?

## 记录

```txt
px git send-email --to mike.kravetz@oracle.com, muchun.song@linux.dev, akpm@linux-foundation.org --cc linux-mm@kvack.org
```

错误，这里不能增加括号。

直接使用 all 吧

mutt 每次抄送的人:

```txt
linux-raid@vger.kernel.org pmenzel@molgen.mpg.de song@kernel.org
```

```txt
akpm@linux-foundation.org linux-mm@kvack.org muchun.song@linux.dev
```

```txt
px git send-email --to song@kernel.org,djeffery@redhat.com,dan.j.williams@intel.com,neilb@suse.de,akpm@linux-foundation.org,shli@fb.com,neilb@suse.com -cc linux-raid@vger.kernel.org
```

## 如何增加 reviewed by

下载下来邮件，自动增加，然后 format 上

## 进一步的讨论

- https://news.ycombinator.com/item?id=24935979

## 一些经验
一次只是 fix 一个问题，将 patch 拆开

## 代办
2. 不知道为什么, 有时候邮件无法发送出去
   https://lore.kernel.org/virtualization/3b76343b-bc54-f704-b567-c586002190ea@redhat.com/T/#u
3. 格式化自己的代码片段，目前就是使用 nvim 的格式化的代码的方法，但是真正正确的写法是什么，我不知道

## 记得看下 Documentation/process/submit-checklist.rst

## 搞一个脚本，自动的将所有的 patch 都编译一次

## 操作的一些感触

1. fix tag 是容易遗忘的地方，而且麻烦的地方，重要

- 麻烦 : checkout 到特定的 commit 之后，代码难以分析了
- 重要 : fix tag 让其他人知道如何正确的 backport patch 的
- 应对方法 : 不要使用 vim 的 :Gitblame 作为最终定论，完整的 checkout 过去
- 暂时没有什么好办法，只能如此了。

2. commit msg

- 各种情况都需要解释清楚才可以

## IRC

https://www.bilibili.com/video/BV1ca411K7FH/
https://kernelnewbies.org/IRC

## patch 清理，搞两个是个熟悉下流程，不要太过分

- https://lwn.net/Articles/943039/
- https://github.com/kuba-moo/ml-stat

搞太多 typo fix 之类 patch 会被别人批评的。

## TODO

https://subspace.kernel.org/etiquette.html : 这个东西的来源是什么? 为什么又存在一个相关的地址。

https://blog.51cto.com/u_15127420/3292855

## 有趣的对话
bcachefs 的作者是直接将 patch 发送给 linus 的，被批评了，但是 ...
https://lore.kernel.org/all/2uuhtn5rnrfqvwx7krec6lc57gptqearrwwbtbpedvlbor7ziw@zgbzssfacdbe/

## kernel dev 自己的 github dashboard

https://www.remword.com/kps_result/index.php

作者很 nice 的，可以把自己的公司加进去.

## 哈哈，只要你在一个模块中总是被 cc ，那么就可以加入为 reviewer 了
https://lore.kernel.org/all/20240220073851.865113-1-chengming.zhou@linux.dev/T/#u


## review github 的 patch
https://github.com/danobi/prr

## 低级错误避免

https://lore.kernel.org/all/mdf6u6quk6khvdqtvlc3w3ppynsvornfg7hycyqhbowdcsyxnc@7gjaz5mxqds6/
> I usually use `--codespell` with ./scripts/checkpatch.pl

## damon 的规则
而且提供了一些工具:
https://www.kernel.org/doc/html/latest/mm/damon/maintainer-profile.html

## 不喜欢这样
- https://www.phoronix.com/news/Russian-Linux-Maintainers-Drop
- https://www.phoronix.com/news/Linus-Torvalds-Russian-Devs
  - https://jhuo.ca/post/opensource_freedom_forever/
  : 看看这个

## 看看这个
https://github.com/nmenon/kernel_patch_verify

## 这个可以用吗?
https://news.ycombinator.com/item?id=41321981

## 看看这个
https://subspace.kernel.org/etiquette.html#

## 似乎 https://patchew.org/linux 可以有更好的效果

https://github.com/patchew-project/patchew

## 总是在讨论这个问题
https://github.com/Rust-for-Linux/linux/issues/1107

看看这里的 b4 的讨论

Himalaya: CLI to Manage Emails (github.com/pimalaya)
https://news.ycombinator.com/item?id=42366025

## 参考一下勇哥的操作
https://blog.csdn.net/huang987246510/article/details/118343051

https://mp.weixin.qq.com/s/p7gg5c9W9Vj1uAMZo1YybQ

## 有趣
https://mp.weixin.qq.com/s/b_sfrZkOMvX0iSGfTsYUGg

https://mp.weixin.qq.com/s/1yGSdkmG_-Gzl_LqFLMLEg : 还是没看懂

https://news.ycombinator.com/item?id=45490652

## 看看 MAINTAINERS 这个文件吧

各个子项目的 repo ，网站
而且发现有好多项目都是部署在 github 上的

https://mp.weixin.qq.com/s/sSFJlxf-Hi_mYEs4arFB7Q

## 都是类似的东西了
https://github.com/neovim/neovim/blob/master/CONTRIBUTING.md#merging-to-master

## qemu 社区
docs/devel/submitting-a-patch.rst
docs/devel/submitting-a-pull-request.rst

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
