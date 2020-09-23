# cmake

https://github.com/onqtam/awesome-cmake

https://github.com/wzpan/cmake-demo

https://github.com/xiaoweiChen/CMake-Cookbook : 一本书的翻译，其实是讲解的很清晰的了

https://github.com/Akagi201/learning-cmake
> docs　下面还有三个pdf 都是值得一读

https://github.com/forexample/package-example : cmake 模板 ?

## hello-world
```
cmake -H. -B_builds
cmake --build _builds

cmake ../
make

mkdir _n
cd _n
cmake -G "Ninja" ..
ninja
```

cmake : 
1. Generate a Project Buildsystem
2. Build a project : make 

### cmake lang
```
cmake_minimum_required(VERSION 3.7.1)

project(hello-world)　// @todo 

set(SOURCE_FILES main.c)

message(STATUS "This is BINARY dir " ${PROJECT_BINARY_DIR})
message(STATUS "This is SOURCE dir " ${PROJECT_SOURCE_DIR})

add_executable(hello-world ${SOURCE_FILES})
```

1. set
2. message
3. add_executable
4. project

1. 生成多个binary 
2. 文件之间的依赖是如何实现的
3. 批量加入文件


### hello-world-clear


```
cmake_minimum_required(VERSION 2.8.4)

project(hello)

add_subdirectory(src)

INSTALL(FILES COPYRIGHT README.md DESTINATION share/doc/cmake/hello-world-clear)

INSTALL(PROGRAMS runhello.sh DESTINATION bin)

INSTALL(DIRECTORY doc/ DESTINATION share/doc/cmake/hello-world-clear)
```

```
cmake_minimum_required(VERSION 2.8.4)

add_executable(hello main.c)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin) // @tood 不知道这一句话的影响是什么

install(TARGETS hello RUNTIME DESTINATION bin)
```

1. add_subdirectory
2. install

1. install 就是文件拷贝，文件夹拷贝以及拷贝之后的权限设置吗 ?
2. EXECUTABLE_OUTPUT_PATH 之类的，还有什么之类环境定义的变量
3. install 参数 RUNTIME 


### hello-world-lib

```
cmake_minimum_required(VERSION 2.8.4)

set(LIBHELLO_SRC hello.c)

add_library(hello_dynamic SHARED ${LIBHELLO_SRC})
add_library(hello_static STATIC ${LIBHELLO_SRC})

set_target_properties(hello_dynamic PROPERTIES OUTPUT_NAME "hello")
set_target_properties(hello_dynamic PROPERTIES VERSION 1.2 SOVERSION 1)

set_target_properties(hello_static PROPERTIES OUTPUT_NAME "hello")

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib) // @tood 不知道这一句话的影响是什么

install(TARGETS hello_dynamic hello_static
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
        
install(FILES hello.h DESTINATION include/hello)
```

1. add_library
2. set_target_properties

### hello-world-shared

```
cmake_minimum_required(VERSION 2.8.4)

project(newhello)

add_subdirectory(src)
```


```
cmake_minimum_required(VERSION 2.8.4)

add_executable(main main.c)

include_directories(../include/hello)

find_library(HELLO_LIB NAMES hello PATHS "../lib/") // @todo 为什么不是 ../lib/

message(STATUS "Library path HELLO_LIB is " ${HELLO_LIB})

target_link_libraries(main ${HELLO_LIB})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
```

1. find_library 




### curl

```
cmake_minimum_required(VERSION 2.8.4)

add_executable(curltest main.c)

#include_directories(/usr/local/include)
#target_link_libraries(curltest curl)

find_package(curl)

if(CURL_FOUND)
  include_directories(${CURL_INCLUDE_DIR})
  target_link_libraries(curltest ${CURL_LIBRARY})
else(CURL_FOUND)
  message(FATAL_ERROR "CURL library not found")
endif(CURL_FOUND)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin) // @todo again why ?
```

1. find_package
2.

> @todo
> 1. find_package 为什么总是失败
> 2. 如何使用Makefile 如何处理ld 的 complaint


1. https://stackoverflow.com/questions/20746936/what-use-is-find-package-if-you-need-to-specify-cmake-module-path-anyway

### hello-module

查找pkg的具体方法:

FindHello.module
```CMakeLists.txt
find_path(HELLO_INCLUDE_DIR hello.h /tmp/usr/include/hello)
find_library(HELLO_LIBRARY NAMES hello PATH /tmp/usr/lib)

if(HELLO_INCLUDE_DIR AND HELLO_LIBRARY)
  SET(HELLO_FOUND true)
endif(HELLO_INCLUDE_DIR AND HELLO_LIBRARY)

if(HELLO_FOUND)
  if(NOT HELLO_FIND_QUIETLY)
    message(STATUS "Found Hello: ${HELLO_INCLUDE_DIR} ${HELLO_LIBRARY}")
  endif(NOT HELLO_FIND_QUIETLY)
else(HELLO_FOUND)
  if(HELLO_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find hello library")
  endif(HELLO_FIND_REQUIRED)
endif(HELLO_FOUND)
```
> HELLO_FIND_REQUIRED 和 HELLO_FIND_QUIETLY 都是表示什么东西 ?

```
cmake_minimum_required(VERSION 2.8.4)

find_package(HELLO)

if(HELLO_FOUND)
  add_executable(hello main.c)
  include_directories(${HELLO_INCLUDE_DIR})
  target_link_libraries(hello ${HELLO_LIBRARY})
else(HELLO_FOUND)
  message(FATAL_ERROR "HELLO library not found")
endif(HELLO_FOUND)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
```

```
cmake_minimum_required(VERSION 2.8.4)

project(hello)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

add_subdirectory(src)
```

### config-file
ref
* <https://cmake.org/Wiki/CMake:How_To_Write_Platform_Checks>
* <https://sidvind.com/wiki/CMake/Configuration_files>
* <https://github.com/libical/libical/blob/master/config.h.cmake>
* <https://github.com/brndnmtthws/conky/blob/master/cmake/build.h.in>
> @todo confusing totally !

### hunter-simple
> @todo not useful now !

### boost

```
project(boost_demo)
cmake_minimum_required(VERSION 3.0)

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})
set(Boost_USE_STATIC_LIBS        OFF)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME     OFF)
set(BOOST_ALL_DYN_LINK           ON)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
```
> 比想象的简单的多，但是中间各种设置 Boost_USE_STATIC_RUNTIME 之类的变量是做什么的



## ref 
1. https://stackoverflow.com/questions/31090821/what-does-the-h-option-means-for-cmake
> H 的含义被替换为S，S表示source code的文件夹

# https://cgold.readthedocs.io/en/latest/

# https://github.com/ttroy50/cmake-examples

# res
1. https://cmake.org/cmake/help/latest/index.html 标准文档

