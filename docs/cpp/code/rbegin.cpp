#include <iostream>
#include <vector>

using namespace std;

int main()
{
	vector<int> x;
	x.push_back(1);
	auto i = x.begin();
	for (; i < x.begin() + 10000; ++i) {
		cout << *i << " ";
	}
	cout << x.capacity() << endl;
	return 0;
}
