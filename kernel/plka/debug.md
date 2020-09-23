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

# 
