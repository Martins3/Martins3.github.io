#include <iostream>
#include <string>

using namespace std;
string v;
double foo() {
  double aa;
  cin >> aa;
  return aa;
}

double g = foo();

int main() {
  cout << g << endl;
}
