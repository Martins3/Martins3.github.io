## [一个关于stack的问题](www.google.com)
```
#include <stdio.h>
int main(int argc, char *argv[])
{

  int b;
  struct A{
    int a;
    int b;
  };

  struct A s;
  s.a = 12;
  s.b = 13;

  int * x = &b;
  x --;
  printf("%p\n", &b);
  printf("%p\n", &s);

  printf("%d\n", *(x));

  return 0;
}

#include <stdio.h>
int main(int argc, char *argv[])
{

  int b;
  struct A{
    int a;
  };

  struct A s;
  s.a = 12;

  int * x = &b;
  x --;
  printf("%p\n", &b);
  printf("%p\n", &s);
  printf("%p\n", &s.a);
  printf("%p\n", x);


  printf("%d\n", *(x));

  return 0;
}



```
本来的想法是 如果b 的下面就是 struct A s ，但是实际上并不是的，当A 中间含有两个元素时候，中间含有一个空的，当A 中间含有偶数个成员的时候，中间没有空缺。
```
struct X{
  int b;
  int a;
  int c;
};
```
关于对其长久忽视的问题是，对于long long int为什么也是会有 对齐问题。 既然内存总是按照32bit 访问的。




## [体系结构的新的黄金时代](https://californiaconsultants.org/wp-content/uploads/2018/04/CNSV-1806-Patterson.pdf)
似乎提供了原本的视频。
https://ieeexplore.ieee.org/xpl/mostRecentIssue.jsp?punumber=8401306　所以ISCA 到底是什么类型会议。

## [如何学习芯片设计](https://news.ycombinator.com/item?id=19890949)

## [gameboy](https://github.com/HFO4/gameboy.live)
一起来　van 游戏

## [Go language 风格的C 并发编程](http://libmill.org/index.html)
似乎可以通过这一个库获取　到底go 是如何实现并发的编程的了
https://news.ycombinator.com/item?id=19879679



## [Intel 芯片漏洞](https://news.ycombinator.com/item?id=19911277)
类似的报告
https://www.cyberus-technology.de/posts/2019-05-14-zombieload.html
## [constexpr 来定位错误](https://shafik.github.io/c++/undefined%20behavior/2019/05/11/explporing_undefined_behavior_using_constexpr.html)

## [自造CPU](http://mycpu.thtec.org/www-mycpu-eu/index1.htm)
围观一下即可


## [为什么Rust的二进制文件如此之大](https://lifthrasiir.github.io/rustlog/why-is-a-rust-executable-large.html)

## [zero cost abstractions](https://boats.gitlab.io/blog/post/zero-cost-abstractions/)

## [2019 内核会议文章](https://lwn.net/Articles/lsfmm2019/)

## [Rust 内存安全](https://kkimdev.github.io/posts/2019/04/22/Rust-Compile-Time-Memory-Safety.html)

## [为什么OO是傻吊模型](http://www.cs.otago.ac.nz/staffpriv/ok/Joe-Hates-OO.htm)
首先回顾一下到底什么叫做面向对象，使用C++为例子。
https://github.com/huihut/interview#%E9%9D%A2%E5%90%91%E5%AF%B9%E8%B1%A1

当说明面向对象的时候，主要为继承(继承，组合)，封装，多态(overload 和 override)

没有看懂的东西
1. 聚合类(Aggregate classes)
2. 一个类（无论是普通类还是类模板）的成员模板（本身是模板的成员函数）不能是虚函数
3. 虚继承

## [go 块的5个原因](https://dave.cheney.net/2014/06/07/five-things-that-make-go-fast)

## [rust 实现shell 自动补全](https://www.joshmcguigan.com/blog/shell-completions-pure-rust/)

## [RISC-V 体系结构介绍](https://twilco.github.io/riscv-from-scratch/2019/03/10/riscv-from-scratch-1.html)

## [公司创始人推荐的书籍](https://postmake.io/books)
1. The learn Startup 推荐
2. Zero to One 不推荐看书，但是推荐https://zhuanlan.zhihu.com/p/27224469 提供的课程
3. traction
3. The 4-Hour Workweek 还行，推荐如何实现高效的工作。
4. 未完待续，但是已经很多了。

最近，我其实一直在提到一件事情，那就是到底怎么才可以跳出当前的状态，感觉从此处入手也是不错的。
但是，有没有一种可能，我们只是为了形成谈资而已。
当前的问题就是如何将各种浪费时间的事情被替换成为这一件事情而已。

## [世界人口变化统计](https://ourworldindata.org/world-population-growth)
世界人口并不是指数增长的，而是在1962年实现了2%的最高增长率，此后人口增长率一直在下降。从1960开始，平均13年人口增长10亿人。
全球人口增长率最高的是oman(也门)。
population momentum指的是虽然出生率下降，但是处于生育年龄的女性(women in the reproductive age bracket)增加，出生的婴儿数量并不会快速下降。
总体而言，中国人口含有少量的外迁。

