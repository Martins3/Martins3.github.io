> cmake 缺少的不是 checksheet，而是他到底是什么东西 ? cmake 的设计模式到底是什么 ?

https://cliutils.gitlab.io/modern-cmake/
https://learnxinyminutes.com/docs/cmake/ : 好像还不错

## CMakeLists 常用语句
1. target_link_libraries
2. add_executable : 
3. 
```
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -g -m64")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -lssl -lcrypto -m64 -lpthread")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)


SET(VERSION 1.0.0.0)

```

4. 三个基本设置: 可以构成基本的模板了!

```
SET(VERSION 1.0.0.0)


SET(SOURCE_DIRECTORY
    ${PROJECT_SOURCE_DIR}/eomaia/base/
    ${PROJECT_SOURCE_DIR}/eomaia/net/
    )
    
SET(INCLUDE_DIRECTORY
    ${PROJECT_SOURCE_DIR}/eomaia/
    )
    
SET(OUTPUT_DIR 
    "${PROJECT_SOURCE_DIR}/eomaia_lib")


INCLUDE_DIRECTORIES(${INCLUDE_DIRECTORY})

FOREACH(SOURCE_DIR ${SOURCE_DIRECTORY})
    AUX_SOURCE_DIRECTORY(${SOURCE_DIR} SOURCES)
ENDFOREACH()

SET_TARGET_PROPERTIES(${OUTPUT_STATIC} PROPERTIES  LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}" )
SET_TARGET_PROPERTIES(${OUTPUT_SHARED} PROPERTIES  LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}" )
```

5. 
```c
if(ANDROID)
else()
endif()
```

6. find_package : https://github.com/conan-io/examples/tree/master/features/cmake/find_package/exported_targets_multiconfig

