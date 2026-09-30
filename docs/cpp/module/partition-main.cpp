// 消费者只需要 import shapes，就能用所有被 re-export 的 partition。
import shapes;
#include <iostream>

int main() {
    std::cout << circle_area(2.0) << ' ' << square_area(3.0) << '\n';
}
