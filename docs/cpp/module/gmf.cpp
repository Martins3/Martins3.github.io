// global module fragment：module; 到 export module 之间的部分。
// 只有这里可以 #include 头文件，让头文件里的声明（比如 std::string）
// 在接口的 export 声明中可用。
module;
#include <string>
export module person;

export class Person {
    std::string name;
public:
    explicit Person(std::string n) : name(std::move(n)) {}
    std::string name_of() const { return name; }
};
