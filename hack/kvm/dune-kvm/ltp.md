# ltp

- [ ] apicmds 是做什么的 ?

## 读读文档

Various callbacks can be set by the test writer, including
+test.test_all+, which we have set to +run()+. The test harness will execute
this callback in a separate process (using +fork()+), forcibly terminating it
if it does not return after +test.timeout+ seconds.

./runltp -f syscalls

/home/maritns3/core/loongson-dune/ltp_dir/ltp/lib/README.md :

    library process
    +----------------------------+
    | main                       |
    |  tst_run_tcases            |
    |   do_setup                 |
    |   for_each_variant         |
    |    for_each_filesystem     |   test process
    |     fork_testrun ------------->+--------------------------------------------+
    |      waitpid               |   | testrun                                    |
    |                            |   |  do_test_setup                             |
    |                            |   |   tst_test->setup                          |
    |                            |   |  run_tests                                 |
    |                            |   |   tst_test->test(i) or tst_test->test_all  |
    |                            |   |  do_test_cleanup                           |
    |                            |   |   tst_test->cleanup                        |
    |                            |   |  exit(0)                                   |
    |   do_exit                  |   +--------------------------------------------+
    |    do_cleanup              |
    |     exit(ret)              |
    +----------------------------+

> ### Test library and exec()
> 
> The piece of mapped memory to store the results to is not preserved over
> exec(2), hence to use the test library from a binary started by an exec() it
> has to be remaped. In this case the process must to call tst\_reinit() before
> calling any other library functions. In order to make this happen the program
> environment carries LTP\_IPC\_PATH variable with a path to the backing file on
> tmpfs. This also allows us to use the test library from shell testcases.


## 分析一下代码
- fork_testrun
  * run_tcases_per_fs
  * tst_run_tcases
  - testrun : 首先会调用一下 fork 维持生活
    - do_test_setup
    - run_tests
    - do_test_cleanup

所以，在 testrun 的位置添加一个 config 判断，然后生成两个链接库，对于每一个 syscall 生成两个二进制文件来执行。

- [ ] 但是，是存在那种可以执行所有 syscall 测试的脚本，修改一下这个脚本。

## 使用 debug 来分析一下

```
gcc  -I../../../../include -I../../../../include -I../../../../include/old/ -g -O2 -g -O2 -fno-strict-aliasing -pipe -Wall -W -Wold-style-definition   -L../../../../lib
 abort01.c   -lltp -o abort01
echo CC testcases/kernel/syscalls/abort/abort01
```

- [ ] 制作出来两个动态链接库出来?

- [ ] 使用两次链接的方法

## Makefile 的依赖路径


- abort:Makefile
  - include/mk/testcases.mk : 定义了生成 libltp 的方法
    - include/mk/env_pre.mk : 各种全局变量的定义
    - include/mk/functions.mk : 定义两个函数
      - generate_install_rule
      - get_make_dirs
  - include/mk/generic_leaf_target.mk
     - include/mk/generic_leaf_target.inc
     - include/mk/env_post.mk : 定义各种各种正在测试的变量相关的数据

/home/maritns3/core/loongson-dune/ltp_dir/ltp/include/mk/lib.mk 定义 libltp 被 ar 的位置

正确的方法是，将含有 DUNE flag 的文件和不含有 dune 的文件当做一个文件，分别处理到 tst_test ltp 和各个普通的文件

- [ ] 也许并不是所有的文件都和 ltp 一样简单的

- [ ]  /home/maritns3/core/loongson-dune/ltp_dir/ltp/include/mk/generic_leaf_target.inc 的

## 结论

lib.mk 是提供给不同的

## 使用 UnixBench 测试性能
