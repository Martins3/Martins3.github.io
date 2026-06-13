## rcu 基本代码基本分析

| Files           | Lines | Code | Comments | Blanks | Details         |
|-----------------|-------|------|----------|--------|-----------------|
| rcutorture.c    | 3912  | 3278 | 311      | 323    | 测试代码        |
| tree.c          | 4968  | 2853 | 1579     | 536    |
| srcutree.c      | 1981  | 1165 | 630      | 186    |
| refscale.c      | 1138  | 863  | 81       | 194    |
| rcuscale.c      | 982   | 754  | 99       | 129    |
| update.c        | 671   | 404  | 197      | 70     |
| rcu_segcblist.c | 633   | 313  | 259      | 61     |
| srcutiny.c      | 284   | 181  | 68       | 35     |
| tiny.c          | 263   | 170  | 64       | 29     |
| sync.c          | 206   | 85   | 100      | 21     |
| tasks.h         | 1976  | 1212 | 556      | 208    |
| tree_nocb.h     | 1762  | 1177 | 387      | 198    |
| tree_plugin.h   | 1305  | 757  | 429      | 119    |
| tree_stall.h    | 1056  | 736  | 210      | 110    | 处理 rcu stalll |
| tree_exp.h      | 1153  | 730  | 309      | 114    |
| rcu.h           | 645   | 402  | 160      | 83     |
| tree.h          | 509   | 306  | 161      | 42     |
| rcu_segcblist.h | 155   | 96   | 41       | 18     |


tree.c 包含的三个文件:
```c
#include "tree_stall.h"
#include "tree_exp.h"
#include "tree_nocb.h"
#include "tree_plugin.h"
```
