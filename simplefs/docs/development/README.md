# Development

- [构建与测试环境](testing.md)
- [xfstests runner 结构](xfstests.md)
- [调试方法与真实案例](debugging.md)

修改代码后的基本顺序是：主机构建、guest 定向回归、检查日志和 dmesg、邻接回归，
最后再做完整 generic 验收。
