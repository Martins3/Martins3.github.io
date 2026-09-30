// TODO
// 抽象类中：在成员函数内可以调用纯虚函数，在构造函数/析构函数内部不能使用纯虚函数。
//   - 其他函数为什么可以使用 纯虚函数, 因为这个 纯虚函数 在子类中间被定义了
//   - 而 constructor 和 deconstructor 当在 抽象函数 调用的时候, 已经失去了 derived class 的环境了
//   - https://stackoverflow.com/questions/8630160/call-to-pure-virtual-function-from-base-class-constructor
