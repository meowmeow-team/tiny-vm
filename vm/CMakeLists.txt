add_library(virtual-machine STATIC
    src/vm.cpp
)

target_include_directories(virtual-machine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(virtual-machine PUBLIC cxx_std_23)
