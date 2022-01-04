# musl
website : https://www.musl-libc.org/
github mirror : https://github.com/ifduyue/musl

I came acorss musl when compiling the project firecracker. Being disgusted with glibc for years,
I want to find a substitute for it from time to time. Glibc has too many macros, C language tricks, I bet noting will you understand with browsing the source code.
Here's simple comparison about [LOC](https://en.wikipedia.org/wiki/Source_lines_of_code) between musl and glibc

- [ ] https://news.ycombinator.com/item?id=21291893

musl
```
github.com/AlDanial/cloc v 1.82  T=0.95 s (2513.4 files/s, 193017.3 lines/s)
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
JSON                             1              0              0          71397
C                             1563           6612           7473          50762
C/C++ Header                   539           4513            285          33124
Assembly                       276            515            641           6532
Bourne Shell                     5            115            169            645
awk                              3             56             74            301
make                             1             68             10            159
sed                              1              0              6              9
-------------------------------------------------------------------------------
SUM:                          2389          11879           8658         162929
-------------------------------------------------------------------------------
```

glibc
```
github.com/AlDanial/cloc v 1.82  T=21.03 s (1028.0 files/s, 280970.3 lines/s)
---------------------------------------------------------------------------------------
Language                             files          blank        comment           code
---------------------------------------------------------------------------------------
D                                     4076         820632              0        1313609
DIET                                  2657         449605              0         721526
C                                     9249         115647         175829         682981
JSON                                     3              0              0         493775
C/C++ Header                          3217          40145          66415         369962
Assembly                              1828          39920          93537         234778
PO File                                 43          40870          55813         106389
Bourne Shell                            88           2037           2656          16058
make                                   207           2652           2642          12223
Python                                  52           1363           3812           8792
TeX                                      1            813           3697           7162
m4                                      55            355            167           3445
Windows Module Definition               18            190              0           2937
SWIG                                     3             12              0           2808
Verilog-SystemVerilog                    2              0              0           2748
awk                                     36            266            368           1890
Bourne Again Shell                       6            212            297           1850
C++                                     40            326            456           1382
Perl                                     4            103            165            830
Korn Shell                               1             55             68            435
yacc                                     1             56             36            290
Pascal                                   7             82            326            182
Expect                                   8              0              0             77
sed                                     11              8             15             70
Gencat NLS                               2              0              3             10
---------------------------------------------------------------------------------------
SUM:                                 21615        1515349         406302        3986209
---------------------------------------------------------------------------------------
```
Although musl's code is clean, it's impossible and laborious to inspect it line by line,
here is What I learn from it.
## compile musl
./configure && make install

## 编译 glibc

[hacking](https://stackoverflow.com/questions/10412684/how-to-compile-my-own-glibc-c-standard-library-from-source-and-use-it)
