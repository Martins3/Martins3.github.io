#include <type_traits>

class A {
    public:
	int x;
	double y;
};

class B {
	int x;
	int y;
};

static_assert(std::is_aggregate_v<A>);
static_assert(!std::is_aggregate_v<B>);

int main()
{
}
