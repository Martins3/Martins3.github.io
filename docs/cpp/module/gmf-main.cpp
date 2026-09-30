// C++23 的 import std; 一次导入整个标准库。
// 和 import person 一起用不会有文本包含冲突。
import person;
import std;

int main() {
    Person p("alice");
    std::cout << p.name_of() << '\n';
}
