# ps 常见用法

## ps -elf

## ps f

## pstree

## ps x

- [ ] 实际上，将状态显示的更加细节
```txt
 PID TTY      STAT   TIME COMMAND
 1688 ?        Ss     0:00 /lib/systemd/systemd --user
 1689 ?        S      0:00 (sd-pam)
 1724 ?        S      0:01 sshd: vagrant@pts/0
 1725 pts/0    Ss     0:00 -bash
 2628 pts/0    R+     0:00 ps x
```
## ps -auxf
