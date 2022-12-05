## bisect

- https://news.ycombinator.com/item?id=32161247 : Things I wish everyone knew about Git (Part II)


- https://gitee.com/all-about-git : 有趣的总结
- https://stackoverflow.com/questions/4713088/how-to-use-git-bisect

- https://mailweb.openeuler.org/hyperkitty/list/kernel@openeuler.org/thread/3CPAMRVNTNOT2TQ4W5S7HNY2HXPBBO4G/

## TODO
- http://www.ruanyifeng.com/blog/2016/01/commit_message_change_log.html
- https://github.blog/2020-04-09-github-protips-tips-tricks-hacks-and-secrets-from-lee-reilly/ : github 使用技巧
- https://koukia.ca/how-to-remove-local-untracked-files-from-the-current-git-branch-571c6ce9b6b1 : 到底应该如何清理一个git仓库

## 好用的工具
https://github.com/commitizen/cz-cli : 学会正确的 commit
https://www.toptal.com/developers/gitignore : 纯工具，再也不用一个个的复制了
https://github.com/tipsy/profile-summary-for-github : github 个人信息生成器

https://github.com/jlord/git-it-electron : 推荐给小朋友用于学习 git


## checklist
https://github.com/521xueweihan/git-tips
https://github.com/k88hudson/git-flight-rules : 如果做成可以fuzzy search 的就很棒!



# 常用技能

2. 展示全部的项目,　即时在非最新版本上面
```
git log --reflog
```

3. 添加一个新的远程仓库
```
cd existing_repo
# git remote rename old-origin
git remote add origin git@github.com:name/reponmae
git push -u origin --all
git push -u origin --tags
```

4. status
```
git status
git reflog
git diff
```


6. remote
```
    $ git remote -v
```
7. branch

```
Create a new branch
$ git branch <BRANCHNAME>

Rename a branch you are currently on
$ git branch -m <NEWBRANCHNAME>

Delete a remote branch
$ git push <REMOTENAME> --delete <BRANCHNAME>
```

8. merge

