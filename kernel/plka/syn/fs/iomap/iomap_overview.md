# fs/iomap

| fiel          | blank | comment | code | desc                                                                 |
|---------------|-------|---------|------|----------------------------------------------------------------------|
| buffered-io.c | 228   | 281     | 1146 |                                                                      |
| direct-io.c   | 76    | 107     | 392  | 唯二的 non static 函数 iomap_dio_rw 的唯一调用者 ext4_file_read_iter |
| trace.h       | 15    | 7       | 169  |                                                                      |
| seek.c        | 29    | 30      | 153  |                                                                      |
| fiemap.c      | 20    | 7       | 121  | 函数无人调用                                                         |
| swapfile.c    | 18    | 43      | 118  |                                                                      |
| apply.c       | 8     | 48      | 38   |                                                                      |
| Makefile      | 3     | 5       | 9    |                                                                      |
| trace.c       | 1     | 8       | 3    |                                                                      |


1. 对于设备 io mmap 找到具体的位置 ？
2. 这似乎根本不是我们想要的东西 ?

## TODO
