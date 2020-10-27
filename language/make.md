
- [ ] use **tlpi-disk's** makefile to understand how to manage a project with many standalone projects with only one file.


## Links
https://chrismorgan.info/blog/make-and-git-diff-test-harness/ : git 和 make 结合，来实现测试套件

https://github.com/seisman/how-to-write-makefile : 还是按照这个认真学一遍吧

# make
## How to process a Makefile
```
target ... : prerequisites ...
    recipe
    ...
    ...
```
By default, make starts with the first target (not targets whose names start with ‘.’). This is called the default goal.

The recompilation must be done if the source file, or any of the header files named as prerequisites, is more recent than the object file, or if the object file does not exist.

## Variables make Makefile simpler

## Letting make Deduce the Recipes
it has an implicit rule for updating a ‘.o’ file from a correspondingly named ‘.c’ file using a ‘cc -c’ command.

## Rules for Cleaning the Directory

To handle unanticipated situations. We would do this:
```
.PHONY : clean
clean :
-rm edit $(objects)
```
This prevents make from getting confused by an actual file called clean and causes it to continue in spite of errors from *rm*

## What Makefile contains
Makefiles contain five kinds of things: **explicit rules**, **implicit rules**, **variable definitions**, **directives**, and **comments**

## Splitting Long Lines
Makefiles use a “line-based” syntax in which the newline character is special and marks the end of a statement

The way in which backslash/newline combinations are handled depends on whether the statement is a recipe line or a non-recipe line

outside of recipe lines, backslash/newlines are converted into a single space charactr.
Once that is done, all whitespace around the backslash/newline is condensed into a single
space: this includes all whitespace preceding the backslash, all whitespace at the beginning
of the line after the backslash/newline, and any consecutive backslash/newline combinations.

## Including Other Makefiles
The directive is a line in the makefile that looks like this:
```
include filenames...
``` 

One occasion for using include directives is when several programs, handled by individual makefiles in various directories, need to use a common set of variable definitions or pattern rules

## How *make* Reads a file
GNU make does its work in two distinct phases. During the first phase it reads all the makefiles, included makefiles, etc. and internalizes all the variables and their values, implicit and
explicit rules, and constructs a dependency graph of all the targets and their prerequisites.
During the second phase, make uses these internal structures to determine what targets will
need to be rebuilt and to invoke the rules necessary to do so.

We say that expansion is *immediate* if
it happens during the first phase: in this case make will expand any variables or functions
in that section of a construct as the makefile is parsed.
We say that expansion is deferred if
expansion is not performed immediately. Expansion of a deferred construct is not performed
until either the construct appears later in an immediate context, or until the second phase

## Seconday Expansion
GNU make works in two distinct **phases**: a read-in phase and a target-update phase.

GNU make also has the ability to enable a second expansion of the prerequisites (only) for some or all targets defined in the makefile.

In order for this second expansion to occur,
the special target .SECONDEXPANSION must be defined before the first prerequisite list that
makes use of this feature.


## Writing Rules
### 文件搜索
VPATH

vpath

### 多目标
`$@` 表示每个目标文件
`$()`　取执行make中间命令的结果


### 静态模式
```
  <targets ...> : <target-pattern> : <prereq-patterns ...>
      <commands>
    ...
```
targets定义了一系列的目标文件，可以有通配符。是目标的一个集合。

target-parrtern是指明了targets的模式，也就是的目标集模式。

prereq-parrterns是目标的依赖模式，它对target-parrtern形成的模式再进行一次依赖目标的定义。

```
objects = foo.o bar.o

all: $(objects)

$(objects): %.o: %.c
    $(CC) -c $(CFLAGS) $< -o $@
```

### 自动生成依赖关系
`$(CC) -MM file.c` 可以自动获取对应的文件
`$<` 每一个依赖文件


## 书写命令

### 命令执行
如果你要让上一条命令的结果应用在下一条命令时，你应该使用分号分隔这两条命令, 而且两条命令必须在同一行

### 嵌套执行make

### 定义命令包

## 使用变量
变量的命名字可以包含字符、数字，下划线（可以是数字开头），但不应该含有 `:`  `#` 、 `=` 或是空字符（空格、回车等）。变量是大小写敏感的，“foo”、“Foo”和“FOO”是三个不同的 变量名。传统的Makefile的变量名是全大写的命名方式，但我推荐使用大小写搭配的变量名.


如果你要使用真实的`$`字符，那么你需要用`$$`来表示。

在使用时，需要给在变量名前加上 $ 符号，但最好用小括号 () 或是大括号 {} 把变量给包括起来

### 变量中的变脸
`=` 引用的变量可以依赖于后定义的变量
`:=`不可以向前引用
`?=`
FOO ?= bar 其含义是，如果FOO没有被定义过，那么变量FOO的值就是“bar”，如果FOO先前被定义过，那么这条语将 什么也不做，其等价于：

### 变量高级用法
`$(var:=a=b)` 将以a结尾的替换为b结尾

### 追加变量值
`+=`

### override

