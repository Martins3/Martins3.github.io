# # python __setattr__ 方法
# <!-- 7ba2050c-ee91-44b6-988a-9dd4332c03e6 -->
#
# 这个小 demo 展示两个语法点
# 1. __setattr__ 方法可以每次属性赋值的时候自动记录下:
#   - 这就是为什么 pytorch 可以自动的利用 named_children() 来打印整个结构图
# 2. python 中的 iter ，如果是 list(iter) ，那么相当于自动的变为一个链表了
from collections.abc import Iterator


class Module:
    _children: dict[str, "Module"]

    def __init__(self) -> None:
        # 直接调用 object.__setattr__()，避免再次进入下面的 __setattr__()。
        object.__setattr__(self, "_children", {})

    def __setattr__(self, name: str, value: object) -> None:
        print(f"__setattr__ 收到：{name=}, {value=}")

        if isinstance(value, Module):
            # name 就是赋值语句左侧的属性名。
            self._children[name] = value

        # 真正把属性保存到对象中。
        object.__setattr__(self, name, value)

    def named_children(self) -> Iterator[tuple[str, "Module"]]:
        yield from self._children.items()


class Child(Module):
    def __init__(self, description: str) -> None:
        super().__init__()
        self.description = description

    def __repr__(self) -> str:
        return f"Child({self.description!r})"


class SimpleModule(Module):
    def __init__(self) -> None:
        super().__init__()

        # Python 会自动调用：
        # self.__setattr__("linear", Child("linear"))
        self.linear = Child("linear")
        self.activation = Child("activation")

        # 普通值不是 Module，不会被记录为 child。
        self.number = 10


def demo() -> None:
    module = SimpleModule()

    print("\n记录下来的名称：", module._children)

    children = module.named_children()
    print("named_children() 返回：", children)
    print("它是迭代器：", iter(children) is children)

    # list() 不要求参数本身是 list，只要求它是可迭代对象。
    print("第一次 list()：", list(children))

    # 迭代器已经被上面的 list() 消费完了，所以这里得到空列表。
    print("第二次 list()：", list(children))

    # 再调用一次 named_children()，会创建一个新的迭代器。
    print("新的迭代器：", list(module.named_children()))


if __name__ == "__main__":
    demo()
