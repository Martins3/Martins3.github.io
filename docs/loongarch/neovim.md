# 在 3a5000 上构建 neovim 0.7
似乎主要是第三方库无法正常的使用:

https://github.com/neovim/neovim/wiki/Building-Neovim#how-to-build-without-bundled-dependencies

似乎没有找到
```txt
CMake Error at /usr/share/cmake-3.13/Modules/FindPackageHandleStandardArgs.cmake:137 (message):
  Could NOT find LibLUV (missing: LIBLUV_LIBRARY LIBLUV_INCLUDE_DIR)
  (Required is at least version "1.30.0")
Call Stack (most recent call first):
  /usr/share/cmake-3.13/Modules/FindPackageHandleStandardArgs.cmake:378 (_FPHSA_FAILURE_MESSAGE)
  cmake/FindLibLUV.cmake:29 (find_package_handle_standard_args)
  CMakeLists.txt:390 (find_package)
```

cmake ../third-party -DUSE_BUNDLED=OFF -DUSE_BUNDLED_LIBUV=ON -DUSE_BUNDLED_LUV=ON -DUSE_BUNDLED_LIBVTERM=ON -DUSE_BUNDLED_TS=ON

sudo apt install libluajit-5.1-dev
sudo apt install libunibilium-dev
sudo apt install libtermkey-dev
sudo luarocks install mpack


在 install luarocks 的过程中发现:
```txt
lmpack.c:25:10: 致命错误：lauxlib.h：没有那个文件或目录
 #include <lauxlib.h>
          ^~~~~~~~~~~
编译中断。
```
原来是需要安装
sudo apt install liblua5.3-dev

## 尽量自己编译吧
make BUNDLED_CMAKE_FLAG="-DUSE_BUNDLED=ON -DUSE_BUNDLED_LUAJIT=OFF -DUSE_BUNDLED_LUAROCKS=OFF -DLUA_PRG=/usr/bin/lua"
```txt
/usr/bin/luajit: module 'lpeg' not found:
        no field package.preload['lpeg']
        no file './lpeg.lua'
        no file '/usr/local/share/luajit-2.1.0-beta3/lpeg.lua'
        no file '/usr/local/share/lua/5.1/lpeg.lua'
        no file '/usr/local/share/lua/5.1/lpeg/init.lua'
        no file './lpeg.so'
        no file '/usr/local/lib/lua/5.1/lpeg.so'
        no file '/usr/local/lib/lua/5.1/loadall.so'
stack traceback:
        [C]: at 0x012009521c
        [C]: at 0x012000604c
-- [/usr/bin/luajit] The 'lpeg' lua package is required for building Neovim
-- Checking Lua interpreter: /usr/bin/lua
/usr/bin/lua: module 'bit' not found:
        no field package.preload['bit']
        no file '/usr/local/share/lua/5.3/bit.lua'
        no file '/usr/local/share/lua/5.3/bit/init.lua'
        no file '/usr/local/lib/lua/5.3/bit.lua'
        no file '/usr/local/lib/lua/5.3/bit/init.lua'
        no file '/usr/share/lua/5.3/bit.lua'
        no file '/usr/share/lua/5.3/bit/init.lua'
        no file './bit.lua'
        no file './bit/init.lua'
        no file '/usr/local/lib/lua/5.3/bit.so'
        no file '/usr/lib/loongarch64-linux-gnu/lua/5.3/bit.so'
        no file '/usr/lib/lua/5.3/bit.so'
        no file '/usr/local/lib/lua/5.3/loadall.so'
        no file './bit.so'
stack traceback:
        [C]: in function 'require'
        [C]: in ?
-- [/usr/bin/lua] The 'bit' lua package is required for building Neovim
CMake Error at CMakeLists.txt:541 (message):
  Failed to find a Lua 5.1-compatible interpreter


-- Configuring incomplete, errors occurred!
See also "/home/loongson/arch/neovim/build/CMakeFiles/CMakeOutput.log".
See also "/home/loongson/arch/neovim/build/CMakeFiles/CMakeError.log".
make: *** [Makefile:102：build/.ran-cmake] 错误 1
```

