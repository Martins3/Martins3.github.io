# Linux Kernel Hacks

<!-- vim-markdown-toc GitLab -->

- [Why](#why)
- [Status](#status)
- [Todo](#todo)
- [License](#license)
    - [License for code](#license-for-code)
    - [License for Documentation](#license-for-documentation)

<!-- vim-markdown-toc -->


## Why
1. linux 内核文档内容陈旧，残缺不全，对于新人非常不友好
2. 没有配套的代码，学习过程将会非常无聊，并且理解会出现偏差。

## Status
🚧 🚧 🚧 🚧 🚧 🚧 
| module  | process rate | TODO                                           |
|---------|--------------|------------------------------------------------|
| memory  | 70           |                                                |
| fs      | 60           | mount's new interface                          |
| process | 50           | scheduler                                      |
| block   | 5            |                                                |
| net     | 0            | read the book *linux kernel network internals* |
| lock    | 2            |                                                |
🚧 🚧 🚧 🚧 🚧 🚧 

## Todo
- [ ] mmdrop()
- [ ] mmgrab()
- [ ] vm_normal_page()
  - [ ] why some page can work without `struct page`
  - [ ] check comments above it
  - [ ] do_wp_page's reference

- [ ] https://www.kernel.org/doc/html/latest/core-api/mm-api.html# : check the doc
- [ ] https://www.kernel.org/doc/gorman/html/understand/ : check the book
- [ ] access_ok()

## License
#### License for code
```txt
Copyright © 2020 martins3

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
```
#### License for Documentation
转发 **CSDN** 按侵权追究法律责任，其它情况随意。
