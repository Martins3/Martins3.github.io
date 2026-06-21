cmake_minimum_required(VERSION 3.15)
project(WindowsAPITests)

# Enable compile_commands.json generation
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows specific libraries
# find_package(WindowsSDK REQUIRED) # Removed as it might not be found in all environments

# Add all test executables directly
add_executable(win_file_io_demo win_file_io_demo.cpp)
add_executable(win_heap_demo win_heap_demo.cpp)
add_executable(win_lock_demo win_lock_demo.cpp)
add_executable(win_process_demo win_process_demo.cpp)
add_executable(win_echo_server win_echo_server.cpp)
add_executable(win_echo_client win_echo_client.cpp)
add_executable(win_echo_server_iocp win_echo_server_iocp.cpp)

# Link Windows libraries
target_link_libraries(win_file_io_demo PRIVATE kernel32.lib)
target_link_libraries(win_heap_demo PRIVATE kernel32.lib)
target_link_libraries(win_lock_demo PRIVATE kernel32.lib)
target_link_libraries(win_process_demo PRIVATE kernel32.lib)
target_link_libraries(win_echo_server PRIVATE kernel32.lib ws2_32.lib)
target_link_libraries(win_echo_client PRIVATE kernel32.lib ws2_32.lib)
target_link_libraries(win_echo_server_iocp PRIVATE kernel32.lib ws2_32.lib mswsock.lib)

# Set output directory
set_target_properties(
    win_file_io_demo 
    win_heap_demo 
    win_lock_demo 
    win_process_demo 
    win_echo_server
    win_echo_client
    win_echo_server_iocp
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)