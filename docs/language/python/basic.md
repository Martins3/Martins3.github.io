https://stackoverflow.com/questions/89228/calling-an-external-command-in-python
https://docs.python-guide.org/
https://github.com/lijin-THU/notes-python

https://github.com/geekcomputers/Python : 一些实用的脚本
https://github.com/coodict/python3-in-one-pic : 思维导图
https://gto76.github.io/python-cheatsheet/ : python check sheet

## 静态函数和静态成员
-  staticmethod 和 classmethod
  - https://stackoverflow.com/questions/136097/difference-between-staticmethod-and-classmethod
- python 的 static member 声明，不再函数中就可以了
  - https://stackoverflow.com/questions/68645/class-static-variables-and-methods

```py
class MyClass:
  i = 3

m = MyClass()
m.i = 4
print(m.i)
print(MyClass.i)
```
这个输出和 cpp 的不一样，这里是 class 和 instance 都又一份 static member


## 比较 a array of dict


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
