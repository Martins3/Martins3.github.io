// 最小的 module interface unit：
// export module 声明这是一个模块接口，export 后的声明才会被导入者看到。
export module math;

export int add(int a, int b) { return a + b; }

// 没有 export 的声明只在本模块内部可见。
int sub(int a, int b) { return a - b; }
