class A {
    public:
	A(const A &) = default; // 拷贝构造
	A &operator=(const A &) = default; // 拷贝赋值
};

class X {
    private:
	int x;
};

class Y : public X {
    private:
	int y;
};

int main()
{
	// 下面的代码将会造成X的部分被释放， 但是Y的独有的部分无法被释放
	X *x = new Y;
	delete x;
}
