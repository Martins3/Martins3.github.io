# wine : 如何实现系统调用虚拟化

大约四年前，我尝试过在 Linux 上使用 wine 打英雄联盟[^1]，但是只能使用外服的，而最近发现似乎国服也可以[^2]。
目前而言，这种方法不太实用。下面主要来记录一下 wine 如何翻译一个 Windows 程序。

[^1]: https://www.jianshu.com/p/d743a3405eef
[^2]: https://snapcraft.io/leagueoflegends