```
override <variable>; = <value>;
override <variable>; := <value>;
```

### 变量

多行变量
环境变量
目标变量
模式变量

### 条件
```
<conditional-directive>
  <text-if-true>
else
  <text-if-false>
endif
```

测试关键字:
ifdef ifndef ifeq ifneq

### 使用函数
函数的语法
```
$(<function> <arguments>)
```
参数间以逗号 , 分隔，而函数名和参数之间以“空格”分隔。函数调用以 $ 开头，以圆括号 或花括号把函数名和参数括起.

1. 字符串处理函数

```[makefile]
  $(subst pattern, replacement, text)
  $(patsubst pattern, replacement, text)  
  和我们前面“变量章节”说过的相关知识有点相似。如 $(var:<pattern>=<replacement>;) 相当于 $(patsubst <pattern>,<replacement>,$(var)) ，而 $(var: <suffix>=<replacement>) 则相当于 $(patsubst %<suffix>,%<replacement>,$(var)) 。
  比如 echo $(var:.c=.o)　将.c结尾的文件替换为.o结尾的文件
  $(strip <string>)
  $(strip <pattern>, <string>)
  $(filter <pattern....>, <string>)
  filter-out 反向过滤函数
  $(sort <list>)

  $(word <n>,<text>)
  $(wordlist <ss>,<e>,<text>)

  $(words <text>) 统计单词个数
  $(firstword <text>)　取出第一个单词
```

2. 文件名处理函数
```
dir
notdir
basename
addsuffix
addprefix
join 将两个列表中间的单词一一连接起来
```

3. 流程控制
```
$(foreach <var>,<list>,<text>)
$(if <condition>,<then-part> [,<else-part>])
$(call <expression>,<parm1>,<parm2>,...,<parmn>)
```

4. 其他
```
$(error <text ...>)
$(shell cmd)
```

## make的运行
1. -f 指定目标

### 检查规则
1. -n
2. -t
3. -q

### 重要的操作
1. -B 认为所有文件全部过期，重新编译

## 隐含规则





## 看不懂的位置
1. *3.5 How Makefiles Are Remade* 想要表达什么。
2. *seconary expansion* 
4. objects = *.o的时候　* 不会展开，而　在依赖位置 main: \*.o 又是展开的，其中的原则是什么？
5. 文件搜索的具体使用代码 vpath 和 VPATH
6. 静态规则　静态体现在哪一个地方


