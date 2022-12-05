#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

using namespace std;

// basic
void A() {
  string pattern("[^c]ei");

  // we can get the whole worlds
  pattern = "[[:alpha:]]*" + pattern + "[[:alpha:]]*";

  regex r(pattern);

  smatch reaults;

  string test_str = "aa   ssfeisssss";

  if (regex_search(test_str, reaults, r)) {
    cout << reaults.str() << endl;
  }
}

// exception
void B() {
  try {
    regex r("[[:alnum]");
  } catch (regex_error e) {
    cout << e.what() << "\ncode: " << e.code() << endl;
  }
}

// iterator
void C() {
  string pattern("[^c]ei");
  pattern = "[[:alpha:]]*" + pattern + "[[:alpha:]]*";

  regex r(pattern);

  string test_str = "aa mafei cei ssfeiss sss";

  for (sregex_iterator it(test_str.begin(), test_str.end(), r), end_it;
       it != end_it; it++) {
    // *it --- a smatch object
    cout << it->str() << endl;
  }
}

// subexpression
void D() {
  // smatch access like array
}

// regex replace
void E() {}

int main() {
  D();
  return 0;
}
