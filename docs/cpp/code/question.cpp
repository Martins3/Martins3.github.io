// TODO 如果函数的参数是 rvalue ，其意义是什么?
void function_arg_rvalue(int &&m)
{
	m = 12;
}

void test3()
{
	int m = 10;
	function_arg_rvalue(12);
}

int main()
{
	test3();
	return 0;
}
