# TODO
https://news.ycombinator.com/item?id=22865357 : C 标准在hn 上的提问。
https://news.ycombinator.com/item?id=22888318 : 嵌入式的内容
http://nickdesaulniers.github.io/blog/2019/05/12/f-vs-f-void-in-c-vs-c-plus-plus/

https://github.com/rby90/project-based-tutorials-in-c
https://github.com/angrave/SystemProgramming : C 语言教程，快速浏览，查漏补缺

# 好玩的东西
https://github.com/taylorconor/quinesnake : 我记得还有一个使用 printf 实现 xxoo 的游戏



# You don't know C

## C Traps and pitfalls
1. int shall not has leading zero
```c
int a = 012;
```

2. token analyzer is greedy, which means nested commnet is invalid
```c
/**/*/ equals */ not nothing !
```

3. use `typedef` to simplify function pointer type
```c
typedef void (*FUN_PTR)(int a, int b);
void handler(int & sum, FUN_PTR f);
```

4. operator priority
    1. logical < arithmetic
    2. shift < arithmetic  shift > relation

5. ;
```c
// after if and while
if (flag);
    printf("hello")

if (flag)
    printf("hello")
```

5. Compute order : only `||` `&&` `?:` `,`
```c
i = 0;
while(i < n){
    y[i] = x[i++];
}
i ++ maybe calcauted before y[i] assigned, maybe after.
```

6. return value for `main` is important

7. getchar return int
```c
int main(){
    int c;
    while((c = getchar()) != EOF){
        putchar(c);
    }
    return 0;
}
```

8. Linker
    1. use `extern` to declare there is a variable.
    2. define a variable : declare and allocate memory


## C is not just C
C is os, compiler and arch.

### Pointer and array
> Pointer and array is not the same thing !
1. C only has **one** dimension array, high dimension are emulated !
2. Only two thing we can do to C array:
    1. get array size
    2. access the pointer pointing to begining of array

3. shorthand
```c
int b[10];
c[0] = 12;
0[c] = 12; // not recommended
```
4. As function parameter ,array will be transform to pointer automaticall.
And this is the reason why both of pointer and array call pass to pointer parameter.
```c
void func(int a[10][10]){

}

void func(int (*a)[10]){

}
```
But this kind of transformation is only appear in function parameter.
```c
extern char * hello;
extern char [] hello;
```

## C 语言的额外审查
https://rules.sonarsource.com/c/


## c 的一些库
https://github.com/attractivechaos/klib

## 整理的挺好的
https://devdocs.io/c/numeric/math/acosh


https://blogs.gentoo.org/mgorny/2024/09/23/overview-of-cross-architecture-portability-problems/

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
