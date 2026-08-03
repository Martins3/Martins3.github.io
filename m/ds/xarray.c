#include "internal.h"

// 看看这个:
// 一个让 Linus Torvalds "不明觉赞" 的内核优化与修复历程 - 鹅厂架构师的文章 - 知乎
// https://zhuanlan.zhihu.com/p/11591870676
int test_xarray(long action)
{
	switch (action) {
		break;
	}
	return 0;
}
