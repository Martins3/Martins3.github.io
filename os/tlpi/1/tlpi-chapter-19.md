# Linux Programming Interface: Monitor Filesystem Events

The inotify mechanism replaces an older mechanism, dnotify, which provided a
subset of the functionality of inotify. We describe dnotify briefly at the end of this
chapter, focusing on why inotify is better.

## 19.1 Overview
inotify_init : 初始化 inotify instance
inotify_add_watch/inotify_rm_watch : 添加监听
read : 读取事件
close : 关闭监听