## [微软的复兴](https://www.bloomberg.com/news/features/2019-05-02/satya-nadella-remade-microsoft-as-world-s-most-valuable-company)

## [Vim 的技巧](https://www.hillelwayne.com/post/intermediate-vim/)

## [重新思考文件](https://www.devever.net/~hl/objectworld)
Everything is file 是一个正确的哲学吗 ?

## [linux 程序性能](https://unixism.net/2019/04/linux-applications-performance-introduction/)
通过各种版本的server 显示 提高性能的方法。

## [Morse理论简介](http://bastian.rieck.me/blog/posts/2019/morse_theory/)
说实话，病的一步一步的治，从看hacker news开始。

## [万物皆](http://wiki.c2.com/?EverythingIsa)

## [Monkey语言](https://monkeylang.org/)
如何使用go实现解释器

## [chrome :　典型的卑鄙小人](https://alexdanco.com/2019/05/30/google-chrome-the-perfect-antitrust-villain/)

## [计算机领域中间的相似题目](http://www.kmjn.org/notes/colons_in_dblp_titles.html)

## [羽毛出现在鸟之前](https://phys.org/news/2019-06-feathers-birds.html)

## [作为交通员的狒狒](https://en.wikipedia.org/wiki/Jack_(baboon))

## [命令行参数如何被实现的](http://daviddeley.com/autohotkey/parameters/parameters.htm)

## [可视化编程](https://divan.dev/posts/visual_programming_go/)

## [如何绘制动物](http://dessinoprimaire.blogspot.com/2012/02/les-animaux-tels-quils-sont.html)

## [技术泡沫是否开始破裂了 ?](http://charleshughsmith.blogspot.com/2019/06/is-tech-bubble-bursting.html)
内容值得一看，英语更是值得学习

## [开源google](https://www.kerkour.fr/blog/bloom-a-free-and-open-source-google/)

## [为什么semantic使用haskell](https://github.com/github/semantic/blob/master/docs/why-haskell.md)

## [印度的语言](https://en.wikipedia.org/wiki/Languages_of_India)

## [512byte的游戏](https://news.ycombinator.com/item?id=20112479)

## [基因算法](https://blog.floydhub.com/introduction-to-genetic-algorithms/)

## [树莓派上的GPIO](https://www.tomshardware.com/reviews/raspberry-pi-gpio-pinout,6122.html)

## [龙猫学术导航](http://www.6453.net/)

## [RISC-V 芯片　1G+ Hz](https://arxiv.org/abs/1906.00478)

## [自定义网址导航](https://github.com/hui-ho/WebStack-Laravel)

## [又一个cmake](https://github.com/SCons/scons)

## [用于图形处理的FPGA](https://arxiv.org/abs/1903.06697)

## [为什么不要使用即时通信](https://dpc.pw/kill-instant-messaging)

## [使用Rust语言进行创作](https://nannou.cc/)
有意思的一匹，先等我学会Rust(先等我学会C++(先等我看懂内核))

## [c++ template 编程 badge](https://awesomekling.github.io/Serenity-C++-patterns-The-Badge/)

## [使用Rust和go　实现高并发　分布式网络编程](https://github.com/pingcap/talent-plan)

## [Awesome Rust](https://github.com/rust-unofficial/awesome-rust)

## [官方提供的rust 训练](https://github.com/rust-lang/rustlings)

## [Awesome静态分析工具](https://github.com/mre/awesome-static-analysis)

## [给非设计人员的设计建议](https://www.freecodecamp.org/news/fundamental-design-principles-for-non-designers-ad34c30caa7/)
> 看看而已

## [有趣和无趣的理论的差别](https://proseminarcrossnationalstudies.files.wordpress.com/2009/11/thatsinteresting_1971.pdf)

## [如何format 浮点数](http://www.zverovich.net/2019/02/11/formatting-floating-point-numbers.html)
> 作者同时也写了 c++ 的 format 库

https://github.com/fmtlib/fmt


## [又一门语言，但是是可视化的](https://luna-lang.org/index.html)

## [比较多个语言](http://thume.ca/2019/04/29/comparing-compilers-in-rust-haskell-c-and-python/)
> 同时提供了cs444 这个好东西

## [又一个实际上显示一句话就没了的操作系统](https://arjunsreedharan.org/post/82710718100/kernels-101-lets-write-a-kernel)

## [新的vps供应商](https://www.hostwinds.com/)

## [重学css布局](https://every-layout.dev/)

## [计算机是如何工作的](https://www.cl.cam.ac.uk/~djg11/howcomputerswork/)

