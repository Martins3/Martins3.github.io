# 学习计算机经验之谈

现在计算机的领域很广泛，从 FPGA 到 NLP，从网页前端到编译器后端，从自动化控制到通信工程，不同的领域对于看到的计算机内容大不相同，
学习计算机的人也从小学生到博士，从在校生到培训班等各色各样，现在我的经验也只是一家之言，如果不同意，欢迎来讨论。

## 不要迷信大学
我个人认为大陆的大学连技校的工作都没有做好, 曾经我以为是上的大学比较差，后来看池先生，发现其实国内大学
都是如此，可以参考国外的大学教程，例如 [Standford 教程](https://docs.google.com/spreadsheets/d/1zfw8nPvJeewxcFUBpKUKmAVE8PjnJI7H0CKimdQXxr0/htmlview)

一个老哥对于国外课程做一个几个评价集合，方便大家选择[合适的课程自学](https://github.com/conanhujinming/comments-for-awesome-courses), 非常不错

以华中科技大学的计算机学院为例，我可以明显的感觉出来很多问题:
- 课程安排比例不合适，最为重要的操作系统，计算机网络，数据结构算法和组成原理几乎都是一个学期上完的，其他时间都是在一些莫名其妙的课程上浪费时间。
  - 比如: 大学物理实验，抄报告就是一个无情的时间黑洞
- 课程老旧
  - 比如: 马光志的 ppt 还是 20 年前写的
- 代码 5 分钟，报告 2 小时
  - 最后的报告都是比页数

其实从一个大学教师的角度来看，本来手上的项目就要做，但是又被安排去教学，这种精力的分散让人很难受，大多数老师对于
上课的态度最后就是讲好 ppt 而已。

黄玄在他的[中国高等教育的系统性失败](https://huangxuan.me/2021/01/19/the-systematic-failure-of-higher-education-in-china/) 中阐述的一些想法
也非常不错。

不听老师的课，不影响你靠高分，也不影响你找到好工作，更不掌握好技术，你已经是一个大人了，你得靠自己，大学教育会毁了你的。

## 远离‘酷炫’的东西
不要着急做一些可以展示起来很酷炫的东西，比如前端，比如使用一个游戏引擎做一个游戏。
也没有必要急着追逐热点（我不是说热点不能做，我是说为了追逐而追逐）。我认为学习计算机最开始的时候，
先把自己变成猛人狠人，保证自己可以轻松的切入到其他的任何领域。

计算机的发展速度很快，这对从事计算机的人很友好
- 新涌入的人总是可以在新的技能点上生根发芽，不会和老人内卷
- 新技术带来新的效率提升，让计算机从业的薪资越来越高。

但是，对于学习却是一个挑战，因为学的一些东西很快就过时了，或者热点行业很快就没落了。
我在 2015 读计算机的时候，之后移动互联网就是热门，但是等到我 2022 毕业的时候，互联网就开始有点进入寒冬了。
但是计算机不代表互联网，你需要打好基础，然后就可以轻松地转入到嵌入式，芯片，区块链等其他领域。

## 学好英语，学会科学上网
如果无法科学上网，现在就停止阅读，立刻去解决。英语，一定要有意识的提高，不过学英语是一个很系统的问题，阅读是最好的老师，可以多逛逛[外国网站](https://www.buzzing.cc/)

不能科学上网意味着:
- 在 CSDN 的粪坑中无限递归
- 视野狭窄

## 无所不知
可以不去做这个领域，但是对于一个领域必须有一个基本认识, 比如花半天的时间了解一下 bitcoin 的原理。

计算机的变化是很快的，很多时候并不是你的 35 岁危机到来了，而是因为这个行业的 35 岁危机到来，
任何时候都要做好准备去接受一个你所在领域死掉的时候，当然这就是计算机很酷的原因之一了吧。

计算机中有一些领域是存在一些酷炫的思想在其中的，是没有办法两句话解释清楚的，是需要花点时间深入其中理解的
- 数据库 : 是实现原理，而不是使用
- 操作系统
- 编译器
- 图形学
- 虚拟化
- 搜索引擎
- 深度学习
- 区块链
- CPU 设计

## 动手是最好的学习方法
很多东西看看文章，总是没有办法融汇贯通，因为省略了细节, 代码一看，自然清晰。

- 把代码从 github 上 clone 下来
  - 项目可以从 project-based-learning / build-your-own-x 中间找，都是一些小例子
- 运行一下
- 大致分析分析，尝试的谢谢代码的文章，整理一下代码的架构


不同的源码阅读难度不同，Linux 内核算是我见过非常有挑战的，但是也是收获最多的。

## 工欲善其事, 必先利其器
很多大学老师一方面总是有意无意的表现出来对于各种编程工具的轻视，认为自己的任务是教授原理，实际上自己拿着十几年前 slides 在哪里重复早已过时的内容，
动手能力几乎为零。

- 将自己的操作系统换成 Linux，不要过多的使用 Windows，越早越好
- 学好 [mit 的这个课程](https://missing-semester-cn.github.io/)

## 附录

https://wangzhe3224.github.io/2021/10/20/roadpath/#more

似乎这个才是真正落到实处的吧:
https://github.com/PKUFlyingPig/cs-self-learning


关于如何自学，已经有非常多的参考资料，我觉得写的都非常好：
- https://github.com/ossu/computer-science
- https://github.com/keithnull/TeachYourselfCS-CN
- https://teachyourselfcs.com
- https://news.ycombinator.com/item?id=21920205
- https://news.ycombinator.com/item?id=19423228
- https://www.nocsdegree.com/
- https://why.degree/motivation/
- https://github.com/PKUanonym/REKCARC-TSC-UHT
- https://github.com/QSCTech/zju-icicles
- https://github.com/SSHeRun/CS-Xmind-Note
- https://github.com/Developer-Y/cs-video-courses#systems-programming
- https://github.com/niudai/How-to-be-a-good-programmer : 经典电子书
- [学习编程的 12 个游戏](https://medium.mybridge.co/12-free-resources-learn-to-code-while-playing-games-f7333043de11#.xi83hlivu)

- https://github.com/tuvtran/project-based-learning :star:
- https://github.com/danistefanovic/build-your-own-x :star:
- https://github.com/charlax/professional-programming
- https://github.com/trimstray/the-book-of-secret-knowledge
- https://github.com/EbookFoundation/free-programming-books
- https://github.com/justjavac/free-programming-books-zh_CN
- https://github.com/wx-chevalier/Developer-Zero-To-Mastery
- https://github.com/donnemartin/interactive-coding-challenges
- https://github.com/donnemartin/system-design-primer

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

[^1]: https://huangxuan.me/2021/01/19/the-systematic-failure-of-higher-education-in-china/
