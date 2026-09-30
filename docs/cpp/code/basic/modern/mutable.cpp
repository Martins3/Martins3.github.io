// https://stackoverflow.com/questions/105014/does-the-mutable-keyword-have-any-purpose-other-than-allowing-a-data-member-to

void test1()
{
	// Since c++11 mutable can be used on a lambda to denote that things captured by value are modifiable (they aren't by default):
	int x = 0;
	auto f1 = [=]() mutable { x = 42; }; // OK
	// auto f2 = [=]() { x = 42; }; 错误
}

// const 的 object 是完全修改其中任何变量的，但是 mutable 修饰的变量可以。

int main(int argc, char *argv[])
{
	return 0;
}
