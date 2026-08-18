#!/usr/bin/env python
import taichi as ti
x = ti.var(ti.f32)
ti.root.dense(ti.ijk, (3, 5, 4)).place(x)

@ti.kernel
def func():
    print("fuck me !")
    print(x.snode().get_shape(0))
    print(x.dim())


func()
