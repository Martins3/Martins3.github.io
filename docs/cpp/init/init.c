void test_int(void)
{
	int a; // 局部自动变量，没有初始化

	// 在 C 语言中，四种基本初始化方法中
	int b = 12;
	int c = { 13 };
	// int m(12);
	// int n{12};
}

struct Point {
	int x;
	int y;
};


void test_struct(void)
{
	struct Point a;

	// struct Point b = 12;
	struct Point c = { 13 };
	// struct Point m(12);
	// struct Point n{12};
}

void test_array(void)
{

	int i1[2];
	// int i2[2] = 1; // 这个不行
	int i3[2] = { 1 };
	// int i4[2]{ 1 };
	// int i5[2](1);

}

int main(int argc, char *argv[])
{
	return 0;
}
