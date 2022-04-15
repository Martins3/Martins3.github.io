1. No password
```sh
# copy local's .ssh/id_rsa.pub to remote servers' .ssh/authorized_keys
ssh -t $server "mkdir -p ~/.ssh"
cat ~/.ssh/id_rsa.pub | ssh $server 'cat >> .ssh/authorized_keys && echo "Key copied"'
# change the file privilege
chmod 644 authorized_keys
```
2. scp
3. -X


3. 保持当前进程运行退出
https://stackoverflow.com/questions/954302/how-to-make-a-programme-continue-to-run-after-log-out-from-ssh

ctrl+z
disown -h %1
bg 1
logout

fg
> 可以一并阅读一下
