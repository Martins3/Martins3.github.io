## sleep 的等级
- https://docs.kernel.org/admin-guide/pm/sleep-states.html

### 介绍各种 /sys/power 的接口

## suspend 的时候
- 大致流程:
  - https://askubuntu.com/questions/792166/how-does-exactly-suspend-works-in-ubuntu
  - https://docs.kernel.org/admin-guide/pm/suspend-flows.html ：更加详细

1. 为什么 gnome 可以可以来设置系统的 suspend 时间
2. 而且 gnome 可以设置 power button 是关机还是 suspend
3. 通过按钮实现的 suspend 和通过 idel 的 suspend 是什么关系

- systemctl suspend
- systemctl hibernate

感觉 suspend 技术在 qemu 中不是太正常啊。


- systemd 中是什么角色
- kernel driver 的如何处理 pm 之类的
