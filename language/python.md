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
