如何测试，本来就提供了两个 demo :

https://github.com/freemed/tty0tty/blob/master/pts/tty0tty.c

执行之后
```txt
 ./tty0tty
(/dev/pts/2) <=> (/dev/pts/3)
```
然后分别在两个窗口执行，然后可以发现两边都是互相输入都是可以回显的:
```txt
socat -d -d STDIO /dev/pts/3
socat -d -d STDIO /dev/pts/2
```

- https://github.com/freemed/tty0tty/blob/master/examples/tnt1_echo_tnt0.py