## [Google 想要重写libc, llvm对此给出意见](http://lists.llvm.org/pipermail/llvm-dev/2019-June/133308.html)

## [The programmer's Stone](https://www.datapacrat.com/Opinion/Reciprocality/r0/index.html)

## [AMD的技术和中国的超算](https://www.wsj.com/articles/u-s-tried-to-stop-china-acquiring-world-class-chips-china-got-them-anyway-11561646798?mod=rsswn)

## [utools](https://u.tools/index.html)
工具

## [Async io lib](https://github.com/libuv/libuv)

## [Decode gnu coreutils](https://www.maizure.org/projects/decoded-gnu-coreutils/)

## [Seven Insight on Queue Theory](http://www.treewhimsy.com/TECPB/Articles/SevenInsights.pdf)

## [branch prediction](https://danluu.com/branch-prediction/)
> 此人还提供了一系列的其他的关于CPU的文章.


## [D language is better than C++](https://news.ycombinator.com/item?id=20323114)


## [书写idempotent bash 脚本](https://arslan.io/2019/07/03/how-to-write-idempotent-bash-scripts/)
> 健壮的脚本应该是每次执行对于system的side effect 都是相同的

## [为什么复杂系统易于崩溃](https://web.mit.edu/2.75/resources/random/How%20Complex%20Systems%20Fail.pdf)
> 作者列举了18条复杂系统易于失败的原因

## [How to user mode linux](https://christine.website/blog/howto-usermode-linux-2019-07-07)
> High priority, 非常值的一看

## [为什么RSA是一个喳喳](https://blog.trailofbits.com/2019/07/08/fuck-rsa/)
> RSA 并不是一个很好的加密方式

## [分类学趣学](https://arxiv.org/abs/1803.05316)
> This book is an invitation to discover advanced topics in category theory through concrete, real-world examples. It aims to give a tour: a gentle, quick introduction to guide later exploration. The tour takes place over seven sketches, each pairing an evocative application, such as databases, electric circuits, or dynamical systems, with the exploration of a categorical structure, such as adjoint functors, enriched categories, or toposes.
> No prior knowledge of category theory is assumed.

## [shadowsocks upgrade](https://www.devtalking.com/articles/shadowsocks-guide/)

## [Boring is good for you](https://thewalrus.ca/why-being-bored-is-good/)

Boredom is one of the most common human experiences, yet it seems continually to defy complete understanding

## [SVG 实现3D](https://prideout.net/blog/svg_wireframes/)

## [元编程的艺术](https://www.ibm.com/developerworks/library/l-metaprog1/index.html)

## [V2ray 实现翻墙](https://www.codercto.com/a/22204.html)

## [Clear is better than clver](https://dave.cheney.net/2019/07/09/clear-is-better-than-clever)

## [libra 两周运行总结](https://www.facebook.com/notes/david-marcus/libra-2-weeks-in/10158616513819148/)

## [鸡汤](https://www.nytimes.com/2019/07/07/smarter-living/its-never-going-to-be-perfect-so-just-get-it-done.html?register=google)

## [data oriented design](http://www.dataorienteddesign.com/dodbook/)

## [吐槽](https://www.sicpers.info/2019/07/the-challenges-of-teaching-software-engineering/)

## [大脑的活跃程度反映了对于概念的理解程度](https://neurosciencenews.com/concept-comprehension-14296/)

## [How to debug ?](https://blog.regehr.org/archives/199)

## [社科](https://newrepublic.com/article/122765/once-joke-goes-viral-who-cares-where-it-came)

## [微软的量子计算教程](https://brilliant.org/courses/quantum-computing/)

## [打结的视频教程](https://www.animatedknots.com/complete-knot-list)

## [兴趣集](https://xinquji.com/)
正如其名称，发现新的有趣的产品

## [Slogang and Post](https://www.copenhagencatalog.org/)

## [新手的容器技术](https://www.freecodecamp.org/news/demystifying-containers-101-a-deep-dive-into-container-technology-for-beginners-d7b60d8511c1/)

## [awk 简介](https://developer.ibm.com/tutorials/l-awk1/)

## [到底什么是salesforce ?](https://tryretool.com/blog/salesforce-for-engineers/)

## [We work 工作介绍](https://www.wework.com/newsroom/posts/wecompany)

## [搭建服务器](https://ruslanspivak.com/lsbaws-part1/)

## [Alice and Bob](http://cryptocouple.com/)

## [现在有人把Deep learning 的不可知当做理论](https://en.wikipedia.org/wiki/Polanyi%E2%80%99s_paradox)
> 看来单词还是需要背

## [物理和数学的关系](http://www.damtp.cam.ac.uk/events/strings02/dirac/speach.html)

## [是时候扩张到火星上了](https://www.seas.harvard.edu/news/2019/07/material-way-to-make-mars-habitable)

