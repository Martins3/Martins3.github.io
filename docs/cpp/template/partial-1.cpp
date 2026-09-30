// 最基本的 partial-template-specialization 例子
//
// https://stackoverflow.com/questions/6138439/understanding-simple-c-partial-template-specialization
//
// 其实初始化了一部分，那么会优先使用
template <typename A, typename B>
class Thing //partial specialization of the class template
{
    public:
	int doSomething();
};

template <typename A>
class Thing<A, int> //partial specialization of the class template
{
    public:
	int doSomething2();
};

template <class A> int Thing<A, int>::doSomething2()
{
	return 0;
}

int main(int argc, char *argv[])
{
	// 很显然，由于 partial-specialization ，所以会优先匹配到    class Thing<A, int>
	Thing<int, int> a;
	a.doSomething2();

	Thing<int, float> b;
	b.doSomething();

	return 0;
}
