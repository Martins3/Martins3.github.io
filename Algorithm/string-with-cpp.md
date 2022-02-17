---
title: String_With_Cpp
date: 2018-04-24 20:34:02
tags: algorithm
---
# String alrotithm described by cpp

# Handling string with cpp

## IO
1. 从line中间抠出word
```cpp
    void get_words(vector<string> & vec, char * line){
        int left = 0;
        int right = 0;

        while(line[left] != '\0'){
            while(line[left] != '\0'){
            if(isspace(line[left])) break;
            left ++;
            }
            right = left;
            while(!isspace(line[right])) right ++;
            // not included right
            string word(line + left, line + right);
            // line may maybe end with space
            if(word.size()) vec.push_back(word);
            // find the next word
            left = right;
        }
    }
```

## 一行行的读文件

```cpp
    #include <fstream>
    string line;
    std::ifstream infile("thefile.txt");
    while (std::getline(infile, line)){
        std::istringstream iss(line);
        int a, b;
        if (!(iss >> a >> b)) { break; } // error
    }
```

## 字符串中间查找
```cpp
    find
```

## 字符串含有分隔符
http://ysonggit.github.io/coding/2014/12/16/split-a-string-using-c.html
```cpp
std::string s = "scott>=tiger>=mushroom";
std::string delimiter = ">=";

size_t pos = 0;
std::string token;
while ((pos = s.find(delimiter)) != std::string::npos) {
    token = s.substr(0, pos);
    std::cout << token << std::endl;
    s.erase(0, pos + delimiter.length());
}
std::cout << s << std::endl;
```

## trim
```
boost::trim()
```

## 构造函数
1. 创建相同字母字符串




# Cpp String
https://www.fluentcpp.com/2017/04/21/how-to-split-a-string-in-c/

## 常用函数
1. stoi
2. string 是可以改变的
3. find 字符串包含 int l = str1.find(str2);
4. char 大小写切换 tolower() toupper() islower isupper
5. substr()
```
std::string str="We think in generalities, but we live in details.";
                                           // (quoting Alfred N. Whitehead)

  std::string str2 = str.substr (3,5);     // "think"

  std::size_t pos = str.find("live");      // position of "live" in str

  std::string str3 = str.substr (pos);     // get from "live" to the end

  std::cout << str2 << ' ' << str3 << '\n';
```
6. isdigit

## IO
1. fgets()
Reads characters from stream and stores them as a C string into str until (num-1)
characters have been read or either a newline or the end-of-file is reached, whichever happens first.
A newline character makes fgets stop reading, but it is considered a valid character by the function and included in the string copied to str.



2. scanf 读取str 的时候， 前面的空字符必定会被抛弃， 后面的不会处理， 也就是下一次读取的时候继续
## spilt
```
std::string s = "scott>=tiger>=mushroom";
std::string delimiter = ">=";

size_t pos = 0;
std::string token;
while ((pos = s.find(delimiter)) != std::string::npos) {
    token = s.substr(0, pos);
    std::cout << token << std::endl;
    s.erase(0, pos + delimiter.length());
}
std::cout << s << std::endl;
```

## string 和 char[] 的转换
1.
```
char arr[];
string a(arr)

string temp = "cat";
char tab2[1024];
strcpy(tab2, temp.c_str());
```
## 初始化String的几种方法
```
string s;

```

## 整数 和 string 之间转化
1. to_string
2. stoi atoi
3. \>\> 运算符
4. sscanf


## 来自于 cstring 常用函数
1. strcpy
2. scanf
Any number of non-whitespace characters, stopping at the first whitespace character found. A terminating null character is automatically added at the end of the stored sequence.
3. sscanf
4. getline

## 正则表达式
1. strchr
returns a pointer to the first occurrence of character in the C string str.

## contains
```
if (s1.find(s2) != std::string::npos) {
    std::cout << "found!" << '\n';
}
```

## sstream
str(), which returns the contents of its buffer in string type.
str(string), which set the contents of the buffer to the string argument.

## trim and reduce
```
#include <iostream>
#include <string>

std::string trim(const std::string& str,
                 const std::string& whitespace = " \t"){
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return ""; // no content

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

std::string reduce(const std::string& str,
                   const std::string& fill = " ",
                   const std::string& whitespace = " \t"){
    // trim first
    auto result = trim(str, whitespace);

    // replace sub ranges
    auto beginSpace = result.find_first_of(whitespace);
    while (beginSpace != std::string::npos)
    {
        const auto endSpace = result.find_first_not_of(whitespace, beginSpace);
        const auto range = endSpace - beginSpace;

        result.replace(beginSpace, range, fill);

        const auto newStart = beginSpace + fill.length();
        beginSpace = result.find_first_of(whitespace, newStart);
    }

    return result;
}

int main(void){
    const std::string foo = "    too much\t   \tspace\t\t\t  ";
    const std::string bar = "one\ntwo";

    std::cout << "[" << trim(foo) << "]" << std::endl;
    std::cout << "[" << reduce(foo) << "]" << std::endl;
    std::cout << "[" << reduce(foo, "-") << "]" << std::endl;

    std::cout << "[" << trim(bar) << "]" << std::endl;
}
```

## MISC
1. 实现对于类对象的排序[^1]

```cpp
class MyStruct{
    int key;
    std::string stringValue;

    MyStruct(int k, const std::string& s) : key(k), stringValue(s) {}

    bool operator < (const MyStruct& str) const{
        return (key < str.key);
    }
};
std::sort(vec.begin(), vec.end());

struct MyStruct{
    int key;
    std::string stringValue;

    MyStruct(int k, const std::string& s) : key(k), stringValue(s) {}

    bool operator > (const MyStruct& str) const{
        return (key > str.key);
    }
};
std::sort(vec.begin(), vec.end(),greater<MyStruct>());


bool comp(const ClassOne& lhs, const ClassOne& rhs)
{
  return lhs.valueData < rhs.valueData;
}

std::sort(cone, cone+10, comp);
or, in C++11,

std::sort(std::begin(cone), std::end(cone), comp);


struct MyStruct
{
    int key;
    std::string stringValue;

    MyStruct(int k, const std::string& s) : key(k), stringValue(s) {}
};

struct less_than_key
{
    bool operator() (const MyStruct& struct1, const MyStruct& struct2)
    {
        return (struct1.key < struct2.key);
    }
};
std::sort(vec.begin(), vec.end(), less_than_key());


// 使用 lambda
sort(a.begin(), a.end(),
       [](const MyStruct &struct1, const MyStruct &struct2) -> bool
       {
         return (struct1.key < struct2.key);
       });
```
总结 : 要么定义为函数，要么定义为 lambda

2. memset 处理二维数组

## priority_queue
- [ ] 和 heap 是什么关系 ?

## ques
1. 最快读入的string的方法是什么 ？

[^1]: https://stackoverflow.com/questions/1380463/sorting-a-vector-of-custom-objects