## [fzf soemthing that really cool](https://news.ycombinator.com/item?id=20455857)

## [程序员应该知道的东西](http://matt.might.net/articles/what-cs-majors-should-know/)

## [Think in math, Write in code !](https://justinmeiners.github.io/think-in-math/)

## [量子计算机中间实现时间翻转](https://www.independent.co.uk/life-style/gadgets-and-tech/news/time-reverse-quantum-computer-science-study-moscow-a8820516.html)
Laboratory of the Physics of Quantum Information at the Moscow Institute of Physics & Technology (MIPT)的研究者发现可以使用量子计算机模拟时间翻转。

## [How to write thesis](https://www.amazon.com/How-Write-Thesis-MIT-Press/dp/0262527138)

## [将自己暴露于internet的url](https://serveo.net/)

## [Latex 教程](https://www.overleaf.com/learn/latex/Learn_LaTeX_in_30_minutes)

## [Linux Kernel peplple](https://people.kernel.org/about)

## [为什么需要阅读](https://goel.io/why-read/)

## [数学是不是真的有用](https://www.reddit.com/r/learnprogramming/comments/cjg8th/a_programmers_regret_neglecting_math_at_university/)

## [BootOs](https://github.com/nanochess/bootOS)

## [another vpn](https://github.com/jedisct1/dsvpn)

## [Algebra with Haskell](https://argumatronic.com/posts/2019-06-21-algebra-cheatsheet.html)

## [Compiler Resouces](http://hexopress.com/@joe/blog/2019/04/14/awesome-compiler-resources/)

## [桌面主题 awesome](https://github.com/elenapan/dotfiles)

## [Student should know](http://matt.might.net/articles/what-cs-majors-should-know/)
似乎收藏过一遍，也没有关注是给本科生还是研究生看的

## [著名程序员的工作时间](https://ivan.bessarabov.com/blog/famous-programmers-work-time)

## [手机上运行Linux](https://postmarketos.org/)

## [管理Docker的图形界面](https://github.com/jesseduffield/lazydocker)
和gotop 的界面太像了，应该是有类似的框架吧

## [youtube-dl 的图形界面版](https://github.com/mayeaux/videodownloader)

## [insight of vim](http://ismail.badawi.io/blog/2014/04/23/the-compositional-nature-of-vim/)

## [Risc-V assembler and runtime simulator](https://github.com/andrescv/Jupiter)

## [又一篇 regex tutorial](https://www.janmeppe.com/blog/regex-for-noobs/)

## [git 当做云盘](https://github.com/muesli/gitomatic)
存储容量几乎无限大的硬盘，配合使用，简直无敌

## [regex tutorial](https://regexr.com/)

## [为什么现代的网页开发如此复杂](https://www.vrk.dev/2019/07/11/why-is-modern-web-development-so-complicated-a-long-yet-hasty-explanation-part-1/)

## [看一看外国人是怎么酸的吧!](https://news.ycombinator.com/item?id=20658739)

## [Standford 监狱试验就是一个渣渣](https://psyarxiv.com/mjhnp/)
## [github terminal](https://hub.github.com/)

## [各种花卉的彩笔画](https://www.c82.net/twining/plants/)

## [History and effective use vim](https://begriffs.com/posts/2019-07-19-history-use-vim.html)

## [Self refelection](https://aeon.co/ideas/why-speaking-to-yourself-in-the-third-person-makes-you-wiser)

## [Python正在统治世界](https://news.ycombinator.com/item?id=20672051)
评论中间争论激烈!

## [ktsan 检查内核中间的data race](https://github.com/google/ktsan/wiki)

## [介绍多款开源软件的书，可在线阅读，闲的时候看看不错。](http://www.aosabook.org/en/index.html)

## [可算是有fuchsia的镜像了](https://fuchsia-china.com/fuchsia-source-code-china-mirror/)

## [latex](https://andreas-zeller.blogspot.com/2017/01/twelve-latex-packages-to-get-your-paper.html)

## [让我们再黑一次oob](https://medium.com/better-programming/object-oriented-programming-the-trillion-dollar-disaster-92a4b666c7c7)

## [邮件列表工具，所以什么是邮件列表](https://listmonk.app/)

## [fast software, best software](https://craigmod.com/essays/fast_software/)

## [tool to memorize](https://github.com/monicahq/monica)

## [a dream of os](http://okmij.org/ftp/papers/DreamOSPaper.html)

## [You don't need more than one cursor in vim](https://medium.com/@schtoeffel/you-don-t-need-more-than-one-cursor-in-vim-2c44117d51db)

## [C 语言的四种写法](https://multun.net/obscure-c-features.html)
非常有意思，目前 Compatible declarations and array function parameters 尚且无法理解。
## [inline 的含义已经变得奇怪了](https://stackoverflow.com/questions/19068705/undefined-reference-when-calling-inline-function)

