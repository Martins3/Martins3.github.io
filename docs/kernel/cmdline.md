# 总结常用的 kernel cmdline

官方文档: https://www.kernel.org/doc/html/latest/admin-guide/kernel-parameters.html


- https://askubuntu.com/questions/716957/what-do-the-nomodeset-quiet-and-splash-kernel-parameters-mean
- [ ] http://happyseeker.github.io/kernel/2018/05/31/about-spectre_v2-boot-parameters.html
- [ ] quiet
- [ ] idle=poll
- [ ] ro
- [ ] 这里的 : https://make-linux-fast-again.com/

```c
noibrs noibpb nopti nospectre_v2 nospectre_v1 l1tf=off nospec_store_bypass_disable no_stf_barrier mds=off tsx=on tsx_async_abort=off mitigations=off
```
- 一些解释https://linuxreviews.org/HOWTO_make_Linux_run_blazing_fast_(again)_on_Intel_CPUs
- nomsi
