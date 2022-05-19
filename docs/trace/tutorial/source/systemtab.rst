================
SystemTab
================

总结
----


手动安装
--------

官方的网站: https://sourceware.org/systemtap/wiki/SystemtapOnArch 上的信息显然过时了，**手动安装还是可以尝试一下**

问题是参考 https://wiki.archlinux.org/index.php/SystemTap 的，**结果没有asp**，解决方法是
cat /proc/config.gz | gunzip > .config
然后切入到内核中间，希望之后可以利用 pacman 进行安装，如果不可以，那么就使用普通方法安装吧!

编译的中间需要注意的:
➜  linux git:(7111951b8d49) ✗ yaourt -S pahole

编译成功之后，make 和 make install

asp 或者 https://wiki.ubuntu.com/Kernel/BuildYourOwnKernel

.. todo::
  1. 根本不知道下面两个的具体含义
    make modules_install
    make install
  2. 使用 mkpkg 的安装方法
  3. debug info 中间的东西到底是什么 ?
  4. 也不知道为什么需要
  5. dkms 是什么东西呀


自动安装
--------
https://medium.com/@evintheair/building-a-custom-kernel-in-manjaro-linux-186da6a1cedf


.. todo::
  关于其中的网络驱动的部分，感觉莫名奇妙.

安装测试函数:
.. code:: sh
  sudo stap -v -e 'probe vfs.read {printf("read performed\n"); exit()}'


虽然有点问题，但是opensnoop 没有问题
1. cat /proc/config.gz | gunzip | grep DEBUG_INFO 中间的结果显示暂时没有任何问题!
2. 利用 ln -s   libjson-c.so.5.0.0 libjson-c.so.4 将其中
::

  ➜  trace-cmd git:(master) ✗ sudo stap -v -e 'probe vfs.read {printf("read performed\n"); exit()}'
  [sudo] password for shen:
  WARNING: Kernel function symbol table missing [man warning::symbols]
  Pass 1: parsed user script and 477 library scripts using 108620virt/88948res/11092shr/77648data kb, in 170usr/20sys/182real ms.
  WARNING: Potential type mismatch in reassignment: identifier 'root_dentry' at /usr/share/systemtap/tapset/linux/dentry.stp:246:3
   source: 		root_dentry = @cast(task, "task_struct")->fs->root->dentry
              ^
  WARNING: Potential type mismatch in reassignment: identifier 'root_dentry' at :242:3
   source: 		root_dentry = & @cast(task, "task_struct")->fs->root
              ^
  Pass 2: analyzed script: 2 probes, 1 function, 5 embeds, 0 globals using 344920virt/327516res/13272shr/313948data kb, in 2340usr/260sys/2618real ms.
  Pass 3: using cached /root/.systemtap/cache/c9/stap_c9d1e6fc044f00df6e422a7ca9cd1f30_2760.c
  Pass 4: using cached /root/.systemtap/cache/c9/stap_c9d1e6fc044f00df6e422a7ca9cd1f30_2760.ko
  Pass 5: starting run.
  read performed
  Pass 5: run completed in 30usr/40sys/574real ms.


.. todo::
  1. https://gitlab.manjaro.org/packages/core/linux44/-/blob/master/PKGBUILD 关于打包，非常好奇
  2. 为什么需要打包，make install 不香气吗 ?
  3. 为什么需要向内核增加 patch，这些 patch 都说了什么


实现原理
--------
根据 https://jvns.ca/blog/2017/07/05/linux-tracing-systems/ 的描述，
1. 写一个 systemtap 程序，并且将其编译到内核中间
2. 该内核模块注册 kprobe
3. 最后将数据导出到用户空间
