add_library(codegen-module STATIC
    src/byte_code.cpp
    src/chunk.cpp
    src/codegen.cpp
    src/opcode.cpp
    src/var_meta.cpp
)

target_include_directories(codegen-module PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(codegen-module PUBLIC cxx_std_23)
