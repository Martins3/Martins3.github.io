# Nasm

## 遇到的问题
1. 标号后面到底 要不要 : 
2. ld 做了什么，hello world 自我修养的上的准备在此处全部没有了，似乎栈空间是操作系统帮程序设置的
3. 当不使用 `_start` 作为入口:ld的警告含义
4. #include 


## 关注的问题
1. 函数定义
2. 宏
3. include
4. ABI接口
    1. 函数调用：传递参数
5. 代码段 数据段



## 暂时不关注的问题
1. 浮点，向量
2. as 和 nasm 的关系: https://developer.ibm.com/articles/l-gas-nasm/ https://stackoverflow.com/questions/13793609/nasm-vs-gas-practical-differences
    1. 内核使用的是as 格式
    2. 使用objdump 得到的结果也是 as 格式


## Man Nasm 
nasm 是一个汇编器

### directive
1. section/segment 将代码写入到指定的位置
2. absolute 
    1. When you have finished doing absolute assembly, you must issue another SECTION directive to return to normal assembly. 握草
    2. 

3. BITS 16, BITS 32 or BITS 64 switches the default processor mode for which nasm is generating code: it is equivalent to USE16 or USE32 in DOS assemblers.
    1. 和 real mode, protected mode 和 amd64 向对应吗 ? 应该不是，只是bit的长度的解释
    2.

4. absolute


### format-specific
1. org
2. group
3. library

### macro


### man没有覆盖的内容
1. 数据类型 : DB, DW, DD, DQ, DT, DO, DY and DZ - are used for declaring initialized data. For example:


# 资料
1. https://www.tortall.net/projects/yasm/manual/html/index.html
