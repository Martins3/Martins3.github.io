# Kernel 调试

- [disassemble with code and line](https://stackoverflow.com/questions/9970636/view-both-assembly-and-c-code)
- [如何增大 dmesg buffer 的大小](https://unix.stackexchange.com/questions/412182/how-to-increase-dmesg-buffer-size-in-centos-7-2)

# 通过插入预防错误的方法实现
```c
dump_page(page, "VM_BUG_ON_PAGE(" __stringify(cond)")");\

void dump_page(struct page *page, const char *reason)
{
	__dump_page(page, reason);
	dump_page_owner(page);
}
EXPORT_SYMBOL(dump_page);
```

> 以后再去慢慢跟踪吧!

## kernel hacking

- `pr_info`
  - 注意 `%px` 来输出指针
- `dump_stack`


- [ ] 这个没有测试过啊
调试内核模块
```sh
cat /proc/modules
objdump -dS --adjust-vma=0xffffffff85037434 vmlinux
```

- 如何 hacking 内核的官方文档:
  - https://www.kernel.org/doc/html/latest/kernel-hacking/index.html
  - https://www.kernel.org/doc/html/latest/trace/index.html#
  - https://www.kernel.org/doc/html/latest/dev-tools/index.html

## memtest
- https://github.com/memtest86plus/memtest86plus

- journalctl -t kernel 展示所有的的内核日志。
