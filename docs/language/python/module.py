#!/usr/bin/env python3
"""
PyTorch nn.Module 调用机制 Demo：module(...) 等价于 module.forward(...)

核心机制：Python 里 m(args) 是语法糖，等价于 m.__call__(args)。
nn.Module 在 nn/modules/module.py 中定义了 __call__（实际逻辑在 _call_impl），
它做三件事：
    1. 触发 _forward_pre_hooks
    2. 调用 self.forward(*args, **kwargs)
    3. 触发 _forward_hooks，返回结果
下面用纯 Python 模拟这条链路，不依赖 torch。
"""


class MyModule:
    """迷你版 nn.Module：只模拟 __call__ -> forward 分发链路"""

    def __init__(self):
        self._forward_pre_hooks = []
        self._forward_hooks = []

    def __call__(self, *args, **kwargs):
        # 对应 nn.Module._call_impl：先跑 forward pre-hooks
        for hook in self._forward_pre_hooks:
            hook(self, args, kwargs)
        # 核心分发：真正干活的是子类定义的 forward
        result = self.forward(*args, **kwargs)
        # 再跑 forward hooks（可拿到结果做后处理）
        for hook in self._forward_hooks:
            hook(self, args, result)
        return result

    def forward(self, *args, **kwargs):
        # 和 nn.Module.forward 一样：默认抛错，提示子类必须实现
        raise NotImplementedError("子类必须实现 forward()")


class SimpleLinear(MyModule):
    def __init__(self):
        super().__init__()
        print("  [SimpleLinear.__init__] Module name")

    def forward(self, x):
        return x * 2


def main():
    """演示：m(...) == m.forward(...)"""
    m = SimpleLinear()
    print("m(12) =", m(12))  # 走 __call__ -> forward
    print("m.forward(12) =", m.forward(12))  # 直接调 forward 结果相同


if __name__ == "__main__":
    main()
