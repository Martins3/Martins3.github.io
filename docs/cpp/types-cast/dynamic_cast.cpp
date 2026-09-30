// https://github.com/ethsonliu/stackoverflow-top-cpp/blob/master/question/010%20-%20static_cast,%20dynamic_cast,%20const_cast%20%E5%92%8C%20reinterpret_cast%20%E6%80%8E%E4%B9%88%E7%94%A8.md
//
// static_cast
// dynamic_cast
// reinterpret_cast
// const_cast
//
// (type)value和type(value) 其实是一个意思，只是写法风格的差异而已。
// 它涵盖了上面四种*_cast的所有功能， 同时它的使用需要完全由程序员自己把控。
//
// 如果需要
// https://stackoverflow.com/questions/15114093/getting-source-type-is-not-polymorphic-when-trying-to-use-dynamic-cast

class Base {};
class Derived : public Base {};

Base a, *ptr_a;
Derived b, *ptr_b;

int main(int argc, char *argv[])
{
	ptr_a = dynamic_cast<Base *>(&b); // 成功
	// ptr_b = dynamic_cast<Derived *>(&a); // 失败，因为基类无虚函数

	return 0;
}