## [How to kill a process cleanly !](http://morningcoffee.io/killing-a-process-and-all-of-its-descendants.html)

## [syslog 是什么](https://devconnected.com/syslog-the-complete-system-administrator-guide/)

## [在线体验各种linux 发行版](https://distrotest.net/)

## [交互式的化学查阅网站](https://ptable.com/)

## [nigeria 的市场之谜](https://medium.com/@drola/the-mystery-of-market-size-in-nigeria-a7c863f537bb)

## [how to return to flow](https://codejamming.org/2019/how-to-return-to-flow)

## [一个新的shell](https://news.ycombinator.com/item?id=20783006)

## [一位大哥的博客，内容贴近底层](https://www.agner.org/digital/)

## [有一个documentation website framework](https://timothycrosley.github.io/portray/)

## [HK问题: resource to gain math intuition](https://news.ycombinator.com/item?id=20804582)

## [slack terminal](https://github.com/erroneousboat/slack-term)
无聊的搞一下，实际上没有什么意义

## [spotify cli](https://github.com/khanhas/spicetify-cli/)
装逼神器，安装了，但是不会使用，难受!

## [climate plan](https://www.gatesnotes.com/Energy/A-question-to-ask-about-every-climate-plan)

## [GRE 的评估程序有问题](https://www.vice.com/en_us/article/pa7dj9/flawed-algorithms-are-grading-millions-of-students-essays)

## [关于rust 用在kernel 上的争论](https://lwn.net/Articles/797558/)

## [latex game](https://texnique.xyz/)

## [You are afraid](https://forge.medium.com/youre-not-lazy-bored-or-unmotivated-35891b1f3376)

## [WTF](https://wtfutil.com/)

## [Why the Periodic Table of Elements Is More Important Than Ever](https://www.bloomberg.com/news/features/2019-08-28/the-modern-triumph-of-the-periodic-table-of-elements)

## [RSA 讲解，使用代码的分析!](https://eli.thegreenplace.net/2019/rsa-theory-and-implementation/)

## [regex game](https://regexcrossword.com/)

## [美国人又在号召大家不要发展畜牧业](https://www.abc.net.au/news/science/2019-08-08/ipcc-report-climate-change-land-use/11391180)

## [可乐使用al 作为包装而不是 palastic](https://www.bloomberg.com/news/articles/2019-08-13/coke-putting-dasani-water-in-cans-amid-backlash-against-plastic)

## [Tesla 的太阳能瓦片](https://sustineri.life/a-new-kindergarten-that-is-built-without-walls-what-is-the-reason-behind-this-spectacular-design/)

## [随机数转化为图片](https://remysharp.com/2019/08/06/predictably-random)

## [linux 上的android 模拟器](https://anbox.io/)

## [What's Devops](https://ilhicas.com/2019/08/11/What-you-as-a-Devops.html)

## [Google create a IR called MLIR](https://ai.google/research/pubs/pub48035)

## [samsung key value ssd](https://www.anandtech.com/show/14839/samsung-announces-standardscompliant-keyvalue-ssd-prototype)

## [美化terminal的工具，感觉还不错的](https://github.com/starship/starship)

## [days are long but decades are short](https://blog.samaltman.com/the-days-are-long-but-the-decades-are-short)

## [loongson has been forgotten by the whole world !](https://news.ycombinator.com/item?id=20914927)

## [如何让自己戒掉戒不掉的程序](https://www.detoxify.app/)

## [logs for shell](https://github.com/7ippo/logForShell)

## [go lang 教程](https://www.yuque.com/ksco/ogg7um)

## [全球火力发电分布图](https://www.carbonbrief.org/mapped-worlds-coal-power-plants)

## [基于一个个例子的 go lang 教程](https://www.yuque.com/ksco/ogg7um)

## [我们的脑袋不是多线程的](http://www.calnewport.com/blog/2019/09/10/our-brains-are-not-multi-threaded/)

## [函数式编程中的三个概念 functor applicative 和 monad](https://typeslogicscats.gitlab.io/posts/functor-applicative-monad.html)

## [how to do code review](https://blog.jez.io/cli-code-review/)

## [modern C](https://gustedt.wordpress.com/2019/09/18/modern-c-second-edition/)
看目录，其中第三章的内容其实还不错的

https://news.ycombinator.com/item?id=21006995

## [写代码绝对是最简单的事情，不然就不会拥有如此之多的人可以自学成才了](https://www.nocsdegree.com/)
自学成才

## [博客系统的搭建](https://news.ycombinator.com/item?id=20796729)

## [cheatlist by pure and clean bash](https://github.com/denisidoro/navi)

