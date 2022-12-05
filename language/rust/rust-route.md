# Resource
- https://github.com/TheAlgorithms/Rust : 这就是入口了，使用这个去做 leetcode
  - https://github.com/EbTech/rust-algorithms : 看完之后，去刷题

https://rtpg.co/2020/12/11/dbg-in-python.html
http://dtrace.org/blogs/bmc/2020/10/11/rust-after-the-honeymoon/

[All](https://github.com/rust-unofficial/awesome-rust#resources)

https://www.ihcblog.com/rust-runtime-design-1/ : rust 的异步 io 模式。

[project base learning](https://github.com/tuvtran/project-based-learning#rust) 其中关于链表的很有意思。

https://anssi-fr.github.io/rust-guide/

https://blog.yoshuawuyts.com/state-machines/ : 博客

https://github.com/phil-opp/blog_os : 使用这个作为项目的基础也是不错的哦 !

https://blog.m-ou.se/writing-python-inside-rust-1/ : 相同的框架，为什么别人的blog 就是这样的

https://zhuanlan.zhihu.com/p/146472398?utm_source=wechat_session&utm_medium=social&utm_oi=602974433556828160&utm_content=first : 学习路线，其实体有意思的


https://github.com/ctjhoa/rust-learning : 各种教程汇总

https://github.com/rust-embedded/awesome-embedded-rust : 其实对于嵌入式并没有什么兴趣
https://lfn3.net/2020/08/03/a-gentle-intro-to-assembly-with-rust/ : Rust with assembly

https://rtpg.co/2020/12/11/dbg-in-python.html

https://fasterthanli.me/articles/a-half-hour-to-learn-rust : 半小时学习 rust

https://nick.groenen.me/posts/rust-error-handling/ : 一个人阅读 the rust book 之后开始写项目之后的感觉， error handling 的确很烦人。

https://github.com/ferrous-systems/elements-of-rust#combating-rightward-pressure : 一些常用写法 和 建议
https://github.com/fishinabarrel/linux-kernel-module-rust : 使用 Rust 来写 kernel module

[neovide](https://github.com/Kethku/neovide) neovim 客户端，只有 7000 行

#### What is the relation with str and String
https://mgattozzi.github.io/2016/05/26/how-do-i-str-string.html

#### Modular
https://doc.rust-lang.org/stable/rust-by-example/mod/split.html

#### Faq for Leetcode
1. [Append to vector as value of hashmap](https://stackoverflow.com/questions/33243784/append-to-vector-as-value-of-hashmap/33243862)
2. [Return a local hashmap](https://stackoverflow.com/questions/32682876/is-there-any-way-to-return-a-reference-to-a-variable-created-in-a-function)
3. [How to return a ref](https://bryce.fisher-fleig.org/blog/strategies-for-returning-references-in-rust/index.html)

4. https://github.com/aylei/leetcode-rust


#### This is fun
3. https://blog.subnetzero.io/post/building-language-vm-part-00/
4. https://vnduongthanhtung.gitbooks.io/migrate-from-c-to-rust/content/string-manipulations.html



## crate
https://github.com/serde-rs/json : 也许下次写小工具的有用，用于解析 json
https://github.com/sharkdp/hexyl : 显示文件的二进制，其实，我只是奇怪，为什么这么简单的程序会有 5k star
https://github.com/uutils/coreutils : 提供各种 Unix 小工具
https://github.com/fdehau/tui-rs : 终端下的图形化框架，好吧，以后写一个 anki 工具吧

## projects
- https://www.philippflenker.com/hecto-chapter-6/ : 使用 rust 写编辑器的教程

## fun
https://github.com/adamsky/globe
https://github.com/rust-unofficial/patterns
http://technosophos.com/2019/08/07/writing-a-kubernetes-controller-in-rust.html
https://www.osohq.com/post/rust-reflection-pt-1
https://github.com/ingraind/redbpf : Rust 提供给 bpf 的接口，但是我没有办法让 cargo 在 Sudo 下运行。

## blog
- https://www.brandons.me/blog/why-rust-strings-seem-hard : 介绍 Rust 的 String 使用

## TODO
3. dyn 关键字
4. use rust::blog::Post; 为什么需要添加rust:: 来指示本地包的作用 ?
5. macro 的定义非常奇怪，模式后面的代码如果想要定义statement 需要含有两层 {{ .}}
6. 我们可以在返回值中间包含mut keyword 吗 ?
7. https://doc.rust-lang.org/rust-by-example/std/hash.html `literal string`为什么总是在添加&使用

- [闭包](https://stevedonovan.github.io/rustifications/2018/08/18/rust-closures-are-hard.html)


第十三章讲到如果可以在参数列表前使用 move 关键字强制闭包获取其使用的环境值的所有权.

因此，Rust 类型系统和 *trait bound* 确保永远也不会意外的将不安全的 Rc<T> 在线程间发送

8. 思考一个问题：
  1. 当一个函数的作用是返回一个 struct, 进而这一个 struct 需要在在各个函数之间传递，如何保证该结构体生命周期的正确性。
  2. 或者说，在 c++ 中间，在一个函数中间 new 了一个对象，之后在任何地方 delete 掉，如何处理
  3. 甚至更加过分一点，一个 thread 创建了一个对象，但是这个对象需要被其他的 thread 使用，如何 ?

## Container
1. 如何才可以实现对于 Me.age 全部加上1
```rs
use std::collections::HashMap;

#[derive(Debug)]
struct Me{
    name : i32,
    age : i32
}



fn main() {
    let mut contacts = HashMap::new();
    contacts.insert(1, Me{age: 1, name: 2});

    for (contact, &number) in contacts.iter() {
      // number 后面添加 & 意味着 number 将会出现borrow，需要显示copy Trait
    }
}
```
2. 什么使用 &  &mut Copy 和 borrow ，是不是都可以实现 ?

## 泛型
1. https://www.reddit.com/r/rust/comments/7llmu1/why_do_you_need_to_declare_generic_twice_in_impl/
2. 如何让泛型变得难以理解:
    1. 多个泛型类型 T U
    2. 对于泛型类型添加限制
    3. 添加生命周期



## 函数式
1. https://stackoverflow.com/questions/34733811/what-is-the-difference-between-iter-and-into-iter
2. https://danielkeep.github.io/itercheat_baked.html
3. https://doc.rust-lang.org/rust-by-example/fn/closures/capture.html 解释为什么有的closure 需要  mut

4. Option 的使用:


## 控制流 类型
1.
```
fn foo(x: i32) -> i32 {
    if x == 5 {
        x
    } else if x == 4{
        x + 1
    }
}
// 这一个代码不可以编译，因为其中没有捕获所有的情况，不能形成为一个变量(!?)

for i in &queue {  // 如果不在此处添加 & ，那么之后即使是 queue.len() 也会导致 borrow 错误，添加之后，那么获取到i的类型也含有 &
    if in_board(i) {
        me += 1;
    }
}
```
2. https://techblog.tonsser.com/posts/what-is-rusts-turbofish

3. while let 语法糖 https://doc.rust-lang.org/beta/rust-by-example/flow_control/while_let.html
    1. 右侧是变量，左侧是模式

4. Also, there's actually 3 different kinds of iterator each collection should endeavour to implement:
   1. IntoIter - T
   2. IterMut - &mut T
   3. Iter - &T

## 并发控制
为此，我们不会冒忘记释放锁并阻塞互斥器为其它线程所用的风险，因为锁的释放是自动发生的。

## 生命周期 和 所有权
1. Rust 永远也不会自动创建数据的 “深拷贝”， 同时使第一个变量无效了
2. 参数和返回值都会 转移所有权
3. 变量的所有权总是遵循相同的模式：将值赋给另一个变量时移动它。当持有堆中数据值的变量离开作用域时，其值将通过 drop 被清理掉，除非数据被移动为另一个变量所有
4. 可以同时出现多个不可变引用 或者 一个可变ref，　可变引用有一个很大的限制：在特定作用域中的特定数据有且只有一个可变引用
5. 我们 也 不能在拥有不可变引用的同时拥有可变引用。不可变引用的用户可不希望在他们的眼皮底下值就被意外的改变了！然而，多个不可变引用是可以的，因为没有哪个只能读取数据的人有能力影响其他人读取到的数据。
6. dangling reference : 本体消失，但是ref 被使用
7. 生命周期确保结构体引用的数据有效性跟结构体本身保持一致。
8. 在给出接收者和方法名的前提下，Rust 可以明确地计算出方法是仅仅读取（&self），做出修改（&mut self）或者是获取所有权（self）。事实上，Rust 对方法接收者的隐式借用让所有权在实践中更友好。
9. 生命周期使用的位置:
    1. 函数: 返回引用指定生命周期
    2. 结构体: 结构体成员包含的引用
    3. trait :

10. 泛型使用的位置:
    1. 函数: 泛型函数
    2. 结构体
    3. trait

11. 泛型和生命周期的描述符都是可以同时使用的, 泛型可以添加限制，限制内容包括生命周期和trait, 但是不可以是结构体
12. 引用到底是什么东西:
  1. 是实现 borrow, 而不是取地址的作用
  2. deref
  3. 解引用强制多态

13. 非mutable 的不可以ref 为mutable

14. leetcode 688 中 对于从queue 中间去处元素的操作，
```
  let x = match queue.get(i) {
      Some(x)=> *x, // 这样的话，是进行了 clone 吗 ?
      None => panic!("Access queue out of order, impossible !")
  };
```

15. 如何在循环中间获取 mutable borrow
    - https://stackoverflow.com/questions/44104519/rust-lifetime-issue-in-loop 这一个例子看不懂啊!
    - https://stackoverflow.com/questions/37986640/cannot-obtain-a-mutable-reference-when-iterating-a-recursive-structure-cannot-b 这一个例子似乎是曾经含有错误，但是现在已经可以正常编译了, 是由于 non lexical lifetime

16. non-lexical-lifetimes
    - https://doc.rust-lang.org/edition-guide/rust-2018/ownership-and-lifetimes/non-lexical-lifetimes.html 官方文档
    - https://stackoverflow.com/questions/50111949/mutable-borrow-in-a-loop


17. leetcode 688 中能否使用更加优雅的指针的方法而不是采用 x y 重复的代码

18. https://rust-unofficial.github.io/too-many-lists/first-ownership.html 说明 mutable reference 对于读也是 exclusive 的，而且 borrow 的含义说明，当使用完成之后，限制就会解除

19. When you invoke `self.item.increment(amount)`, you are trying to pass `self.item` by value, thus moving ownership to the `Item::increment` function, but you are not allowed to do this with references to values that you don't own.
    1. 看上去，move 和 mut borrow 看似都是让某一个用户独占该变量，但是关键的不同在于，borrow 会归还，但是move 不会归还了。
    2. 如果将所有权传递给函数，那么函数调用结束之后，该变量就被释放了

20. By default, **a pattern match** will try to move its contents into the new branch, but we can't do this because we don't own self by-value here.

21. https://stackoverflow.com/questions/27197938/memreplace-in-rust mem::replace 解决了什么问题，为什么其本身需要通过unsafe机制实现
    1. 如果想要实现赋值操作，要么实现clone，要么move ownership
    2. 没有办法保证移动 ownership 之后立刻销毁了，那么之后使用变量的人就GG
    3. replace 操作保证转移了 ownership 之后总是含有发新的赋值，只是变量发生了变化

22. as_ref 的作用是什么: as_ref 和 map 的文档可以看一下，很恐怖的
```
    let text: Option<String> = Some("Hello, world!".to_string());
    // First, cast `Option<String>` to `Option<&String>` with `as_ref`,
    // then consume *that* with `map`, leaving `text` on the stack.
    let text_length: Option<usize> = text.as_ref().map(|s| s.len());
    println!("still can print text: {:?}", text);
```
> 其中 as_ref 和 take() 的功能类似

23. It turns out that **writing the argument of the closure that way doesn't specify that value is a mutable reference**.
Instead, it creates a pattern that will be matched against the argument to the closure; |&mut value| means "the argument is a mutable reference,
but just copy the value it points to into value, please." If we just use |value|, the type of value will be &mut i32 and we can actually mutate the head.
        list.peek_mut().map(|value| *value = 42); // 正确
        list.peek_mut().map(|mut value| *value = 42); // @todo 诡异的警告
        list.peek_mut().map(|&mut value| *value = 42); // 报错，和上述的理论相符

24. Quite simply, a lifetime is the name of a region (~block/scope) of code somewhere in a program. That's it. When a reference is tagged with a lifetime, we're saying that it has to be valid for that entire region. Different things place requirements on how long a reference must and can be valid for. The entire lifetime system is in turn just a constraint-solving system that tries to minimize the region of every reference.
    1. 生命周期是为了处理ref 而产生的

25. Within a function body you generally can't talk about lifetimes, and wouldn't want to anyway. The compiler has full information and can infer all the contraints to find the minimum lifetimes. However at the type and API-level, the compiler doesn't have all the information. It requires you to tell it about the relationship between different lifetimes so it can figure out what you're doing.
    1. 函数中间的确没有，更多是函数生命和结构体申明
    2. 函数需要生命周期的典型 : 传递两个不同生命周期的ref ，返回其中的一个ref
    3. 结构体需要生命周期的典型 : 当结构体中间持有ref, 需要指明结构体和其中ref 生命周期的关系. 思考一下:C++中间是如何处理class中间的ref 的

26. So if you keep the output around for a long time, this will expand the region that the input must be valid for.
Once you stop using the output, the compiler will know it's ok for the input to become invalid too.
    1. 生命周期限定符辅助输入的生命周期的, 因为output lifetime 依赖于 input lifetime ，只要 output 没有被 disable ，那么 input 禁止被disable
    2. 有一个错觉，那就是引用会到scope/block 结束的位置的时候消失，但是实际上并不是的，消失的位置是在无人使用的时候

27. https://users.rust-lang.org/t/what-is-the-point-of-lifetime-parameters-in-struct-impl-blocks/14631/2
    1. struct 的生命周期
    2. subtype 的含义是什么




## 字符串
1. “字符串 slice” 的类型声明写作 &str
2. String 和 str http://www.ameyalokare.com/rust/2017/10/12/rust-str-vs-String.html
3. Prefer &str as a function parameter or if you want a read-only view of a string; String when you want to own and mutate a string.
4. String 类型的slice 就是 str, 因为str本身描述不可变的，所以slice 作为没有所有权的类型是很科学的
5. 可以使用 to_string 方法，它能用于任何实现了 Display trait 的类型，字符串字面值也实现了它. 也可以使用 String::from 函数来从字符串字面值创建 String
6. 也可以使用 String::from 函数来从字符串字面值创建 String
7. 使用+运算符或!format宏拼接字符串

## malloc
除了数据被储存在堆上而不是栈上之外，box 没有性能损失。不过也没有很多额外的功能。它们多用于如下场景
1. 当有一个在编译时未知大小的类型，而又想要在需要确切大小的上下文中使用这个类型值的时候 : 递归类型：因为Box 将真正的数据存储在heap 上，自己只是一个指针，所以可以结束无限循环
2. 当有大量数据并希望在确保数据不被拷贝的情况下转移所有权的时候 :
3. 当希望拥有一个值并只关心它的类型是否实现了特定 trait 而不是其具体类型的时候 : 17 trait 对象

4. Box的作用，malloc 的效果的效果，可以使用ref 的形式的访问，只是将真正的数据放到heap之中，borrow ref的规则依旧满足.
5. Rc的作用，可以实现多个人访问，使用clone 实现多个访问, 和Box相同的，访问直接使用`*` 就可以了，但是似乎仅仅限于
6. Rc provides shared ownership so by default its contents can't be mutated, while Box provides exclusive ownership and thus mutation is allowed:

```rs
use std::rc::Rc;

#[derive(Debug)]
struct A{
    name: i32,
    addr: i32,
}

fn main() {
    let mut b = A{name : 2, addr : 3};
    let mut x = Rc::new(&mut b);
    let mut c = Rc::get_mut(&mut x).unwrap();
    c.name = 4;
    // b.name = 5; 取消注释之后失败，因为x 的初始化的时候已经获取了其mut 的，所以无法在修改，如果是获取其ownership，将print 报错
    println!("{:#?}", c);
    println!("{:#?}", b);
}
// x的初始化参数必须是 &mut ,　没有mut , c.name 的赋值失败，没有&, 最后一行无法输出



use std::rc::Rc;

fn main() {
    let b = 12;
    let mut x = Rc::new( b);
    let c = Rc::get_mut(&mut x).unwrap();
    *c = 4;
    println!("{}", c);
    println!("{}", b);
}
```
7. is a & reference so the data it refers to cannot be written : WDNMD, 出现该错误的原因并不是 &不可以访问，需要进一步deref (感觉这一个rust 的deref 本身有点问题)，真正的原因是需要成为mutable 的。
成为mutable 的要求是一个chain.

```rs
fn main() {
   let mut i = Box::new(12); // 必须含有 mut
   *i = 12;
}
```

```rs
fn main() {
   let mut b = 12;
   let mut i = Box::new(b);
   *i = 13;
   println!("{}", b);
   b = 14;
   println!("{}", b);
}
// 1. 没有报错
// 2. 输出 12 14, 不是13 14 说明 i 和 b　已经分离
真正的原因是: Box::new的操作会调用Copy(应该是，没有证据)，将数据copy 到Heap 上面，而原来的数据还是在stack 上，
将b的申明中间的mut 删除掉之后，只是b 不能继续赋值了。
```
8. Rc RefCell 的clone 参数必定是ref(因为需要实现多个ownership, 而且clone 的参数要求)，但是包括Box在内的new 的参数 不能是ref
9. Box::new Rc::new 的对象，前者可以直接使用 `*`, 后者需要 get_mut 才可以。其中的原因是，Box 的ownership 是独占的，但是 Rc 必须保证只有一个 strong_accout的时候才可以进行修改. 如果想要实现多个人修改，那么就需要采用ref cell

```rs
#[derive(Debug)]
struct A{
    name: i32,
    addr: i32,
}

fn main() {
    let mut b = A{name : 2, addr : 3};
    let mut x = Box::new(&mut b);
    x.name = 4; // 再一次说明 Box 可以直接访问发，而且更加过分的是，没有使用(*x).name
    println!("{:#?}", x);
    println!("{:#?}", b);
}
```

```rs
#[derive(Debug)]
struct A{
    name: i32,
    addr: i32,
}

fn main() {
    let b = A{name : 0, addr : 0};
    let mut x = Box::new(b);
    x.name = 4;
    println!("{:#?}", x);
    // println!("{:#?}", b);
}
// 写法有点奇怪了，一般来说需要将b的声明添加mut 就可以 实现对于b的数值进行修改了，其实并没有造成误解，因为
// b 的ownership 已经没有，而 x 必须含有mut
```

```rs
fn main() {
    let mut b = A{name : 0, addr : 0};
    let mut x = Box::new(&b);
    // x.name = 4; // 此行出现错误，报错会说，这是ref, 其实是mutable,传入Box::new不是mutable 的，所以自然不可以改变。
    println!("{:#?}", x); // 但是读取操作并不影响，依旧是可以使用的
    // println!("{:#?}", b);
}
```

10. borrow 之后就不可以再转移ownership了，没有ownership 变量相当于消失了，没有mut 的之后，还可以打印一下。

11. Rc<RefCell> 还是 RefCell<Rc> ： 可变多个


## 面向对象
1. 当使用 trait 对象时，Rust 必须使用动态分发
2. 如果一个 trait 中所有的方法有如下属性时，则该 *trait 是对象安全*的：
    1. 返回值类型不为 Self
    2. 方法没有任何泛型类型参数
3. 讲道理，完全没有理解面向对象的内容

## 模式匹配
1. 注意, 只使用 `_` 和使用以下划线开头的名称有些微妙的不同：比如 `_x` 仍会将值绑定到变量，而 `_` 则完全不会绑定。


## 宏
1. 在附录 C 中会探讨 derive 属性，其生成各种 trait 的实现。
2. 宏和函数的最后一个重要的区别是：在调用宏 之前 必须定义并将其引入作用域，而函数则可以在任何地方定义和调用
3. However, unlike macros in C and other languages, **Rust macros are expanded into abstract syntax trees**, rather than string preprocessing,
so you don't get unexpected precedence bugs.
4. there are three basic ideas for macro:
    1. Pattern and Designator
    2. overloading
    3. repetition
5. 无论何时导入定义了宏的包，#[macro_export] 注解说明宏应该是可用的。 如果没有该注解，这个宏不能被引入作用域。
6. https://danielkeep.github.io/tlborm/book/index.html 似乎宏并不是被大多数使用的

## unsafe
1. 常量与静态变量的另一个区别在于静态变量可以是可变的。访问和修改可变静态变量都是 不安全 的
2. 使用 unsafe 来进行这**四个操作（超级力量）**之一是没有问题的，甚至是不需要深思熟虑的
    * 解引用裸指针
    * 调用不安全的函数或方法
    * 访问或修改可变静态变量
    * 实现不安全 trait
3.


## closure
1. 闭包可以通过三种方式捕获其环境，他们直接对应函数的三种获取参数的方式：获取所有权，可变借用和不可变借用


## 从最简单的问题分析起
为什么在Rust中间创建一个链表如此麻烦:
https://news.ycombinator.com/item?id=16442743

### Question
```
  let arr: [i32; 5] = [1, 2, 3, 4, 5];
  let mut guess = String::new();
  io::stdin().read_line(&mut guess)
      .expect("Failed to read line");
  let guess: i32 = guess.trim().parse().expect("fuck learn this is not easy");
  println!("fuck this line {} {}", arr[guess], guess);
```

### Return
what is the purpose of keyword `return`
how to return in match
```
    fn get_number(c: char) -> i32 {
        match c {
            'I' => return 2,
            _ => return 3,
        }
        2
    }
```

```
use std::collections::BTreeMap;

// lost control of reference
pub fn majority_element(nums: Vec<i32>) -> i32 {
    // why should use divide and conquer
    let threshold = nums.len() as i32 / 2;
    let mut scores = BTreeMap::new();
    for i in &nums {
        match scores.get(i).cloned() {
            Some(x) => scores.insert(*i, x + 1),
            None => scores.insert(*i, 1),
        };
    }

    for (key, value) in &scores {
        // println!("{}: {}", key, value);
        if *value > threshold {
            return *key;
            // this line can not be changed to ---> *key
        }
    }
    1 as i32
}

fn main() {
    let v = vec![2, 3, 2];
    println!("{}", majority_element(v));
}
```

#### clone() how to copy and clone a data
https://stackoverflow.com/questions/28800121/use-of-moved-values
When a function requires a parameter by value, the compiler will check if the value can be copied by checking if it implements the trait Copy.
1. If it does, the value is copied (with a memcpy) and given to the function, and you can still continue to use your original value.
2. If it doesn't, then the value is moved to the given function, and the caller cannot use it afterwards


## Rust for leetcode
1. 创建指定大小的初始化数组 vec![0; 26]
2. iter String
```
        let bytes = word.as_bytes();
        let mut v = &mut self.node;

        for (_, &item) in bytes.iter().enumerate() {
```

## Vector赋值


## oop 测试代码
```rs
pub mod blog {
    #![allow(dead_code)]
    pub struct Post {
        content: String,
    }
    pub struct DraftPost {
        content: String,
    }
    impl DraftPost {

        pub fn add_text(&mut self, text: &str) {
            self.content.push_str(text);
        }
        pub fn request_review(self) -> PendingReviewPost {
            PendingReviewPost {
                content: self.content,
            }
        }
    }

    pub struct PendingReviewPost {
        content: String,
    }

    impl PendingReviewPost {
        pub fn approve(self) -> Post {
            Post {
                content: self.content,
            }
        }
    }


    impl Post {
        pub fn new() -> DraftPost {
            DraftPost {
                content: String::new(),
            }
        }

        pub fn content(&self) -> &str {
            &self.content
        }
    }

    trait State {
        fn request_review(self: Box<Self>) -> Box<dyn State>;
        fn approve(self: Box<Self>) -> Box<dyn State>;
        fn content<'a>(&self, post: &'a Post) -> &'a str {
            ""
        }
    }
    struct Draft {}

    impl State for Draft {
        fn request_review(self: Box<Self>) -> Box<dyn State> {
            Box::new(PendingReview {})
        }
        fn approve(self: Box<Self>) -> Box<dyn State> {
            self
        }
    }


    struct PendingReview {}

    impl State for PendingReview {
        fn request_review(self: Box<Self>) -> Box<dyn State> {
            self
        }
        fn approve(self: Box<Self>) -> Box<dyn State> {
            Box::new(Published {})
        }
    }


    struct Published {}

    impl State for Published {
        fn request_review(self: Box<Self>) -> Box<dyn State> {
            self
        }
        fn approve(self: Box<Self>) -> Box<dyn State> {
            self
        }
        fn content<'a>(&self, post: &'a Post) -> &'a str {
            &post.content
        }
    }

    fn oop() {
        let mut post = Post::new();
        post.add_text("I ate a salad for lunch today");

        let post = post.request_review();

        let post = post.approve();

        assert_eq!("I ate a salad for lunch today", post.content());
    }
}
```
