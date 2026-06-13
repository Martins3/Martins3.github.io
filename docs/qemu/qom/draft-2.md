## qom 的 object_class_property 到底是谁的 property
<!-- b8b086d3-403e-45e9-b0c9-1889600c666f -->

codex 解释的很好了，object_class_property_add_bool 其实让 class 添加了一个属性的入口，
注意，这个函数的第一个参数就是 ObjectClass ，这个操作的意思是个 class 下的所有的 obj 都添加上一个
入口

```txt
• object_class_property_add_bool() 可以把它理解成：

  “在 QOM 类型系统里，给某个类注册一个 bool 类型的属性描述符；这个属性以后所有该类实例都能看到，但属性值通常还是存在实例里。”

  核心实现就在 qom/object.c:2397。它本身没做复杂逻辑，只做了两件事：

  1. 分配一个 BoolProperty，把你传进来的强类型回调保存起来：
     bool (*get)(Object *, Error **)
     void (*set)(Object *, bool, Error **)
  2. 调用通用的 object_class_property_add(..., "bool", ...) 注册到 klass->properties 里，真正暴露出去的是通用的 property_get_bool/
     property_set_bool 适配层。

  也就是说，它是“typed callback -> 通用 QOM property 框架”的一个胶水函数。

  怎么理解这个机制

  - “class property” 这里的意思不是“值存放在 class 里”，而是“属性接口定义挂在 class 上”。
  - 实例访问属性时，QOM 会先查 class property，再查 instance property，见 qom/object.c:1326。
  - 类属性支持继承，父类注册的属性，子类实例天然可见，查找是沿父类链递归的，见 qom/object.c:1377。
  - 因为 object_class_property_add() 注册时会先 assert(!object_class_property_find(...))，所以子类不能用同名属性把父类覆盖掉，见 qom/object.c:1299。

  访问时实际发生什么

  - 读属性时，QOM 走通用 Visitor 接口，property_get_bool() 先调用你传入的 typed getter 拿到 bool，再交给 visitor 输出。
  - 写属性时，property_set_bool() 先从 visitor 里解析出 bool，再调你的 typed setter。
  - 所以同一套属性可以同时服务于 QMP、命令行字符串解析、默认值注入等通路。这层适配就在 qom/object.c:2370。

  一个具体例子
  MachineClass 在 class_init 里注册了 "usb" 这个 bool 属性，见 hw/core/machine.c:1145。
  但真正的值读写发生在 MachineState 实例字段上，getter/setter 里先把 Object * 转成 MachineState *，再访问 ms->usb，见 hw/core/
  machine.c:411。

  所以它的心智模型最好是：

  “类上声明属性，实例上承载状态，QOM 用 visitor 做统一读写分发。”

  默认值怎么进来
  如果后面再对这个 ObjectProperty * 调 object_property_set_default_bool()，QOM 会把默认值挂到 prop->defval，并设置 prop->init，见
  qom/object.c:1581。
  对象初始化早期会遍历所有 class property，把默认值写进新实例，见 qom/object.c:543。

  一句话总结：

  object_class_property_add_bool() 不是“保存一个 bool 值”，而是“给一个类型声明一个 bool 属性入口，并把实例字段的读写接到 QOM 的统一属性系统里”。
```


<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
