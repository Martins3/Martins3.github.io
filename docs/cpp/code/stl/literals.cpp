// https://en.cppreference.com/w/cpp/language/user_literal
// 第一个 demo
#include <iostream>

#include <string>

void        operator ""_km(long double); // OK, will be called for 1.0_km
// void        operator "" _km(long double); // same as above, deprecated
std::string operator ""_i18n(const char*, std::size_t); // OK

template<char...>
double operator ""_pi(); // OK
float  operator ""_e(const char*); // OK

// error: suffix must begin with underscore
// float operator ""Z(const char*);

// error: all names that begin with underscore followed by uppercase
// letter are reserved (NOTE: a space between "" and _).
double operator"" _Z(long double);

// OK. NOTE: no space between "" and _.
double operator""_Z(long double);

// OK: literal operators can be overloaded
double operator ""_Z(const char* args);

int main() {}
