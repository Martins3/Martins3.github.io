# attr && xattr

fs/xattr.c 中提供相关的系统调用:
- setxattr
- lsetxattr
- fsetxattr

- getxattr
- fgetxattr
- lgetxattr

## 工具
chattr

```sh
file=/mnt/a.txt
rm -f $file
touch $file
python3 -c "import os; os.setxattr('$file', 'user.lek', b'value', os.XATTR_CREATE)"
python3 -c "path = '$file'; import os; print([(attr, os.getxattr(path, attr)) for attr in os.listxattr(path)])"
# 得到这个结果
# [('user.lek', b'value')]
```

这个动作如果在 ext2 上执行:
```txt
Traceback (most recent call last):
  File "<string>", line 1, in <module>
OSError: [Errno 95] Operation not supported: '/mnt/a.txt'
```
因为当时的 kernel config 没有打开
CONFIG_EXT2_FS_XATTR=y

其实这样的话，其实就可以添加很多有趣的标记了。

## 实现
看了一下 ext2_setattr 和 ext2_getattr 的实现，只是提供 inode 的各种基本属性而已。

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
