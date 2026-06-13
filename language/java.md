## 重学Java
http://hollischuang.gitee.io/tobetopjavaer/ 这个教程不错
https://github.com/Snailclimb/JavaGuide : 这个应该一般
https://github.com/doocs/advanced-java : 中文教程，数据库，分布式之类的角度讲解 java




# 概述
1. 字节码
2. 如果有public class, 那么最多只有一个， 文件名与之相同
3. 标识符命名规则
4. java数据类型
    1. 基本类型
        1. byte short int long
        2. char
        3. double  float
        4. boolean
    2. 引用类型
        1. class
        2. interface
        3. array
5. 字面值和后缀字母
6. 字符串 连接 + +=
```
String s = "2" + 1;
```
7. 嵌套if语句: if 和 最近的else配对
8. 操作符的优先级
    1. 二元操作符左边的操作数比右边的操作数优先运算
9. 常用的数学函数
    1. [随机数](https://stackoverflow.com/questions/363681/how-do-i-generate-random-integers-within-a-specific-range-in-java?answertab=votes#tab-top)
```
// way one
Min + (int)(Math.random() * ((Max - Min) + 1))

// way two
import java.util.concurrent.ThreadLocalRandom;
// nextInt is normally exclusive of the top value,
// so add 1 to make it inclusive
int randomNum = ThreadLocalRandom.current().nextInt(min, max + 1);
```
10. Character类
```
isDigit方法判断一个字符是否是数字
isLetter方法判断一个字符是否是字母
isLetterOrDigit方法判断一个字符是否是字母或数字
isLowerCase方法判断一个字符是否是小写
isUpperCase方法判断一个字符是否是大写
toLowerCase方法将一个字符转换成小写
toUpperCase方法将一个字符转换成大写
```
11. 分析　== equal()
    1. == 比较地址
    2. new 由于是开辟空间，　所以 ==比较总是不同
    3. Integer.value() 实现装箱的操作，依赖了常量池


11. String类

```
构造函数
长度(length)
获取字符(charAt)
连接(concat)
截取(substring)
比较(equals, equalsIgnoreCase, compareTo, startWith,endWith, regionMatch)
转换(toLowerCase, toUpperCase, trim, replace)
查找(indexOf, lastIndexOf)
字符串和数组间转换(getchars, toCharArray)
字符串和数字间转换(valueOf)
```
12. StringBuffer 和 StringBuilder
StringBuffer适合多线程

```
append方法在字符串的结尾追加数据
insert方法在指定位置上插入数据
reverse方法翻转字符串
replace方法替换字符
toString方法返回String对象
capacity方法返回缓冲区的容量
length方法返回缓冲区中字符的个数
setLength方法设置缓冲区的长度
charAt方法返回指定位置的字符
setCharAt方法设置指定位置的字符
```
13. boxing and unboxing
    1. public abstract int intValue()
    2. public static type parseType(String s)
       public static type parseType(String s, int radix)
    3. valueOf(String s)



# method
1. 函数返回值不是函数重载的依据
2. 没有默认参数， 最后一个可为变长参数
3. 没有局部static变量

# 包
包(package)是相关类和接口的集合,它可以对类进行组织、提供访问保护和名称空间管理。
组织功能：将相关的类和接口组织在一起，便于识别和管理
防止命名冲突：不同包中的相同命名，不会产生冲突，就是名字空间
访问控制：控制包间的类型的可访问性
语法
package packagename;
package语句必须位于源文件的第一条语句，源文件中定义的所有类和接口都属于指定的包。
如果没有使用package语句，那么类和接口被放在缺省包(default package)中，缺省包是一个没有名称的包



Java要求包名与文件系统的目录结构对应。对于名为  com.prenhall.mypackage的包，必须创建对应的目录结构。
包实际是上是包含字节码的目录
为了使Java知道包在文件系统的位置，必须修改环境变量classpath
set classpath=%classpath%;E:\JavaCourse\ch05\bin编译：
在com的上级目录下运行   javac com\prenhall\mypackege\Format.java -d e:\JavaCourse\ch05\bin
运行：在任何目录下（设定好classpath后
java com.prenhall.mypackege.TestFormatClass


使用组织的internet域名的反序形式命名包。
一个组织内部发生的命名冲突需要由组织内部的约定来处理，通常在组织名称后面包含项目名称。
包名必须与目录结构一一对应，目录名之间用.分隔。

# 数组
1. new 之后，内存单元初始化为０　或者　null
2. datatype[] array;
3. 使用花括号提供初始值
```
double[ ] myList = {1.9, 2.9, 3.4, 3.5}

double[ ] myList;
myList = new double[] {1.9, 2.9, 3.4, 3.5}
```

4. 复制数组的方法
    1. 使用循环来复制每个元素
    2. 使用System.arraycopy方法
    3. 调用数组的clone方法赋值
5. 变长参数
    1. 在方法的申明中，指定的类型后面跟省略号
    2. Java将可变长参数当数组看待，通过参数的length属性得到可变参数的个数
6. 高维数组中，数组的长度可以不同

# 对象和类
1. java没有析构函数，但垃圾自动回收之前会调用finalize()方法，可以覆盖定义该函数
2. [final]<https://en.wikipedia.org/wiki/Final_(Java)>
    1. final class
    2. final method : 如果非static, 禁止override, 否则，hidden
    3. final variabl : 对于blank final,  如果非static, 所有的constructor需要处理
       ，否则在static block中间初始化
    4. 构造函数不可以为final

3. static 方法不可以实现多态
4. 访问控制
    1. 本类　本包　子类　其他包
    2. private #   protected public
    3. 子类类体中可以访问从父类继承来的protected成员.
    但如果子类和父类不在同一个包里，子类里不能访问父类实例的protected成员, 也是
    就访问控制要求被强化了
5. 软件开发
    1. association
    2. aggregation　表示has-a关系
    3. composition 丛书这强烈依赖聚集者
    4. dependency
    5. inherence

6. 继承
    1. 子类可以不继承父类的构造函数

7. 初始化对象的过程
    1. 如果从未创建过对象，那么首先初始化父类的静态变量
    2. 首先super()，然后执行变量初始化(如果有赋值语句), 最后执行super后面的语句
8. 当类缺少无参构造函数的时候，依旧可以创建该对象的数组，因为数组默认创建的为
   null
9. 只有一个方法可以访问的时候才可以覆盖，如果子类定义了一个signature相同的函数，
   两者没有继承关系，但是不违背语法(子类假装没有看见)，但是如果的父类的可访问方
   法含有final修饰，那么相同的signature的函数不可以创建
10. 当类型转化为父类类型，那么就没有办法调用子类方法，除非由于override
11. override和overload没有混合使用，也就是只有signature完全相同的时候，才会发生
    override.
12. Object类
    1. equals toString getClass和 clone finalize方法
    2. getClass禁止覆盖
    3. 对于基本类型，使用==进行比较，如果是ref类型，则使用equals比较
    4. 覆盖equals函数，最好要同时覆盖hashCode,因为若两个对象相等，则他们的hashCode一定相等，若不覆盖就可能不相等
    5. 覆盖equals函数，首先应该用instance检查参数的类型是否和当前对象的类型一样
13. 子类类型转化为父类，需要首先使用instanceof 检查，然后使用强制类型装换
14. 抽象方法和抽象类必须添加abstract关键字
15. 只有实例方法可以声明为抽象方法, 也就是static和 abstract不可以同时修饰函数
16. interface
    1. 不可以定义构造函数
    2. 字段默认public static final
    3. method默认public abstract
    4. 使用extends继承接口
17. 动态绑定
    1. 函数可以动态绑定，但是变量不可以
    2. 静态函数不可以动态绑定
    3. 如果没有，那么就必定不可以实现动态绑定
    4. 当子类中间定义和super class中间相同函数名和参数签名相同，那么返回值必须相
       同
    5. 访问权限不可以缩小
    6. 子类不可以抛出更多的错误
    7. 可以定义满足覆盖条件的静态函数，从而实现隐藏super class的函数
    8. super class的非抽象函数
    9. 私有方法由于子类没有办法访问到，但是可以定义覆盖函数，但是没有不可以动态
       绑定
    10. final static private 修饰的函数全部前期绑定





# Exception and IO
1. Exception 程序可以处理，但是Error是系统错误，无法处理
2. throw之后语句都不会执行
3. 无论同层的catch是否捕获，处理异常，同层的finally都是会执行的
4. 自定义异常类
    1. 比如继承自Throwable或其子类
    2. 通常继承自Exception或其子类


# JavaFx基础
1. 属性绑定
2. 样式

# 泛型
1. Java泛型通过擦除实现
2. 反射

3. 泛型的放置位置在返回值之前，类名之后
4. 通配泛型 :　三种类型
5. 当编译完成之后，类型擦除，所以
    1. 不可以 new E()
    2. new E[]
    3. 通过类型装换无法保证安全
    4. 不可以使用　new A< E >[] `(new ArrayList<String>[])new ArrayList[10]`
    5. 在静态上下文不允许使用泛型的参数类型
    6. 异常类不可以泛型, 泛型类不可以扩展Java.lang.Throwable


# Appendix

### Use Vim
1. https://www.techrepublic.com/article/configure-vi-for-java-application-development/
2. use spacevim layers
3. https://stackoverflow.com/questions/1085146/programming-java-with-vim

### Change Default Java in Arch
https://wiki.archlinux.org/index.php/java#Change_default_Java_environment
