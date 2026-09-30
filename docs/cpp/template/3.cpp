template <typename T> struct X {};

template <typename T> struct Y {
	using ReboundType = X<T>; // 这里为什么是正确的？
	using MemberType2 =
		typename X<T>::MemberType; // 这里的typename是做什么的？
};

int main (int argc, char *argv[]) {

	return 0;
}
