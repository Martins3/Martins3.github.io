# Mac

## 我的确尝试用过

不得不承认，Mac 有一些优点的:

- 使用 ARM 指令集，我感觉 x86
  为了兼容历史，指令集太恶心了，难学，难用，难以实现。（2025-12-23 更新，是我年轻了，arm 也不容易，手册一万多页）
- 续航时间长，去公司上班实际上是可以不用带充电器的，也就是大约 10
  小时，而小米的笔记本最多只有 4 ~ 5 小时。
- 安装 Slack ，WeChat 非常容易。

还有一些优点，但是对于我没有什么意义:

- 性能更好，编译更快，但是我主要编译 Linux 和
  QEMU，前者编译不出来，后者编译的选项有问题，而且在 server 上编译速度更快。
- 屏幕是 4k 的，但是屏幕太小了，不连外接显示器没法用。

但是也存在很多问题或者我难以习惯的地方:

- 合上盖子，必须插上电源。
- 无线鼠标，和无线键盘非常卡，几乎没有办法用[^1][^2]，我花了 20
  块钱买了一个有线的鼠标。
- 没有原生的 nfs 支持，所以没有 sshfs，我只好使用 syncthing 来同步。
- Ctrl Win Alt 键的重新定义，目前只会在外接键盘上使用电脑。

## 价格昂贵

在 2024 年，Apple 居然还在售卖 8G 内存 + 256 SSD 的笔记本电脑，并且说得益于
MacOS 和统一内存，8G 内存可以当作 16G 内存使用。 但是你要知道，1w
的价位，你很容易地可以购买到 64G 内存 + 2T 固态 + 13900HX 。不得不说，Only Apple
can do 。

## 难以个性化

似乎 Apple 从 iPhone 的成功中得到一个经验，那就是用户不知道自己想要什么。
现在其将这种想法放到其他的产品，认为用户不知道如何使用电脑，按照它的方式去使用就好了。

Apple 想要构建一个封闭的生态，他在在这个生态是神一样的存在，让其他厂商连喝汤的机会都没有。对此，我想说，得道多助，失道寡助。
相似的，还有一个公司，叫做英伟达，虽然英伟达的确如日中天。

由于在社交媒体上见过太多鼓吹 Mac 的人，以至于 Mac 的用户已经给我留下了一个刻板印象: 精致，干净但是缺乏对于问题的深入理解的乖宝宝。

## 无法玩游戏

## 解决办法 1 : Asahi

总之，对于我长期使用 Linux 桌面环境的用户而言，且工作学习和 Linux
关联性很强的用户，Mac 几乎没有任何价值。 在 2024 年，Linux
有原生的微信支持。对于我来说，企业微信和腾讯会议在 Windows 上用起来更加丝滑。
使用 Asahi linux ，安装过程非常简单:

![](./img/asahilinux.jpeg)

因为 Apple 没有对应的硬件手册，全靠社区逆向，内核为了支持 Apple
，不得不增加一些窒息的代码 例如为了 Mac 的 nvme 驱动
[drivers/nvme/host/apple.c](https://github.com/torvalds/linux/blob/master/drivers/nvme/host/apple.c)

## 解决办法 2 : Gnome + NixOS
- https://world.hey.com/dhh/linux-as-the-new-developer-default-at-37signals-ef0823b7

[^2]: 不过对于 Apple 的鼠标应该是没有问题的，那将是另外的一笔巨款。
[^3]: https://stackoverflow.com/questions/52801814/this-syntax-requires-an-imported-helper-but-module-tslib-cannot-be-found-wit

## 折腾过的地方

### bash 版本过低
- 似乎必须使用 homebrew 中的 bash https://github.com/jitterbit/get-changed-files/issues/15
- 使用这个方法切换: https://johndjameson.com/blog/updating-your-shell-with-homebrew

### 获取 ip addr
- 打开 https://superuser.com/questions/104929/how-do-you-run-a-ssh-server-on-mac-os-x
- https://www.hellotech.com/guide/for/how-to-find-ip-address-on-mac

1. neovim 插件 markdown-preview 不能正常工作

### 安装 nerdfont
- https://gist.github.com/davidteren/898f2dcccd42d9f8680ec69a3a5d350e

### 安装 Linux 虚拟机
虽然各种发行版的 server 版本都可以安装，但是只有 Ubuntu 桌面版支持的比较好 [^4]

首先安装 Ubuntu Server，然后手动安装桌面环境:
```sh
sudo apt update && sudo apt upgrade
sudo apt install ubuntu-desktop
```

### 目前为未解决的问题
- 应该是环境变量的问题，kitty 必须从 iterm 中启动， 可以使用 open $(which kitty) 来测试
- kitty 中文渲染不正常，可以确认不是软件的问题，因为插上外接显示器之后，字体就渲染不正常了。
- 暂时没有安装上 nixpkgs；


### 使用 kitty 作为 terminal 是更好的
https://unix.stackexchange.com/questions/500072/how-do-i-copy-and-paste-with-kitty

因为 kitty 可以自动处理掉 copy and paste 的问题

### 快捷键修改

#### 三个关键修饰键的修改
键盘-> 快捷键 -> 修饰键

在 windows 键盘中:
Ctrl win alt

修改为
contorl control command option

1. 所以现在的不同就是 command c / command v  不是 control c 和 control v
2. fn 对于我基本没有什么作用，直接替换为 ctrl ，这样的话就可以用手掌来按 ctrl 了

#### 调换 excape 和 caps 的问题
可以直接在系统中设置，让 caps 配置为 escape 的功能
https://stackoverflow.com/a/40254864

#### 额外的配置

~安装 [Amethyst](https://github.com/ianyh/Amethyst)，然后使用 Shift Alt J 可以替换实现 Ubuntu 中 Alt Tab 的功能；~
~使用 Ctrl Shift Left 和 Ctrl Shift Right 切换桌面。~

1. 配置 `显示启动台` 为 control p : 解决两个显示器的时候，如果使用手势打开的时候，启动台总是会显示在笔记本的屏幕上
2. 配置 `` 为 contorl l : 更加快速的切换程序
3. ctrl 1 -4 : 切换工作区

## 配置
- 减弱动态效果

- 似乎打开前台任务可以解决 ctrl l 失效的问题

## 安装 nix

[^4]: https://askubuntu.com/questions/1405124/install-ubuntu-desktop-22-04-arm64-on-macos-apple-silicon-m1-pro-max-in-parall
[^5]: https://github.com/noah-nuebling/mac-mouse-fix

## 向日葵无法启动
sunlogin
https://yanyunfeng.com/article/8

此外，这个人的 blog 也挺有意思的

## 暂时无法解决的问题
1. suspend 之后，快捷键消失
2. 打开 qq 音乐，发热严重

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
