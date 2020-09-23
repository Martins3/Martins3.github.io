
## 对于一个文件的 Makefile 

```Makefile
ssl_server.out : ssl_server.c
	clang -o $@ $^
```

## 递归
https://stackoverflow.com/questions/17834582/run-make-in-each-subdirectory

$(MAKE) -C dir
