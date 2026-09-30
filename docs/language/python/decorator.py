# # python decorator
# <!-- cec572c5-1058-499d-a752-58a35d7e460d -->
# 不修改原函数代码, 给函数附加额外行为 (横切关注点, 如计时/日志/鉴权/缓存)
# 本质: 装饰器是"接收函数、返回新函数"的高阶函数
# 所以，在 decorator 中，会定义一个 wrapper ，然后返回

import time


def work():
    total = 0
    for i in range(1_000_000):
        total += i
    return total


def timer(func):
    def wrapper(*args, **kwargs):
        start = time.perf_counter()
        result = func(*args, **kwargs)  # 原函数照常执行
        elapsed = time.perf_counter() - start
        print(f"{func.__name__}() took {elapsed * 1000:.2f} ms")
        return result  # 返回值原样透传

    return wrapper


print("before:", work())

# 核心用法: 用 timer 包一层, 再赋回给原名
# work 的源码一行没改, 但行为被增强了
work = timer(work)
print("after :", work())

# @timer 只是上面 work = timer(work) 的语法糖
@timer
def other():
    return "hello"


print(other())
