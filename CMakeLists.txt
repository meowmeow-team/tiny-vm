cmake_minimum_required(VERSION 4.4.4)
project(tiny-vm LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(lexer)
add_subdirectory(parser)
add_subdirectory(semantic_analysis)

add_executable(tiny-vm
    core/main.cpp
    core/logger.cpp
    core/exception.cpp
    core/source_manager.cpp
)

target_link_libraries(tiny-vm PRIVATE lexer)
target_link_libraries(tiny-vm PRIVATE parser)
target_link_libraries(tiny-vm PRIVATE semantic_analysis)

if(BUILD_TESTING)
    include(CTest)
    include(FetchContent)

    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.2
    )
    FetchContent_MakeAvailable(Catch2)

    enable_testing()
    add_subdirectory(tests)
endif()
