add_library(parser STATIC
    src/ast.cpp
    src/parser.cpp
)

target_include_directories(parser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(parser PUBLIC cxx_std_23)