但是显然安装过 lpeg
```txt
/usr/lib/loongarch64-linux-gnu/liblua5.1-lpeg.so.2
/usr/lib/loongarch64-linux-gnu/liblua5.1-lpeg.so.2.0.0
/usr/lib/loongarch64-linux-gnu/liblua5.2-lpeg.so.2
/usr/lib/loongarch64-linux-gnu/liblua5.2-lpeg.so.2.0.0
/usr/lib/loongarch64-linux-gnu/liblua5.3-lpeg.so.2
/usr/lib/loongarch64-linux-gnu/liblua5.3-lpeg.so.2.0.0
/usr/lib/loongarch64-linux-gnu/lua/5.1/lpeg.so
/usr/lib/loongarch64-linux-gnu/lua/5.2/lpeg.so
/usr/lib/loongarch64-linux-gnu/lua/5.3/lpeg.so
/usr/local/lib/lua/5.3/lpeg.so
/usr/local/lib/luarocks/rocks/lpeg
/usr/local/lib/luarocks/rocks/lpeg/1.0.2-1
/usr/local/lib/luarocks/rocks/lpeg/1.0.2-1/lpeg-1.0.2-1.rockspec
/usr/local/lib/luarocks/rocks/lpeg/1.0.2-1/rock_manifest
/usr/share/doc/lua-lpeg
/usr/share/doc/lua-lpeg/changelog.Debian.gz
/usr/share/doc/lua-lpeg/changelog.gz
/usr/share/doc/lua-lpeg/copyright
/var/lib/dpkg/info/lua-lpeg:loongarch64.list
/var/lib/dpkg/info/lua-lpeg:loongarch64.md5sums
/var/lib/dpkg/info/lua-lpeg:loongarch64.shlibs
/var/lib/dpkg/info/lua-lpeg:loongarch64.triggers
```
现在，问题在于 lpeg 现在  lua/5.3 下面


## 似乎 mpack 需要和 luajit 的版本绑定
```txt
-- CMAKE_INSTALL_PREFIX=/usr/local
-- CMAKE_BUILD_TYPE=Debug
-- MIN_LOG_LEVEL not specified, default is 1 (INFO)
-- Replacing -O3 in CMAKE_C_FLAGS_RELEASE with -O2
-- Found TreeSitter 0.6.3
-- Found UNIBILIUM 2.0.0
-- Found LIBVTERM 0.1.4
-- Found Iconv
-- Checking Lua interpreter: /usr/bin/luajit
/usr/bin/luajit: module 'mpack' not found:
        no field package.preload['mpack']
        no file './mpack.lua'
        no file '/usr/share/luajit-2.1.0-beta3/mpack.lua'
        no file '/usr/local/share/lua/5.1/mpack.lua'
        no file '/usr/local/share/lua/5.1/mpack/init.lua'
        no file '/usr/share/lua/5.1/mpack.lua'
        no file '/usr/share/lua/5.1/mpack/init.lua'
        no file './mpack.so'
        no file '/usr/local/lib/lua/5.1/mpack.so'
        no file '/usr/lib/loongarch64-linux-gnu/lua/5.1/mpack.so'
        no file '/usr/local/lib/lua/5.1/loadall.so'
stack traceback:
        [C]: at 0x012005c264
        [C]: at 0x0120005300
-- [/usr/bin/luajit] The 'mpack' lua package is required for building Neovim
-- Checking Lua interpreter: /usr/bin/lua
/usr/bin/lua: module 'bit' not found:
        no field package.preload['bit']
        no file '/usr/local/share/lua/5.3/bit.lua'
        no file '/usr/local/share/lua/5.3/bit/init.lua'
        no file '/usr/local/lib/lua/5.3/bit.lua'
        no file '/usr/local/lib/lua/5.3/bit/init.lua'
        no file '/usr/share/lua/5.3/bit.lua'
        no file '/usr/share/lua/5.3/bit/init.lua'
        no file './bit.lua'
        no file './bit/init.lua'
        no file '/usr/local/lib/lua/5.3/bit.so'
        no file '/usr/lib/loongarch64-linux-gnu/lua/5.3/bit.so'
        no file '/usr/lib/lua/5.3/bit.so'
        no file '/usr/local/lib/lua/5.3/loadall.so'
        no file './bit.so'
stack traceback:
        [C]: in function 'require'
        [C]: in ?
-- [/usr/bin/lua] The 'bit' lua package is required for building Neovim
CMake Error at CMakeLists.txt:541 (message):
  Failed to find a Lua 5.1-compatible interpreter


-- Configuring incomplete, errors occurred!
See also "/home/loongson/arch/neovim/build/CMakeFiles/CMakeOutput.log".
See also "/home/loongson/arch/neovim/build/CMakeFiles/CMakeError.log".
```

