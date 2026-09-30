// 消费者：import 模块后，使用其 export 出来的名字。
import math;
#include <iostream>

int main() {
    std::cout << add(3, 4) << '\n';
    // sub 没有被 export，在模块外不可见，下面这行编译会报错。
    // std::cout << sub(10, 4) << '\n';
}
