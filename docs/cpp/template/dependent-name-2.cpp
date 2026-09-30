// https://stackoverflow.com/questions/1527849/how-do-you-understand-dependent-names-in-c
//
// Names that depend on a template don't get bound until the point of instantiation whereas
// names that don't get bound at the point of definition.
#include <iostream>

template <class T> class Dependent {
    protected:
	T data{};

    public:
	T getData();

	void show()
	{
		std::cout << "Dependent" << std::endl;
	}
};

template <> class Dependent<float> {
    public:
	void show()
	{
		std::cout << "Dependent" << std::endl;
	}
};

template <typename T> T Dependent<T>::getData()
{
	std::cout << "注意，class 自己的函数没有问题" << std::endl;
	std::cout << data << std::endl;
	std::cout << this->data << std::endl;
	return Dependent<T>::data;
}

int data = 1234;

template <class T> class OtherDependent : public Dependent<T> {
	// using Dependent<T>::data; //Ok now.
    public:
	void printT() const
	{
		// 1. 继承的函数，无法直接感知到 data
		// 由于没有明确 using Dependent<T>::data ，所以这个 data 直接使用全局变量
		// 了
		std::cout << "T: " << data << std::endl;

		// 2. 这两种写法都是可以
		std::cout << "T: " << this->data << std::endl;
		std::cout << "T: " << Dependent<T>::data << std::endl;

		// 这也是可以的
		// std::cout << "T: " << Dependent<bool>::data << std::endl;
	}
};

// 注意，这就是当前的 class 的全部定义，和 OtherDependent 不同，没有任何的继承
template <> class OtherDependent<int> {
    public:
	void printT() const
	{
		std::cout << "OtherDependent" << std::endl;
	}
};

int main()
{
	OtherDependent<bool> how;
	how.printT();
	how.show();
	how.getData();

	OtherDependent<int> o;
	o.printT();

	// 在 template <class T> class OtherDependent : public Dependent<T> {
	// 中使用了
	// using Dependent<T>::data;
	//
	// 由于 float 可以匹配到的 template <> class Dependent<float> 其中根本就没有定义
	//
	// 所以，当一个 class 继承了一个 template class 的时候，不能随意假设
	OtherDependent<float> m;
	m.show();
	// FIXME 我感觉这里非常奇怪，居然只要不去调用，就不会报错，如何理解?
	// 既然可以等到调用的时候再去报错，
	// template <class T> class OtherDependent : public Dependent<T> { 中直接使用 data 有什么问题
	// 也可以等真的有调用的时候来展示啊
	//
	// m.printT();

	return 0;
}