## [开源c++ ide](https://github.com/eranif/codelite)
开发还是非常活跃的，一个可以成为谈资的东西而已!

## [关于语言的争论](https://news.ycombinator.com/item?id=21036037)

## [Kung fu chess](https://www.kfchess.com/)
将原来的象棋中间添加了时间，非常的刺激，可以改装成为一个中国象棋的版本

## [riskv 版本的xv6](https://news.ycombinator.com/item?id=21040057)

## [python 教程，微软出品 自学资源](https://github.com/microsoft/c9-python-getting-started)

## [github 上的一个组织各种算法的实现](https://github.com/TheAlgorithms)

## [为什么有的社会更加具有创业精神 ?](https://papers.ssrn.com/sol3/papers.cfm?abstract_id=3449762)
https://news.ycombinator.com/item?id=21041277

深入挖掘，逐条阅读:

## [A paper about fermi](https://arxiv.org/abs/1902.04450)

## [open source seed](https://opensourceseeds.org/en)
I don't know why we can't setup organization for things like this !

## [数学家对于黑板和橡皮的迷恋](https://www.nytimes.com/2019/09/23/science/mathematicians-blackboard-photographs-jessica-wynne.html)
也没有读到什么东西，只是看英语去了，其实也是现在经常遇到的问题，理不清逻辑在哪里!


## [有人举报facebook不正当打压对手](https://www.wsj.com/articles/snap-detailed-facebooks-aggressive-tactics-in-project-voldemort-dossier-11569236404?mod=rsswn)
墙倒众人推

## [美国人开始思考自己的债务问题了](https://www.usatoday.com/story/money/columnist/2019/09/22/debt-debt-doom-america/2384550001/)

## [为什么只有美国拥有硅谷](http://paulgraham.com/america.html)
> 虽然文章是2006年写的，但是可以判断，随后的移动网络的爆发，美国人再一次占领了制高点，虽然收到一点点威胁，但是，其地位其实进一步强化的。
> 虽然很多内容是经验之谈，但是其中部分观点值得思考


6. In America Work Is Less Identified with Employment.

Gradually employment has been shedding such paternalistic overtones and becoming simply an economic exchange.
But the importance of the new model is not just that it makes it easier for startups to grow. More important, I think, is that it it makes it easier for people to start startups.

Even in the US most kids graduating from college still think they're supposed to get jobs,
as if you couldn't be productive without being someone's employee.
**But the less you identify work with employment, the easier it becomes to start a startup.**
When you see your career as a series of different types of work,
instead of a lifetime's service to a single employer, there's less risk in starting your own company, because you're only replacing one segment instead of discarding the whole thing.
> 其实可以解释为什么会有人一想到被公司辞退就恐惧的一匹，在公司中间工作只是一个segment，同样，在学校学习也只是一个segment而已，不要总是想着现在就应该做什么事情，接着下一步做什么事情就可以了。

Incidentally, America's private universities are one reason there's so much venture capital. A lot of the money in VC funds comes from their endowments. So another advantage of private universities is that a good chunk of the country's wealth is managed by enlightened investors.
## [社会科学的博客](http://headsalon.org/)

## [电脑水平分布](https://www.nngroup.com/articles/computer-skill-levels/)

## [tmux 教程](https://leimao.github.io/blog/Tmux-Tutorial/)

## [学习css 的游戏](http://cssgridgarden.com/)

## [比较不同的数据库](https://www.prisma.io/blog/comparison-of-database-models-1iz9u29nwn37)

## [美国人对于沙特的概念](https://oilprice.com/Energy/Crude-Oil/Why-The-Saudis-Are-Lying-About-Their-Oil-Production.html)

## [为什么男生更加喜欢打游戏，而女生更加喜欢微博](http://archive.is/YRQwS)
Even if women only use those sites more than men because that is where their friends are, many experts and parents say they have found that girls appear to have a greater fear of missing out, which compels them to keep up with what their friends are posting.
> 害怕被忽视

Researchers at the University of Zurich looked at how differences in brain functioning can help explain why women tend to be more prosocial—that is, helpful, generous and cooperative—than men. In the 2017 study, they hypothesized that the areas of women’s brains related to reward processing are more active when they share rewards and that those areas in men are more active when receiving selfish rewards.

## [tea bag 中间可能含有数百万的 micro-plastic](https://www.cbc.ca/news/technology/tea-bags-plastic-study-mcgill-1.5295662)

## [过度加工的食品更加容易导致肥胖](https://www.scientificamerican.com/article/a-new-theory-of-obesity/)

## [far memory data structures](https://blog.acolyer.org/2019/06/26/designing-far-memory-data-structures/)

## [光线追踪](https://alain.xyz/blog/raytracing-denoising)