9. 将本地分支设置为完全与远程分支相同 [link](https://stackoverflow.com/questions/1628088/reset-local-repository-branch-to-be-just-like-remote-repository-head)
```
使用 git reset

git fetch origin
git reset --hard origin/master
git clean -f -d
```



https://stackoverflow.com/questions/1470572/ignoring-any-bin-directory-on-a-git-project
11. checkout to a branch

[git checkout <sh1>](https://stackoverflow.com/questions/7539130/go-to-particular-revision)


## [stackoverflow top question](https://www.codementor.io/alexershov/11-painful-git-interview-questions-and-answers-you-will-cry-on-lybbrqhvs)
1. https://stackoverflow.com/questions/1470572/ignoring-any-bin-directory-on-a-git-project
2. https://stackoverflow.com/questions/17857723/whats-the-difference-between-git-reflog-and-log
3. https://stackoverflow.com/questions/3329943/git-branch-fork-fetch-merge-rebase-and-clone-what-are-the-differences/

# Good questions
https://stackoverflow.com/questions/1440181/how-do-i-backport-a-commit-in-git https://mirrors.edge.kernel.org/pub/software/scm/git/docs/git-cherry-pick.html


## 重学[廖雪峰](https://www.liaoxuefeng.com/wiki/0013739516305929606dd18361248578c67b8067c8c017b000/0013743256916071d599b3aed534aaab22a0db6c4e07fd0000)



## 传输协议
ssh://[user@]host.xz[:port]/path/to/repo.git/

git://host.xz[:port]/path/to/repo.git/

http[s]://host.xz[:port]/path/to/repo.git/

ftp[s]://host.xz[:port]/path/to/repo.git/


## Github Workflow 的理解
本地工作永远不要在master 上面，checkout 到一个特定的分支上面，push 之后，在github 上操作git review 的东西，如果含有修改，继续commit　然后提交，此时master 的分支领先本地分支，而且本地分支被领先，所以可以直接pull fetch 即可.
如果没有出现冲突，那么只是需要手动按一下页面上的按钮，但是如果出现冲突，使用如下流程:
```
git checkout master
git pull
git pull https://gitee.com/martins3/book.git wow
git mergetool
```


// 这一个操作gerrit 如何实现 ?
## git reset 和 git merge
git reset --hard 82323f23　清空所有内容，但是tracked 文件不变
git reset  82323f23 保持当前内容不变
git reset : 缺省选择最近的commit,其余含义相同,所以导致此命令成为将stage过的文件取消stage
git reset --hard : 如果一个文件没有被track过，hard 选项是无效的

> Resets the index but not the working tree (i.e., the changed files are preserved but not marked for commit) and reports what has not been updated. This is the default action.

> Resets the index and working tree. Any changes to **tracked files** in the working tree since <commit> are discarded.

[更加清晰的解释](https://gist.github.com/tnguyen14/0827ae6eefdff39e452b)


set the current branch head (HEAD) to <commit>,
optionally modifying index and working tree to match. The <tree-ish>/<commit> defaults to HEAD in all forms.

`git reset` 回去的方法:
git log --reflog 来找到对应的hash数值


## 查看一个文件的历史
https://stackoverflow.com/questions/278192/view-the-change-history-of-a-file-using-git-versioning
1. gitk filename
2. git log -p filename
3. SpaceVim 下: <SP> + g + V(似乎目前只能查看diff 而不能查看原文)

## git revert
将某一次commit的修改撤销
https://www.atlassian.com/git/tutorials/undoing-changes/git-revert

## git rebase
From a content perspective, rebasing is changing the base of your branch from one commit to another making it appear as if you'd created your branch from a different commit.
Internally, Git accomplishes this by creating new commits and applying them to the specified base. It's very important to understand that even though the branch looks the same, it's composed of entirely new commits.

[本文](https://www.codercto.com/a/45325.html)说明rebase的两个功能:
1. 合并细碎的commit
2. 实现类似于merge的功能



## 为什么git需要 stage(暂存区)
https://gitolite.com/uses-of-index.html

新手一般使用`git add -A; git commit` ，好像stage 没有用途一样，其实并不是。

## git diff 的三种版本
1. git diff　展示尚未stage的
2. git diff --cached　展示stage的
3. git diff HEAD 全部展示

> 如何git diff 以文件的形式展示 ?


## 放弃未stage的修改
git checkout filename
git checkout .


## 跟踪
展示所有 tracked 的文件
git ls-files -t
展示所有 untracked 的文件
git ls-files --others

## 历史
git log -n1 -p
> n表示参数个数 p表示生成patch

## 修改commit
仅仅修改commit内容:
git commit --amend --only
我需要把暂存的内容添加到上一次的提交(commit)
git commit --amend

## Unstaged的内容
untracked 文件其实算是特殊
git checkout -f

## 我想从Git删除一个文件，但保留该文件
(master)$ git rm --cached log.txt

## 两个特殊符号
Say you want to move a lot of levels up in the commit tree.
It might be tedious to type `^` several times, so Git also has the tilde `~`operator.

`^`的数目是: 回退数量，数字，回退选择parent 序号


## Checkout的含义是什么
checkout 的作用切换HEAD的位置:
checkout + branchName
checkout + tag
checkout + hashValue

可变选项: 同时创建branch, 相对偏移^ 和~

## Cherry Pick
https://stackoverflow.com/questions/9339429/what-does-cherry-picking-a-commit-with-git-mean

merge 会收割全部的commit,而 cherry-pick 仅仅添加部分的即可，不一定是添加其他分支的内容，只是将commit中间修改apply 上去.


## 解决方案
1. 在某一个commit 中间添加了print 在其他的commit 修复了代码,使用 cherry-pick 或者　rebase 实现部分 commit 合并

## Git Workflow
Man gitworkflows
> 没有读完，过于晦涩


1. 合并小的 commit 比　拆分大的 commit 更加简单


## MISC
1. [what's branch tip](https://stackoverflow.com/questions/16080342/what-is-a-branch-tip-in-git)


## HEAD
HEAD 默认在当前branch的开始位置.
当切换branch 之后，默认会后队列开始的位置.
通过checkout 实现HEAD 位置的切换

## submodule


## Log
log branchName
log tag
log hash
log filename : 仅仅显示与之有关的内容

对于log 可以 grep commit 消息，对于时间，作者, tag 等进行filter

对于log 的输出可以进行排序

`git log --pretty="format:%H"` 的内容:
https://devhints.io/git-log-format

显示到指定的branch 中间
https://stackoverflow.com/questions/7693249/how-to-list-commits-since-certain-commit

仅仅显示前面10个log
## Hook
似乎有两个含义:
1. 用于实现部分提交
2. 用于实现自动执行脚本

## track
http://www.drdobbs.com/architecture-and-design/git-tracking-relationships-use-the-full/240168881

## git push
1. git push origin master 会出现失败，当本地的master 没有办法对于远程的master 进行 fast-forward 的时候.
2. 当采用


## 搭建git 服务器
1. 服务器端
```
cd git-in-practice
git init --bare
```

2. 客户端
```
➜  git-in-practice git:(master) ✗ git remote add origin huxueshi@172.17.103.55:/home/shen/git-in-practice # 添加
➜  git-in-practice git:(master) git remote set-url origin huxueshi@172.17.103.55:/home/huxueshi/git-in-practice # 修正
➜  git-in-practice git:(master) git push origin master
```

甚至可以对于本地操作:

```sh
# systemctl restart sshd # 如果port 22不可用
git clone shen@localhost:/home/shen/Core/repo-in-practice/test
```

## 如何缩减仓库体积
https://gitee.com/help/articles/4232


## About Conficts
https://help.github.com/en/articles/about-merge-conflicts

Often, merge conflicts happen when people make different changes to the same line of the same file, or when one person edits a file and another person deletes the same file.
> 出现冲突不会是因为

Merge 的前提是必须含有公共的ancester. 如果是在同一个仓库中间，必定会存在一个父类的节点，最不济，在init 的位置，所以不同的仓库
即使内容完全相同，也是没有办法实现merge 的.
merge 的关键在于内容是否相同，而不是在于谁处于领先的状态.

如果一个分支比如(dev)，始终领先于master ，那么master 不停的合并 dev（spacevim.d ) 的操作，那么永远不会出现冲突，
出现冲突不是因为内容conflict，而是双方的commit 出现冲突。

merge 的过程中间不会出现commit 丢失, 所有的commit 会含有parent 和children 的关系，merge 的过程中间保持不变。

本地master 和 remote master　实际上完全就是两个分支，只是由于简写造成的.


fetch 的内容在哪里 : fetch 会刷新remote/xxxx/branchname 中间的内容，只是和远程同步

冲突的内容是修改的位置是否发生冲突，而不是两个版本的内容是否含有冲突，当一个分支在第一行添加了文档，第二个分支在把最后一行删除了，两者不会冲突.

## About clone
1. clone 默认针对于所有的分支，默认分支为remote当前处于的分支，比如fucker.
> 1. 如果不是clone 下所有分支，那么重新下载必定会联网，但是并不会
> 2. 解释art repo 下载下来的时候，git branch 的数目明显的不对的原因是: git clone 只会分析本地分支, 虽然远程分支下载下来了，但是没有chekcout ,　所以没有该本地分支，所以art 二级clone 的时候少量的branch


2. Branch 'master' set up to track remote branch 'master' from 'origin'.
> track 的含义是  --track  吗 ?




# 问题
2. 为什么删除文件, merge 之后还是存在，其中的原因是不是由于　git add \*(See Mips run 的某些chapter 修改名字的问题)
3. 如何实现本地和远程完全相同(本地添加的文件会删除，删除的文件会消失)
5. git rm 之类操作还有什么

## ref
https://datree.io/blog/top-10-github-best-practices/

https://ohshitgit.com/ : 常见的 git 出现问题的修复的, 学到的两点:
1. git clean 清理掉delete untracked files and directories
2. git checkout 进入到某一个 commit，对于没有 tracked 的文件不处理

## git 常用操作
- 将一个远程仓库清空为一个 commit
  - https://gist.github.com/stephenhardy/5470814

- git log --first-parent 可以实现类似 sublime merge 的操作，可以用于处理
  - [ ] 如何让 tig 的 log 实现类似的操作？

- git .. 和 ... 不同
  - https://stackoverflow.com/questions/462974/what-are-the-differences-between-double-dot-and-triple-dot-in-git-com

- 如何查看到
  - https://stackoverflow.com/questions/6191138/how-to-see-commits-that-were-merged-in-to-a-merge-commit

- 如何将远程的 tag 同步到本地
  - https://stackoverflow.com/questions/1841341/remove-local-git-tags-that-are-no-longer-on-the-remote-repository

- 如何删除远程的 tag
  - git push --delete origin tagname
