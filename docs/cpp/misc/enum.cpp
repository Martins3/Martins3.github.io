#include <iostream>
// cpp 有作用域的 enum
enum class movie_type { action, thrill };

// c 风格作用域的 enum
enum MigrationState { START };

int main(int argc, char *argv[])
{
	movie_type a = movie_type::action;
	// enum class 没办法 print
	// std::cout << a << std::endl;
	// 但是可以通过 static_cast 来转换
	std::cout << static_cast<int>(a) << std::endl;
	return 0;
}
