#include <bits/stdc++.h>
#include <vector>
using namespace std;
int main(int argc, char *argv[])
{
	for (size_t i = 0; i < 20; i++) {
		// 这里每次都是清空的
		vector<int> m;
		m.push_back(i);
		std::cout << m.size() << std::endl;
	}
	return 0;
}
