// https://stackoverflow.com/questions/8246117/compiling-error-on-template-method-return-is-instance-from-inner-class
template <typename T> struct MyTemplate {
	class Inner {};
	Inner met();
	T getFoo();
	T f;
};

// 看似是因为 class Inner 的类型是取决于
template <typename T> typename MyTemplate<T>::Inner MyTemplate<T>::met()
{
	Inner i = Inner(); // 这个也是 ok 的
	return typename MyTemplate<T>::Inner(); // 这里的 typename 可以不用
}

int main()
{
	MyTemplate<int> foo;

	MyTemplate<int>::Inner bar = foo.met();
}
