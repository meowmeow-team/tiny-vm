cmake_minimum_required(VERSION 4.4.4)
project(tiny-vm LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(lexer)

add_executable(tiny-vm
    core/main.cpp
    core/logger.cpp
    core/exception.cpp
    core/source_manager.cpp
)

target_link_libraries(tiny-vm PRIVATE lexer)