## 使用 cmake .. 编译出现问题
```txt
/home/loongson/arch/neovim/src/nvim/log.c: 在函数‘v_do_log_to_file’中:
/home/loongson/arch/neovim/src/nvim/log.c:306:3: 错误：unknown type name ‘uv_timeval64_t’; did you mean ‘uv_timeval_t’?
   uv_timeval64_t curtime;
   ^~~~~~~~~~~~~~
   uv_timeval_t
/home/loongson/arch/neovim/src/nvim/log.c:307:7: 警告：implicit declaration of function ‘uv_gettimeofday’; did you mean ‘uv_getnameinfo’? [-Wimplicit-function-declaration]
   if (uv_gettimeofday(&curtime) == 0) {
       ^~~~~~~~~~~~~~~
       uv_getnameinfo
/home/loongson/arch/neovim/src/nvim/log.c:308:26: 错误：在非结构或联合中请求成员‘tv_usec’
     millis = (int)curtime.tv_usec / 1000;
                          ^
[100%] Building C object src/nvim/CMakeFiles/nvim.dir/lua/spell.c.o
make[2]: *** [src/nvim/CMakeFiles/nvim.dir/build.make:2374：src/nvim/CMakeFiles/nvim.dir/log.c.o] 错误 1
make[2]: *** 正在等待未完成的任务....
make[1]: *** [CMakeFiles/Makefile2:4934：src/nvim/CMakeFiles/nvim.dir/all] 错误 2
make: *** [Makefile:152：all] 错误 2
```
因为这需要更新 libuv 的库了。

## 直接编译，放过 lua
make BUNDLED_CMAKE_FLAG="-DUSE_BUNDLED=ON -DUSE_BUNDLED_LUAJIT=OFF"


## fs_opendir 找不到
在 loongson 和 x86 上测试下面的代码，可以发现 fs_opendir 不存在
```c
local start_dir_handle = vim.loop.fs_opendir("/home/maritns3/core/vn/", nil, 50)
print(vim.inspect(vim))
print(vim)
print(vim.loop)
print(vim.loop.fs_opendir)
print("this is me !")
print(start_dir_handle)
```

找了半天，最后发现是在 https://github.com/luvit/luv/search?q=fs_opendir 中

直接搬运到 Lua 中，发现 luv 是永远都卡到这里了:
cmake ../third-party -DUSE_BUNDLED=OFF -DUSE_BUNDLED_LUV=ON

## nodejs
是我错怪 coc.nvim 了，直接 clone 然后 checkout 到 release 就可以了

- [ ] coc 的插件总是失败
- [ ] python provider 失败

## 安装 pyright
这个代码需要用户态的上下文切换，这个部分和汇编代码相关，我不像再去尝试了
https://github.com/python-greenlet/greenlet

## 设置终端
