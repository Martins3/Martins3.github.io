// primary module interface unit：模块的入口，把各个 partition 聚合起来。
// export import 表示"导入并再导出"，让使用 shapes 的人也能看到这些 partition。
export module shapes;

export import :circle;
export import :square;
