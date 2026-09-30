#include <utility>
#include <cstring>
#include <string>
#include <cassert>

using namespace std;

int main()
{
	tuple<int, string> a(10, "fuck");
	size_t len = tuple_size<tuple<int, string> >::value;
	printf("%ld\n", len);
	return 0;
}
