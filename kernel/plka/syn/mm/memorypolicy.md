# mm/memorypolicy.c




```c
int vma_dup_policy(struct vm_area_struct *src, struct vm_area_struct *dst) // todo 万万没有想到，memorypolicy 和 vma 有关系
{
	struct mempolicy *pol = mpol_dup(vma_policy(src));

	if (IS_ERR(pol))
		return PTR_ERR(pol);
	dst->vm_policy = pol;
	return 0;
}
```

