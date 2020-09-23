# Engineering a Compiler : Intermediate Representatio



#### 5.4.2 Static Single-Assignment Form
当前的情况成为模拟执行，找到所有的代码的路径，然后分析，结果是，不仅仅是函数内部，函数调用关系图也需要进行模拟执行。

模拟执行的关键在于，循环执行的次数, 一个环应该遍历的次数是什么 ?

https://en.wikipedia.org/wiki/Static_single_assignment_form
