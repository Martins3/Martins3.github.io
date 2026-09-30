// module partition：模块内部的一个子模块。
// 名字 shapes:circle 表示它属于 shapes 模块的 circle 分区。
export module shapes:circle;

export constexpr double pi = 3.14159265;
export double circle_area(double r) { return pi * r * r; }
