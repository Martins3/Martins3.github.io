https://stackoverflow.com/questions/89228/calling-an-external-command-in-python
https://docs.python-guide.org/
https://github.com/lijin-THU/notes-python

https://github.com/geekcomputers/Python : 一些实用的脚本
https://github.com/coodict/python3-in-one-pic : 思维导图
https://gto76.github.io/python-cheatsheet/ : python check sheet


- [ ] 总结一些从 fio 测试的中间获取的内容吧

```python
import pprint
import os
import subprocess
import shlex

def rename():
    directory = os.path.realpath("./")
    for subdir, dirs, files in os.walk(directory):
        for filename in files:
            if filename.find('.py') > 0:
                if 'pyc' in filename:
                    continue
                subdire_path = os.path.relpath(subdir, directory)
                filepath = os.path.join(directory, subdire_path, filename)

                cmd = "f2format -na {}".format(filepath)
                # cmd = f'sed -i -E "s/python3/python3/" {filepath}'
                subprocess.check_call(shlex.split(cmd))

if __name__ == "__main__":
    pass
```

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

## decorator
- https://www.jianshu.com/p/ee82b941772a

简单来说，就是首先调用函数之前，首先调用 decorator，最后根据 return 调用 decorator 中定义的函数
而且是可以递归的。

## getter / setter
- https://stackoverflow.com/questions/2627002/whats-the-pythonic-way-to-use-getters-and-setters
```python
class C(object):
    def __init__(self):
        self._x = None

    @property
    def x(self):
        """I'm the 'x' property."""
        print("getter of x called")
        return self._x

    @x.setter
    def x(self, value):
        print("setter of x called")
        self._x = value

    @x.deleter
    def x(self):
        print("deleter of x called")
        del self._x


c = C()
c.x = 'foo'  # setter called
foo = c.x    # getter called
del c.x      # deleter called
```
