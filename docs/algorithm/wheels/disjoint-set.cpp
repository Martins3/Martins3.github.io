#include <bits/stdc++.h>
#include <vector>
using namespace std;
class DisjointSet {
	vector<int> arr;

    public:
	explicit DisjointSet(int size)
		: arr(size, 0)
	{
	}

	int find(int x)
	{
		if (arr[x])
			return arr[x] = find(arr[x]);
		return x;
	}

	void union_pair(int x, int y)
	{
		x = find(x);
		y = find(y);
		arr[y] = x;
	}
};

int main(int argc, char *argv[])
{
	return 0;
}