## [书籍的形式对于阅读习惯的改变](https://www.thebritishacademy.ac.uk/blog/how-invention-book-changed-how-people-read)

## [魔方数学](https://web.mit.edu/sp.268/www/rubik.pdf)
> 没有时间阅读，但是应该非常的有意思的

## [英伟达发布99美元的DIY AI计算机](https://www.engadget.com/2019/03/18/nvidia-jetson-nano-ai-computer/)
## [函数式编程教程 y compbinator](https://mvanier.livejournal.com/2897.html)

## [linux 桌面发展历程](https://opensource.com/article/19/8/how-linux-desktop-grown)

## [有一个docker 入门教程](https://docker-curriculum.com/)

## [Dalai 在西方人眼中到底是什么态度](http://nautil.us/issue/76/language/consciousness-doesnt-depend-on-language)

## [小心含有多个类型相同的参数的函数](https://dave.cheney.net/2019/09/24/be-wary-of-functions-which-take-several-parameters-of-the-same-type)

## [forbes 30 under 30's](https://www.theinformation.com/articles/the-forbes-30-under-30-hustle?utm_source=hackernews&utm_medium=unlock)

## [各种语言对比](https://drewdevault.com/2019/09/08/Enough-to-decide.html)
c++ 心里苦啊!

## [在追逐新技术中间迷失自己](https://news.ycombinator.com/item?id=20916310)
新技术，新名词给人一种自己在学习的感觉，那其实只是一种自我安慰，如果新的方案没有解决问题的话!

## [Java 招聘完全指南](https://www.hackerearth.com/recruit/resources/e-books/hire-java-developer/)

## [小的操作系统代码教程](https://littleosbook.github.io/)
其中关于IO的内容还是比较有意思的，推荐给不想写ucore 的人吧!

## [Automate work flow tool](https://github.com/n8n-io/n8n)

## [c++ queue 的申明和想象的有点不同](https://stackoverflow.com/questions/10293302/why-cant-i-construct-a-queue-stack-with-brace-enclosed-initializer-lists-c1)

## [失败，到今天才发现!](http://lists.kernelnewbies.org/mailman/listinfo/kernelnewbies)

## [自己展示自己的网站](https://www.strml.net/)

## [CS的最佳书籍](https://www.coderscat.com/best-cs-books#i-4)

## [latex 符号搜索](https://latex.guide/)

## [NLP入门教学](https://easyai.tech/blog/59pdf-nlp-all-in-one/)

## [little Handbook of Statistical Practice](http://www.jerrydallal.com/LHSP/LHSP.htm)

## [异步编程](https://luminousmen.com/post/asynchronous-programming-blocking-and-non-blocking)

似乎一直都没有从本质上搞清楚这一个问题，从硬件支持，操作系统，　运行时库文件，以及用户层次

## [Unix 的文件操作](https://www.ibm.com/developerworks/aix/library/au-unixtext/index.html)
实际上，自己写c++ 实现操作并没有这一个操作好

## [Marp有一个框架](https://yhatt.github.io/marp/)
看来无论什么框架，都是需要使用CSS的

## [可视化线性代数](http://immersivemath.com/ila/index.html)

## [初级应用topology](https://www.math.upenn.edu/~ghrist/notes.html)
> 高中一直想看的书籍

## [Man watch](https://unix.stackexchange.com/questions/8699/is-there-a-utility-that-interprets-proc-interrupts-data-in-time)

## [elf](https://linux-audit.com/elf-binaries-on-linux-understanding-and-analysis/)

## [pic](https://i.redd.it/ly3a3nrltqg41.jpg)

## [给小朋友玩的项目](https://github.com/Rapiz1/DungeonRush)


## [rust 的24天学习之路](http://zsiciarz.github.io/24daysofrust/book/vol1/day4.html)

Rust,绝对不是浅尝辄止的内容。

## [论文](https://arxiv.org/pdf/1904.04953.pdf)
> 没看

## [与其解决996 不如解决翻墙的问题](https://weibo.com/tv/v/Hp9hqmUGK?sudaref=github.com&display=0&retcode=6102)


## [量子计算机入门](https://quantum.country/qcvc)
> 据说其中还有好多习题

## [commit信息的重要性](https://github.com/RomuloOliveira/commit-messages-guide)
> 显然，很重要

## [一个新的语言，没有循环](https://github.com/Microsoft/BosqueLanguage)

## [c++并发编程](https://github.com/cpp-taskflow/cpp-taskflow/blob/master/awesome-parallel-computing.md)


## [forbes报道zoom的老板](https://www.forbes.com/sites/alexkonrad/2019/04/19/zoom-zoom-zoom-the-exclusive-inside-story-of-the-new-billionaire-behind-techs-hottest-ipo/#61beefb74af1)

## [1b3b的视频生成引擎](https://github.com/3b1b/manim)


