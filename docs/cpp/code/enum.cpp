enum class movie_type { action, thrill };

int main(int argc, char *argv[])
{
	movie_type a = movie_type::action;
	// enum class 没办法 print
	/* std::cout << a << std::endl; */
	return 0;
}
