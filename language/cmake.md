# My cmake Notes

## basic
cmake build projects in three steps:
1. create dir
2. cmake
3. make

```sh
cmake -Bbuilds
cmake --build builds

mkdir _build
cmake ../
make

mkdir _n
cd _n
cmake -G "Ninja" ..
ninja
```

### Commands
1. `project` : set the name of project.
2. `add_subdirectory` : add a subdirectory to project.
3. `install` : Specify rules to run at install time
    - copy file to /usr/lib/, /usr/bin
4. `set` : Set a normal, cache, or environment variable to a given value. 
    - Multi-item variable : https://stackoverflow.com/questions/31037882/whats-the-cmake-syntax-to-set-and-use-variables
5. `target_source` : add_executable and target_source ?
    - [add further source files to this executable after this line but in the same or an included CMakeList.txt file after having defined an executable with `ADD_EXECUTABLE(MyExampleApp main.cpp)`](https://stackoverflow.com/questions/9339851/can-one-add-further-source-files-to-an-executable-once-defined)
6. `add_library` : Add a library to the project using the specified source files.
7. `add_executable` : Add an executable to the project using the specified source files.
8. `include_directories` : Add include directories to the build.
    - CMAKE_INCLUDE_CURRENT_DIR : automatically add the current source and build directories to the include path.
    - target_include_directories : https://stackoverflow.com/questions/31969547/what-is-the-difference-between-include-directories-and-target-include-directorie
9. `target_link_libraries` : Specify libraries or flags to use when linking a given target and/or its dependents.
    - [Do not use link_directories like this in CMake.](https://stackoverflow.com/questions/31438916/cmake-cannot-find-library-using-link-directories)
10. find_package
  - [ ] https://stackoverflow.com/questions/20746936/what-use-is-find-package-if-you-need-to-specify-cmake-module-path-anyway
12. add_definitions
    - add_definitions(-DDEBUG)

## [Variables](https://gitlab.kitware.com/cmake/community/-/wikis/doc/cmake/Useful-Variables)
1. Locations : location constant variable
    - CMAKE_SOURCE_DIR
    - PROJECT_BINARY_DIR 
    - CMAKE_CURRENT_SOURCE_DIR 
    - CMAKE_MODULE_PATH
2. Environment Variables
    - CMAKE_INCLUDE_PATH
    - CMAKE_LIBRARY_PATH
3. System & Compiler Information
    - CMAKE_MAJOR_VERSION
4. Various Options
5. Compilers and Tools
    - CMAKE_C_COMPILER

## Advance Topic
1. [Custom command](https://gist.github.com/baiwfg2/39881ba703e9c74e95366ed422641609) : mimic Makefile style


## reference
- https://github.com/ttroy50/cmake-examples :star: :star: :star:
- https://github.com/Akagi201/learning-cmake :star: provide examples, but some of them don't work.
- https://github.com/onqtam/awesome-cmake
- https://github.com/wzpan/cmake-demo
- https://github.com/xiaoweiChen/CMake-Cookbook : 一本书的翻译，其实是讲解的很清晰的了
- https://github.com/forexample/package-example : cmake 模板 ?
- https://cgold.readthedocs.io/en/latest/