# 学习跟我一起写Makefile
[文档地址](https://seisman.github.io/how-to-write-makefile/)



## 遇到的总结
[learn cmake](https://cmake.org/cmake-tutorial/)

## make
2. difference between `equals`
[link](https://stackoverflow.com/questions/448910/what-is-the-difference-between-the-gnu-makefile-variable-assignments-a)

3. 不要含有tab
```
Makefile:1: *** missing separator. Stop
```

4. Makefile中间执行shell
[link](https://www.gnu.org/software/make/manual/html_node/Shell-Function.html)

5. [include](https://www.gnu.org/software/make/manual/html_node/Include.html)

6. [条件语句](https://ftp.gnu.org/old-gnu/Manuals/make-3.79.1/html_chapter/make_7.html)

7. 字符串处理函数

8. [-I And -L](https://stackoverflow.com/questions/519342/what-is-the-difference-between-i-and-l-in-makefile)

9. echo
Normally make prints each line of the recipe before it is executed. We call this echoing because it gives the appearance that you are typing the lines yourself.
When a line starts with @, the echoing of that line is suppressed. The *@* is discarded before the line is passed to the shell. [...]

10. PHONY
A phony target is one that is not really the name of a file; rather it is just a name for a recipe to be executed when you make an explicit request.
There are two reasons to use a phony target:
  1. to avoid a conflict with a file of the same name
  2. improve performance.
  3. 实现同时编译多个文件,

11. Special Built-in Target Names
Certain names have special meanings if they appear as targets. for example, *.PHONY*

12. function
  1. there are some builtin function for `string` and `filename`
  2. [multiline](https://www.gnu.org/software/make/manual/html_node/Multi_002dLine.html) &&
  [Canned Recipes](https://www.gnu.org/software/make/manual/html_node/Canned-Recipes.html#Canned-Recipes)

13. Canned Recipes
  1. the ‘$’ characters, parentheses, variable names, and so on, all become part of the value of the variable you are defining


14. `eval`
    1. https://www.gnu.org/software/make/manual/html_node/Eval-Function.html
    2. It’s important to realize that the eval argument is expanded twice;
first by the eval function, then the results of that expansion are expanded again when they are parsed as makefile syntax.
    3. define 和C语言中间的宏类似，定义一堆语句，当使用call 的时候对于语句进行赋值，当eval 的时候，这些语句会被执行。
    4. Makefile 中间执行的语句分为两种，一种是各种变量的赋值之类，另一种是真正的动作。应该是各种赋值会最先执行完成，然后对于求解，就包括eval 之类东西
    5. 但是无法解释多级 eval 和 call 的实现!
        2. call 和 define 共同使用的含义，define 只是代表了一堆表达式，完全可以不怕麻烦的方法。当使用call 的时候是在查询其中的$1 之类的东西
        3. 部分表达式中间会出现$(tmp) 之类的情况，消耗$ 的方法不同，call 会消耗$1 之类的，eval 会消耗所有尚且含有的
        4. evla 和 $ 
    6. https://www.cmcrossroads.com/article/makefile-optimization-eval-and-macro-caching
    7. 如果没有多级define, 那么多级eval call 组合也没有必要
    8. 可以无限call，每次call 其实只是替换参数

15. `define`
    1. https://www.gnu.org/software/make/manual/html_node/Multi_002dLine.html#Multi_002dLine
    2. https://makefiletutorial.com/


16. `info` `error` `warn`
    1. https://stackoverflow.com/questions/11775733/how-can-i-print-message-in-makefile/11776179



## Snippet
```
SRCS = $(shell find src/ -name "*.c")
OBJS = $(SRCS:src/%.c=$(OBJ_DIR)/%.o)

# Compilation patterns
$(OBJ_DIR)/%.o: src/%.c
	@echo + CC $<
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(SO_CFLAGS) -c -o $@ $<

$(BINARY): $(OBJS)
	@echo + LD $@
	@$(LD) -O2 -o $@ $^ 
```

## 如何处理
当前目录下含有一堆.c 文件，比如a.c b.c c.c 现在如何生成 a.out b.out c.out 


## ucore Makefile 分析

#### 分析lab5
1. all: 到底是什么?
    1. all 什么都不是，只是偶尔会出现最前面
    2. 第一个出现的目标就是默认目标
    3. 当规则什么依赖都没有的时候，只要make 该规则就会触发

2. Makefile 中间的 call
    1. 并没有什么复杂的，自己定义函数以及内置部分函数



```
UINCLUDE	+= user/include/ \
			   user/libs/

USRCDIR		+= user

ULIBDIR		+= user/libs

UCFLAGS		+= $(addprefix -I,$(UINCLUDE))
USER_BINS	:=


# 将所有的文件全部收集起来!
listf_cc = $(call listf,$(1),$(CTYPE))
# list all files in some directories: (#directories, #types)
listf = $(filter $(if $(2),$(addprefix %.,$(2)),%),\
		  $(wildcard $(addsuffix $(SLASH)*,$(1))))



# add_files_cc 的作用

# cc compile template, generate rule for dep, obj: (file, cc[, flags, dir])
define cc_template
$$(call todep,$(1),$(4)): $(1) | $$$$(dir $$$$@)
	@$(2) -I$$(dir $(1)) $(3) -MM $$< -MT "$$(patsubst %.d,%.o,$$@) $$@"> $$@
$$(call toobj,$(1),$(4)): $(1) | $$$$(dir $$$$@)
	@echo + cc $$<
	$(V)$(2) -I$$(dir $(1)) $(3) -c $$< -o $$@
ALLOBJS += $$(call toobj,$(1),$(4))
endef

OBJPREFIX	:= __objs_
# change $(name) to $(OBJPREFIX)$(name): (#names)
packetname = $(if $(1),$(addprefix $(OBJPREFIX),$(1)),$(OBJPREFIX))

# add files to packet: (#files, cc[, flags, packet, dir])
define do_add_files_to_packet
__temp_packet__ := $(call packetname,$(4))
ifeq ($$(origin $$(__temp_packet__)),undefined)
$$(__temp_packet__) :=
endif
__temp_objs__ := $(call toobj,$(1),$(5))
$$(foreach f,$(1),$$(eval $$(call cc_template,$$(f),$(2),$(3),$(5))))
$$(__temp_packet__) += $$(__temp_objs__)
endef


add_files = $(eval $(call do_add_files_to_packet,$(1),$(2),$(3),$(4),$(5)))
add_files_cc = $(call add_files,$(1),$(CC),$(CFLAGS) $(3),$(2),$(4))


$(call add_files_cc,$(call listf_cc,$(ULIBDIR)),ulibs,$(UCFLAGS))
$(call add_files_cc,$(call listf_cc,$(USRCDIR)),uprog,$(UCFLAGS)) # uprog 为packname

UOBJS	:= $(call read_packet,ulibs libs)

define uprog_ld
__user_bin__ := $$(call ubinfile,$(1))
USER_BINS += $$(__user_bin__)
$$(__user_bin__): tools/user.ld
$$(__user_bin__): $$(UOBJS)
$$(__user_bin__): $(1) | $$$$(dir $$$$@)
	$(V)$(LD) $(LDFLAGS) -T tools/user.ld -o $$@ $$(UOBJS) $(1)
	@$(OBJDUMP) -S $$@ > $$(call cgtype,$$<,o,asm)
	@$(OBJDUMP) -t $$@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$$$/d' > $$(call cgtype,$$<,o,sym)
endef

$(foreach p,$(call read_packet,uprog),$(eval $(call uprog_ld,$(p))))
```