## [看一看nature的文章](https://www.nature.com/articles/s41467-019-09311-w)
> 装逼而已，不要激动

## [Linux network queue](https://github.com/leandromoreira/linux-network-performance-parameters)
> 教程

## [比较C++和rust的编译安全](https://kkimdev.github.io/posts/2019/04/22/Rust-Compile-Time-Memory-Safety.html)


## [leisure is our killer app](https://sloanreview.mit.edu/article/leisure-is-our-killer-app/)


## [富人的痛苦体会不到啊!](https://www.bennettnotes.com/post/making-money-out-of-every-hobby/)

## [linux mm, newbie 的sub web](https://linux-mm.org/)

## [Fuchsia Hacker News 上的讨论](https://news.ycombinator.com/item?id=19485121)

## [100 个gcc 技巧](https://github.com/hellogcc/100-gcc-tips)

## [100个gdb 技巧](https://github.com/hellogcc/100-gdb-tips)

## [log in software engineer](https://www.slideshare.net/geshan/logging-best-practices)

## [开源视频编辑器](https://kdenlive.org/en/)


## [基于chrome的android模拟器](https://archon-runtime.github.io/)

## [latex 的作图](https://castel.dev/post/lecture-notes-2/#)



## [rust 嵌入式编程教学](https://rust-embedded.github.io/book/intro/index.html)


## [提交自己第一个patch](https://etsai.site/2019/04/18/your-first-linux-kernel-patch/)
1. 问题是什么时候，可以开始读arvix 上的操作系统文章
2. 什么时候开始提交kernel patch
## [漫画](https://code2048.com/)

## [csapp试验](http://csapp.cs.cmu.edu/3e/labs.html)
> 绝对不可以错过
> 但是还是首先完成南大试验的清理工作

## [终端里面的游戏模拟器](https://github.com/HFO4/gameboy.live)

## [MIT应用数学](https://www.harshsikka.me/self-studying-the-mit-applied-math-curriculum/)

## [llvm-mca](https://llvm.org/docs/CommandGuide/llvm-mca.html)
居然可以实现静态分析代码

## [试验小鼠价格爆涨](https://www.bloomberg.com/news/articles/2019-04-01/china-s-demand-for-17-000-gene-altered-lab-mice-is-skyrocketing)

## [论文下载神器](http://www.6453.net/)

## [各种开源资料](https://goalkicker.com/)
> 并么有检查这些资料的质量

## [动手写一个数据库](https://cstack.github.io/db_tutorial/)

## [needs an alternative web](https://www.forbes.com/sites/cognitiveworld/2019/03/15/society-desperately-needs-an-alternative-web/#6bba5f5c24e3)
文章是好文章，只是GRE还是需要背啊!

## [如何避免失败](https://blog.ycombinator.com/how-not-to-fail/)

## [使用git commit 定义的深奥语言](https://morr.cc/legit/)
傻吊网友的力量是无穷的啊!

## [a world run with code](https://blog.stephenwolfram.com/2019/05/a-world-run-with-code/)
一份演讲的记录

## [北大acm icpc](https://lib-pku.github.io/#acm-icpc%E6%9A%91%E6%9C%9F%E8%AF%BE)
看一下正宗大学的acm到底是搞什么玩意儿。

## [git bisect](https://thoughtbot.com/blog/git-bisect)

## [高性能计算介绍](http://pages.tacc.utexas.edu/~eijkhout/istc/html/index.html)

## [为什么正则分布会消失](https://alexdanco.com/2015/12/17/taylor-swift-ios-and-the-access-economy-why-the-normal-distribution-is-vanishing/)

## [stb library](https://github.com/nothings/stb)
一个文件，一个库，可以看一下，应该主要是用于教学的东西

## [The Chinese researcher painting the printing industry green](https://www.nature.com/articles/d41586-019-00886-4?from=timeline)
看一看，都是中科院，差距为什么这么大

## [org mode](https://beorg.app/orgmode/letsgetgoing/)
到底什么是org mode，如何在vim 上面部署该功能?

## [opengl 教程](http://www.opengl-tutorial.org/beginners-tutorials/tutorial-2-the-first-triangle/)
刚刚搭建完成环境，感觉是非常不错，我觉得project based learn 所有任务全部可以看一下

## [linux 的编译系统](https://www.linuxjournal.com/content/kbuild-linux-kernel-build-system)
查资料查到了，但是有必要将内核的编译，链接，装载过程搞清楚

## [继承还是组合](https://lwn.net/SubscriberLink/787800/b7f5351b3a41421a/)
是时候理解为什么继承的问题是什么了!

## [2D 图像在现代GPU中间的实现方式](https://raphlinus.github.io/rust/graphics/gpu/2019/05/08/modern-2d.html)
