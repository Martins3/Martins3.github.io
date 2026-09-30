# Python 条件表达式（三元运算符）
# 语法: A if condition else B

class Obj:
    pass

obj = Obj()
print(obj.dtype if hasattr(obj, "dtype") else "default")
# 不用纠结这个 ty 报错，这是没办法的，这个 dtype 是动态添加的
obj.dtype = "float16"
print(obj.dtype if hasattr(obj, "dtype") else "default")
