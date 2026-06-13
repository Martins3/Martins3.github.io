# kernel/auditsc.md

1. 提供了大量的辅助函数，被 audit.h 封装，就是那些双下划线的函数
    1. 封装增加的内容，: unlikely(audit_context()
2. 提供了几个 filter 函数，用于实现辅助
